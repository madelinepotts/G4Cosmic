#include "G4Cosmic/CORSIKAPrimaryGenerator.hh"

#include "G4Cosmic/JsonWriter.hh"
#include "G4Cosmic/RunMetadata.hh"

#include "PrimaryRecord.hh"
#include "RunAction.hh"

#include "G4Event.hh"
#include "G4GenericMessenger.hh"
#include "G4LogicalVolume.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4RotationMatrix.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"
#include "G4ThreeVector.hh"
#include "G4TransportationManager.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VSolid.hh"
#include "G4ios.hh"
#ifdef G4MULTITHREADED
#include "G4AutoLock.hh"
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

#ifdef G4MULTITHREADED
G4Mutex gCorsikaIoMutex = G4MUTEX_INITIALIZER;
#endif

// Process-wide record of the batch cache produced during this executable run.
// In Geant4 MT mode each worker owns a PrimaryGeneratorAction, so instance
// members alone cannot tell worker N that worker 0 already generated the batch.
G4String gGeneratedBatchCommand;
G4String gGeneratedBatchCacheFile;
G4int gGeneratedBatchEventsPerBatch = 0;
G4int gGeneratedBatchCount = 0;

bool IsMasterThread()
{
#ifdef G4MULTITHREADED
    return G4Threading::IsMasterThread();
#else
    return true;
#endif
}

bool ShouldPrintVerbose(G4int verbose)
{
    return verbose != 0 && IsMasterThread();
}

std::string TrimComment(const std::string& line)
{
    const auto comment = line.find('#');
    return line.substr(0, comment);
}

bool IsBlank(const std::string& text)
{
    return text.find_first_not_of(" \t\r\n") == std::string::npos;
}

bool FileExists(const G4String& fileName)
{
    std::ifstream input(fileName);
    return static_cast<bool>(input);
}

std::string ToString(G4double value)
{
    std::ostringstream os;
    os << std::setprecision(12) << value;
    return os.str();
}

bool NeedsShellQuotes(const std::string& value)
{
    return value.find_first_of(" \t\"'") != std::string::npos;
}

std::string QuoteArgument(const std::string& value)
{
    if (value.empty()) {
        return "\"\"";
    }
    if (!NeedsShellQuotes(value)) {
        return value;
    }

    std::string quoted = "\"";
    for (const char ch : value) {
        if (ch == '\\' || ch == '"') {
            quoted.push_back('\\');
        }
        quoted.push_back(ch);
    }
    quoted.push_back('\"');
    return quoted;
}

void ReplaceAll(std::string& text, const std::string& from, const std::string& to)
{
    if (from.empty()) {
        return;
    }

    std::size_t start = 0;
    while ((start = text.find(from, start)) != std::string::npos) {
        text.replace(start, from.size(), to);
        start += to.size();
    }
}

bool WildcardMatches(const std::string& pattern, const std::string& text)
{
    std::size_t p = 0;
    std::size_t t = 0;
    std::size_t star = std::string::npos;
    std::size_t match = 0;

    while (t < text.size()) {
        if (p < pattern.size() &&
            (pattern[p] == '?' || pattern[p] == text[t])) {
            ++p;
            ++t;
        }
        else if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            match = t;
        }
        else if (star != std::string::npos) {
            p = star + 1;
            t = ++match;
        }
        else {
            return false;
        }
    }

    while (p < pattern.size() && pattern[p] == '*') {
        ++p;
    }

    return p == pattern.size();
}

bool RayIntersectsBox(const G4ThreeVector& origin,
                      const G4ThreeVector& direction,
                      const G4ThreeVector& boxMin,
                      const G4ThreeVector& boxMax)
{
    G4double tMin = 0.0;
    G4double tMax = std::numeric_limits<G4double>::infinity();

    const G4double o[3] = {origin.x(), origin.y(), origin.z()};
    const G4double d[3] = {direction.x(), direction.y(), direction.z()};
    const G4double lo[3] = {boxMin.x(), boxMin.y(), boxMin.z()};
    const G4double hi[3] = {boxMax.x(), boxMax.y(), boxMax.z()};

    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(d[axis]) < 1.0e-15) {
            if (o[axis] < lo[axis] || o[axis] > hi[axis]) {
                return false;
            }
            continue;
        }

        G4double t1 = (lo[axis] - o[axis]) / d[axis];
        G4double t2 = (hi[axis] - o[axis]) / d[axis];
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMax < tMin) return false;
    }

    return tMax >= 0.0;
}

void SolidWorldBoundingBox(const G4VSolid* solid,
                           const G4RotationMatrix& localToWorldRotation,
                           const G4ThreeVector& localToWorldTranslation,
                           G4ThreeVector& worldMin,
                           G4ThreeVector& worldMax)
{
    G4ThreeVector localMin;
    G4ThreeVector localMax;
    solid->BoundingLimits(localMin, localMax);

    const std::array<G4ThreeVector, 8> corners = {{
        {localMin.x(), localMin.y(), localMin.z()},
        {localMin.x(), localMin.y(), localMax.z()},
        {localMin.x(), localMax.y(), localMin.z()},
        {localMin.x(), localMax.y(), localMax.z()},
        {localMax.x(), localMin.y(), localMin.z()},
        {localMax.x(), localMin.y(), localMax.z()},
        {localMax.x(), localMax.y(), localMin.z()},
        {localMax.x(), localMax.y(), localMax.z()},
    }};

    bool first = true;
    for (const auto& corner : corners) {
        const G4ThreeVector world =
            localToWorldRotation * corner + localToWorldTranslation;

        if (first) {
            worldMin = world;
            worldMax = world;
            first = false;
        } else {
            worldMin.setX(std::min(worldMin.x(), world.x()));
            worldMin.setY(std::min(worldMin.y(), world.y()));
            worldMin.setZ(std::min(worldMin.z(), world.z()));
            worldMax.setX(std::max(worldMax.x(), world.x()));
            worldMax.setY(std::max(worldMax.y(), world.y()));
            worldMax.setZ(std::max(worldMax.z(), world.z()));
        }
    }
}

} // namespace

namespace G4Cosmic {

CORSIKAPrimaryGenerator::CORSIKAPrimaryGenerator()
    : gun_(std::make_unique<G4ParticleGun>(1))
{
    minEnergy_ = 1.0 * TeV;
    maxEnergy_ = 100.0 * TeV;
    monoEnergy_ = 10.0 * TeV;
    minZenith_ = 0.0 * deg;
    maxZenith_ = 60.0 * deg;
    minAzimuth_ = 0.0 * deg;
    maxAzimuth_ = 360.0 * deg;
    ConfigureMessenger();
}

CORSIKAPrimaryGenerator::~CORSIKAPrimaryGenerator() = default;

void CORSIKAPrimaryGenerator::ConfigureMessenger()
{
    messenger_ = std::make_unique<G4GenericMessenger>(
        this, "/g4cosmic/corsika/", "G4Cosmic CORSIKA shower-source controls");

    messenger_->DeclareMethod("generationMode", &CORSIKAPrimaryGenerator::SetGenerationMode,
                              "CORSIKA generation mode: file or batch.");
    messenger_->DeclareMethod("inputMode", &CORSIKAPrimaryGenerator::SetInputMode,
                              "Deprecated alias for generationMode. command maps to batch.");
    messenger_->DeclareMethod("cacheFile", &CORSIKAPrimaryGenerator::SetCacheFile,
                              "G4Cosmic/CORSIKA text .dat shower list to read or generate.");
    messenger_->DeclareMethod("file", &CORSIKAPrimaryGenerator::SetFileName,
                              "Deprecated alias for cacheFile.");
    messenger_->DeclareMethod("command", &CORSIKAPrimaryGenerator::SetCommand,
                              "Legacy single-token external command. Prefer runner and runnerScript for commands with arguments.");
    messenger_->DeclareMethod("runner", &CORSIKAPrimaryGenerator::SetRunner,
                              "Executable used for the CORSIKA runner, for example python or a corsika8 wrapper executable.");
    messenger_->DeclareMethod("runnerScript", &CORSIKAPrimaryGenerator::SetRunnerScript,
                              "Script or application path run by /g4cosmic/corsika/runner.");
    messenger_->DeclareProperty("eventsPerBatch", eventsPerBatch_,
                                "Number of CORSIKA showers to request per generated batch.");
    messenger_->DeclareProperty("eventsPerRun", eventsPerBatch_,
                                "Deprecated alias for eventsPerBatch.");
    messenger_->DeclareProperty("reuseCache", reuseCache_,
                                "Optional reproducibility mode. Default 0 regenerates batch caches at program start; set 1 to reuse an existing cacheFile from a previous run.");
    messenger_->DeclareProperty("regenerate", regenerate_,
                                "When nonzero, rerun the external command whenever the source is reloaded.");

    messenger_->DeclareMethod("primary", &CORSIKAPrimaryGenerator::SetPrimary,
                              "Primary cosmic-ray species forwarded to the CORSIKA 8 runner.");
    messenger_->DeclareMethod("energyMode", &CORSIKAPrimaryGenerator::SetEnergyMode,
                              "Energy mode forwarded to the CORSIKA 8 runner: powerLaw or mono.");
    messenger_->DeclarePropertyWithUnit("minEnergy", "GeV", minEnergy_,
                                        "Minimum primary energy for power-law generation.");
    messenger_->DeclarePropertyWithUnit("maxEnergy", "GeV", maxEnergy_,
                                        "Maximum primary energy for power-law generation.");
    messenger_->DeclarePropertyWithUnit("energy", "GeV", monoEnergy_,
                                        "Primary energy for mono-energetic generation.");
    messenger_->DeclareProperty("spectralIndex", spectralIndex_,
                                "Positive gamma for dN/dE proportional to E^-gamma.");
    messenger_->DeclarePropertyWithUnit("minZenith", "deg", minZenith_,
                                        "Minimum primary zenith angle forwarded to the CORSIKA 8 runner.");
    messenger_->DeclarePropertyWithUnit("maxZenith", "deg", maxZenith_,
                                        "Maximum primary zenith angle forwarded to the CORSIKA 8 runner.");
    messenger_->DeclarePropertyWithUnit("minAzimuth", "deg", minAzimuth_,
                                        "Minimum primary azimuth angle forwarded to the CORSIKA 8 runner.");
    messenger_->DeclarePropertyWithUnit("maxAzimuth", "deg", maxAzimuth_,
                                        "Maximum primary azimuth angle forwarded to the CORSIKA 8 runner.");

    messenger_->DeclareMethod("idScheme", &CORSIKAPrimaryGenerator::SetIdScheme,
                              "Particle ID scheme in the text .dat file: pdg or corsika.");
    messenger_->DeclareMethod("positionUnit", &CORSIKAPrimaryGenerator::SetPositionUnit,
                              "Position unit used by input x/y/z columns: m, cm, or mm.");
    messenger_->DeclareMethod("momentumUnit", &CORSIKAPrimaryGenerator::SetMomentumUnit,
                              "Momentum unit used by input px/py/pz columns: GeV or MeV.");
    messenger_->DeclareMethod("timeUnit", &CORSIKAPrimaryGenerator::SetTimeUnit,
                              "Time unit used by input time column: s, ns, us, or ms.");
    messenger_->DeclareProperty("loop", loop_,
                                "When nonzero, wrap around if /run/beamOn exceeds the loaded CORSIKA shower count.");
    messenger_->DeclareProperty("verbose", verbose_,
                                "Print CORSIKA diagnostics from the master thread only (0/1).");
    messenger_->DeclareMethod("acceptanceMode", &CORSIKAPrimaryGenerator::SetAcceptanceMode,
                              "CORSIKA geometric acceptance: all or volume.");
    messenger_->DeclareMethod("acceptanceVolume", &CORSIKAPrimaryGenerator::SetAcceptanceVolume,
                              "Logical-volume name or wildcard used when acceptanceMode is volume.");
    messenger_->DeclareProperty("maxAcceptanceTrials", maxAcceptanceTrials_,
                                "Maximum loaded/generated showers to test while satisfying volume acceptance.");
    messenger_->DeclareMethod("reload", &CORSIKAPrimaryGenerator::Reload,
                              "Forget cached CORSIKA showers and reload/regenerate on the next event.");
    messenger_->DeclareMethod("apply", &CORSIKAPrimaryGenerator::ApplyConfiguration,
                              "Prepare the CORSIKA source: parse cacheFile or generate/load a batch.");
}

void CORSIKAPrimaryGenerator::ApplyConfiguration()
{
    if (eventsPerBatch_ <= 0) {
        if (IsMasterThread()) {
            G4cout << "G4Cosmic: /g4cosmic/corsika/eventsPerBatch must be > 0." << G4endl;
        }
        return;
    }
    if (maxAcceptanceTrials_ <= 0) {
        if (IsMasterThread()) {
            G4cout << "G4Cosmic: /g4cosmic/corsika/maxAcceptanceTrials must be > 0." << G4endl;
        }
        return;
    }
    if (energyMode_ == "powerLaw" && (minEnergy_ <= 0.0 || maxEnergy_ < minEnergy_)) {
        if (IsMasterThread()) {
            G4cout << "G4Cosmic: CORSIKA minEnergy/maxEnergy are invalid." << G4endl;
        }
        return;
    }
    if (energyMode_ == "mono" && monoEnergy_ <= 0.0) {
        if (IsMasterThread()) {
            G4cout << "G4Cosmic: CORSIKA mono energy must be > 0." << G4endl;
        }
        return;
    }

    Reload();
    LoadIfNeeded();
}

void CORSIKAPrimaryGenerator::SetGenerationMode(const G4String& mode)
{
    if (mode != "file" && mode != "batch" && mode != "command") {
        if (IsMasterThread()) {
            G4cout << "G4Cosmic: /g4cosmic/corsika/generationMode must be 'file' or 'batch'." << G4endl;
        }
        return;
    }

    generationMode_ = (mode == "command") ? "batch" : mode;
    Reload();
}

void CORSIKAPrimaryGenerator::SetInputMode(const G4String& mode)
{
    SetGenerationMode(mode);
}

void CORSIKAPrimaryGenerator::SetCacheFile(const G4String& fileName)
{
    cacheFile_ = fileName;
    Reload();
}

void CORSIKAPrimaryGenerator::SetFileName(const G4String& fileName)
{
    SetCacheFile(fileName);
}

void CORSIKAPrimaryGenerator::SetCommand(const G4String& command)
{
    command_ = command;
    Reload();
}

void CORSIKAPrimaryGenerator::SetRunner(const G4String& executable)
{
    runner_ = executable;
    Reload();
}

void CORSIKAPrimaryGenerator::SetRunnerScript(const G4String& script)
{
    runnerScript_ = script;
    Reload();
}

void CORSIKAPrimaryGenerator::SetPrimary(const G4String& primary)
{
    primary_ = primary;
    Reload();
}

void CORSIKAPrimaryGenerator::SetEnergyMode(const G4String& mode)
{
    if (mode != "powerLaw" && mode != "mono") {
        if (IsMasterThread()) {
            G4cout << "G4Cosmic: /g4cosmic/corsika/energyMode must be 'powerLaw' or 'mono'." << G4endl;
        }
        return;
    }
    energyMode_ = mode;
    Reload();
}

void CORSIKAPrimaryGenerator::SetIdScheme(const G4String& scheme)
{
    if (scheme != "pdg" && scheme != "corsika") {
        if (IsMasterThread()) {
            G4cout << "G4Cosmic: /g4cosmic/corsika/idScheme must be 'pdg' or 'corsika'." << G4endl;
        }
        return;
    }
    idScheme_ = scheme;
    Reload();
}

void CORSIKAPrimaryGenerator::SetPositionUnit(const G4String& unitName)
{
    if (unitName != "m" && unitName != "cm" && unitName != "mm") {
        if (IsMasterThread()) {
            G4cout << "G4Cosmic: /g4cosmic/corsika/positionUnit must be m, cm, or mm." << G4endl;
        }
        return;
    }
    positionUnit_ = unitName;
    Reload();
}

void CORSIKAPrimaryGenerator::SetMomentumUnit(const G4String& unitName)
{
    if (unitName != "GeV" && unitName != "MeV" && unitName != "gev" && unitName != "mev") {
        if (IsMasterThread()) {
            G4cout << "G4Cosmic: /g4cosmic/corsika/momentumUnit must be GeV or MeV." << G4endl;
        }
        return;
    }
    momentumUnit_ = unitName;
    Reload();
}

void CORSIKAPrimaryGenerator::SetTimeUnit(const G4String& unitName)
{
    if (unitName != "s" && unitName != "ns" && unitName != "us" && unitName != "ms") {
        if (IsMasterThread()) {
            G4cout << "G4Cosmic: /g4cosmic/corsika/timeUnit must be s, ns, us, or ms." << G4endl;
        }
        return;
    }
    timeUnit_ = unitName;
    Reload();
}

void CORSIKAPrimaryGenerator::SetAcceptanceMode(const G4String& mode)
{
    if (mode != "all" && mode != "volume") {
        if (IsMasterThread()) {
            G4cout << "G4Cosmic: /g4cosmic/corsika/acceptanceMode must be 'all' or 'volume'." << G4endl;
        }
        return;
    }
    acceptanceMode_ = mode;
}

void CORSIKAPrimaryGenerator::SetAcceptanceVolume(const G4String& logicalVolumeName)
{
    acceptanceVolume_ = logicalVolumeName;
    cachedAcceptanceVolume_ = "";
    acceptanceBoxes_.clear();
}

void CORSIKAPrimaryGenerator::Reload()
{
    loadedCacheFile_ = "";
    loadedGenerationMode_ = "";
    loadedIdScheme_ = "";
    loadedPositionUnit_ = "";
    loadedMomentumUnit_ = "";
    loadedTimeUnit_ = "";
    showers_.clear();
    skippedLines_ = 0;

    if (regenerate_ != 0) {
        generatedBatchCount_ = 0;
    }
}

G4double CORSIKAPrimaryGenerator::PositionScale() const
{
    if (positionUnit_ == "m") return m;
    if (positionUnit_ == "cm") return cm;
    if (positionUnit_ == "mm") return mm;
    throw std::runtime_error("Unsupported /g4cosmic/corsika/positionUnit.");
}

G4double CORSIKAPrimaryGenerator::MomentumScale() const
{
    if (momentumUnit_ == "GeV" || momentumUnit_ == "gev") return GeV;
    if (momentumUnit_ == "MeV" || momentumUnit_ == "mev") return MeV;
    throw std::runtime_error("Unsupported /g4cosmic/corsika/momentumUnit.");
}

G4double CORSIKAPrimaryGenerator::TimeScale() const
{
    if (timeUnit_ == "s") return s;
    if (timeUnit_ == "ns") return ns;
    if (timeUnit_ == "us") return us;
    if (timeUnit_ == "ms") return ms;
    throw std::runtime_error("Unsupported /g4cosmic/corsika/timeUnit.");
}

G4int CORSIKAPrimaryGenerator::ConvertInputId(G4int inputId) const
{
    if (idScheme_ == "pdg") {
        return inputId;
    }

    switch (inputId) {
        case 1:  return 22;    // gamma
        case 2:  return -11;   // e+
        case 3:  return 11;    // e-
        case 5:  return -13;   // mu+
        case 6:  return 13;    // mu-
        case 7:  return 111;   // pi0
        case 8:  return 211;   // pi+
        case 9:  return -211;  // pi-
        case 10: return 130;   // K0L
        case 11: return 321;   // K+
        case 12: return -321;  // K-
        case 13: return 2112;  // neutron
        case 14: return 2212;  // proton
        case 15: return -2212; // anti-proton
        case 16: return 310;   // K0S
        case 25: return -2112; // anti-neutron
        default: return 0;
    }
}

std::string CORSIKAPrimaryGenerator::BuildDefaultRunnerCommand() const
{
    if (runner_.empty()) {
        throw std::runtime_error("/g4cosmic/corsika/runner cannot be empty.");
    }
    if (runnerScript_.empty()) {
        throw std::runtime_error("/g4cosmic/corsika/runnerScript cannot be empty.");
    }

    std::ostringstream command;
    command << QuoteArgument(std::string(runner_))
            << ' ' << QuoteArgument(std::string(runnerScript_))
            << " --events " << eventsPerBatch_
            << " --output " << QuoteArgument(std::string(cacheFile_))
            << " --primary " << QuoteArgument(std::string(primary_))
            << " --energy-mode " << QuoteArgument(std::string(energyMode_))
            << " --min-energy-gev " << ToString(minEnergy_ / GeV)
            << " --max-energy-gev " << ToString(maxEnergy_ / GeV)
            << " --energy-gev " << ToString(monoEnergy_ / GeV)
            << " --spectral-index " << ToString(spectralIndex_)
            << " --min-zenith-deg " << ToString(minZenith_ / deg)
            << " --max-zenith-deg " << ToString(maxZenith_ / deg)
            << " --min-azimuth-deg " << ToString(minAzimuth_ / deg)
            << " --max-azimuth-deg " << ToString(maxAzimuth_ / deg);

    return command.str();
}

std::string CORSIKAPrimaryGenerator::ExpandedCommand() const
{
    std::string command = command_.empty() ? BuildDefaultRunnerCommand() : std::string(command_);
    ReplaceAll(command, "__output__", std::string(cacheFile_));
    ReplaceAll(command, "__cache__", std::string(cacheFile_));
    ReplaceAll(command, "__events__", std::to_string(eventsPerBatch_));
    ReplaceAll(command, "__batch__", std::to_string(generatedBatchCount_));
    ReplaceAll(command, "__primary__", std::string(primary_));
    ReplaceAll(command, "__energy_mode__", std::string(energyMode_));
    ReplaceAll(command, "__min_energy_gev__", ToString(minEnergy_ / GeV));
    ReplaceAll(command, "__max_energy_gev__", ToString(maxEnergy_ / GeV));
    ReplaceAll(command, "__energy_gev__", ToString(monoEnergy_ / GeV));
    ReplaceAll(command, "__spectral_index__", ToString(spectralIndex_));
    ReplaceAll(command, "__min_zenith_deg__", ToString(minZenith_ / deg));
    ReplaceAll(command, "__max_zenith_deg__", ToString(maxZenith_ / deg));
    ReplaceAll(command, "__min_azimuth_deg__", ToString(minAzimuth_ / deg));
    ReplaceAll(command, "__max_azimuth_deg__", ToString(maxAzimuth_ / deg));
    return command;
}

void CORSIKAPrimaryGenerator::GenerateBatchFile() const
{
    if (command_.empty() && (runner_.empty() || runnerScript_.empty())) {
        throw std::runtime_error(
            "/g4cosmic/corsika/generationMode " + std::string(generationMode_) +
            " requires either runner/runnerScript or a legacy command.");
    }
    if (cacheFile_.empty()) {
        throw std::runtime_error(
            "/g4cosmic/corsika/generationMode " + std::string(generationMode_) +
            " requires /g4cosmic/corsika/cacheFile <output.dat>.");
    }

    const std::string command = ExpandedCommand();

    const bool generatedThisProcess =
        gGeneratedBatchCommand == command &&
        gGeneratedBatchCacheFile == cacheFile_ &&
        gGeneratedBatchEventsPerBatch == eventsPerBatch_ &&
        gGeneratedBatchCount > 0 &&
        FileExists(cacheFile_);

    // Default batch behavior is to regenerate a fresh cache at the start of a
    // program run.  After one worker has produced that fresh cache, the other
    // workers must read it rather than launching duplicate runner processes.
    if (generationMode_ == "batch" && generatedThisProcess) {
        return;
    }

    // Optional reproducibility mode: allow a cache created by an earlier run to
    // be reused instead of regenerated.  Normal demos leave reuseCache at 0.
    if (generationMode_ == "batch" && regenerate_ == 0 && reuseCache_ != 0 && FileExists(cacheFile_)) {
        gGeneratedBatchCommand = command;
        gGeneratedBatchCacheFile = cacheFile_;
        gGeneratedBatchEventsPerBatch = eventsPerBatch_;
        if (gGeneratedBatchCount == 0) {
            ++gGeneratedBatchCount;
        }
        return;
    }

    if (command == "python" || command == "python3" || command == "py") {
        throw std::runtime_error(
            "Refusing to launch an interactive Python prompt for CORSIKA generation. "
            "Set /g4cosmic/corsika/runnerScript or use the default macros.");
    }
    if (IsMasterThread()) {
        G4cout << "G4Cosmic: generating CORSIKA batch " << generatedBatchCount_
               << " with external command:" << G4endl
               << "  " << command << G4endl;
    }

    const int status = std::system(command.c_str());
    if (status != 0) {
        throw std::runtime_error(
            "CORSIKA external command failed with status " +
            std::to_string(status) + ": " + command);
    }

    cachedCommand_ = command;
    cachedCacheFile_ = cacheFile_;
    cachedEventsPerBatch_ = eventsPerBatch_;
    ++generatedBatchCount_;

    gGeneratedBatchCommand = command;
    gGeneratedBatchCacheFile = cacheFile_;
    gGeneratedBatchEventsPerBatch = eventsPerBatch_;
    ++gGeneratedBatchCount;
}

std::vector<CORSIKAPrimaryGenerator::ShowerRecord>
CORSIKAPrimaryGenerator::ReadShowerFile(const G4String& fileName, G4int& skippedLines) const
{
    if (fileName.empty()) {
        throw std::runtime_error(
            "/g4cosmic/source corsika requires /g4cosmic/corsika/cacheFile <path>.");
    }

    std::ifstream input(fileName);
    if (!input) {
        throw std::runtime_error(
            "Unable to open CORSIKA .dat shower list: " + std::string(fileName));
    }

    std::vector<ShowerRecord> parsedShowers;
    skippedLines = 0;

    const G4double xScale = PositionScale();
    const G4double pScale = MomentumScale();
    const G4double tScale = TimeScale();

    G4int currentEventId = std::numeric_limits<G4int>::min();
    ShowerRecord currentShower;
    std::string line;
    G4int lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;
        const auto data = TrimComment(line);
        if (IsBlank(data)) {
            continue;
        }

        G4int eventId = 0;
        G4int inputId = 0;
        G4double x = 0.0;
        G4double y = 0.0;
        G4double z = 0.0;
        G4double px = 0.0;
        G4double py = 0.0;
        G4double pz = 0.0;
        G4double time = 0.0;

        std::istringstream row(data);
        if (!(row >> eventId >> inputId >> x >> y >> z >> px >> py >> pz >> time)) {
            ++skippedLines;
            if (ShouldPrintVerbose(verbose_)) {
                G4cout << "G4Cosmic: skipped malformed CORSIKA .dat line "
                       << lineNumber << "." << G4endl;
            }
            continue;
        }

        if (currentEventId == std::numeric_limits<G4int>::min()) {
            currentEventId = eventId;
            currentShower.inputEventId = eventId;
        }

        if (eventId != currentEventId) {
            if (!currentShower.particles.empty()) {
                parsedShowers.push_back(currentShower);
            }
            currentShower = ShowerRecord{};
            currentShower.inputEventId = eventId;
            currentEventId = eventId;
        }

        ParticleRecord particle;
        particle.inputId = inputId;
        particle.pdg = ConvertInputId(inputId);
        particle.x = x * xScale;
        particle.y = y * xScale;
        particle.z = z * xScale;
        particle.px = px * pScale;
        particle.py = py * pScale;
        particle.pz = pz * pScale;
        particle.time = time * tScale;

        if (particle.pdg == 0) {
            ++skippedLines;
            if (ShouldPrintVerbose(verbose_)) {
                G4cout << "G4Cosmic: skipped unsupported CORSIKA id "
                       << inputId << " on line " << lineNumber << "." << G4endl;
            }
            continue;
        }

        currentShower.particles.push_back(particle);
    }

    if (!currentShower.particles.empty()) {
        parsedShowers.push_back(currentShower);
    }

    if (parsedShowers.empty()) {
        throw std::runtime_error(
            "CORSIKA .dat shower list contained no usable showers: " +
            std::string(fileName));
    }

    return parsedShowers;
}

void CORSIKAPrimaryGenerator::ReplaceLoadedShowers(
    std::vector<ShowerRecord>&& showers, G4int skippedLines) const
{
    showers_ = std::move(showers);
    skippedLines_ = skippedLines;
    loadedCacheFile_ = cacheFile_;
    loadedGenerationMode_ = generationMode_;
    loadedIdScheme_ = idScheme_;
    loadedPositionUnit_ = positionUnit_;
    loadedMomentumUnit_ = momentumUnit_;
    loadedTimeUnit_ = timeUnit_;

    if (IsMasterThread()) {
        G4cout << "G4Cosmic: loaded CORSIKA .dat source '" << cacheFile_
               << "' with " << showers_.size() << " shower(s)"
               << " using generationMode '" << generationMode_ << "'.";
        if (skippedLines_ > 0) {
            G4cout << " Skipped " << skippedLines_ << " line(s).";
        }
        G4cout << G4endl;
    }
}

void CORSIKAPrimaryGenerator::LoadIfNeeded() const
{
#ifdef G4MULTITHREADED
    // Each worker owns its own PrimaryGeneratorAction, so without a process-wide
    // lock all workers can decide the CORSIKA cache is missing and launch the
    // external runner at the same time.  Serialize cache generation and parsing.
    G4AutoLock lock(&gCorsikaIoMutex);
#endif

    if (!showers_.empty() &&
        loadedCacheFile_ == cacheFile_ &&
        loadedGenerationMode_ == generationMode_ &&
        loadedIdScheme_ == idScheme_ &&
        loadedPositionUnit_ == positionUnit_ &&
        loadedMomentumUnit_ == momentumUnit_ &&
        loadedTimeUnit_ == timeUnit_) {
        return;
    }

    if (generationMode_ == "file") {
        G4int skipped = 0;
        ReplaceLoadedShowers(ReadShowerFile(cacheFile_, skipped), skipped);
        return;
    }

    if (reuseCache_ != 0 && regenerate_ == 0 && FileExists(cacheFile_)) {
        G4int skipped = 0;
        ReplaceLoadedShowers(ReadShowerFile(cacheFile_, skipped), skipped);
        return;
    }

    GenerateBatchFile();
    G4int skipped = 0;
    ReplaceLoadedShowers(ReadShowerFile(cacheFile_, skipped), skipped);
}

void CORSIKAPrimaryGenerator::RefreshAcceptanceVolumes() const
{
    if (acceptanceMode_ != "volume") {
        return;
    }

    if (acceptanceVolume_.empty()) {
        throw std::runtime_error(
            "CORSIKA acceptanceMode is 'volume', but no /g4cosmic/corsika/acceptanceVolume was provided.");
    }

    if (cachedAcceptanceVolume_ == acceptanceVolume_ && !acceptanceBoxes_.empty()) {
        return;
    }

    acceptanceBoxes_.clear();
    cachedAcceptanceVolume_ = acceptanceVolume_;

    auto* navigator =
        G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking();
    auto* world = navigator ? navigator->GetWorldVolume() : nullptr;

    if (world == nullptr) {
        throw std::runtime_error(
            "Unable to resolve Geant4 world volume for CORSIKA acceptance lookup. "
            "Make sure /run/initialize has been called before /run/beamOn.");
    }

    const std::string pattern = acceptanceVolume_;

    std::function<void(const G4VPhysicalVolume*,
                       const G4RotationMatrix&,
                       const G4ThreeVector&)> visit;

    visit = [&](const G4VPhysicalVolume* physicalVolume,
                const G4RotationMatrix& localToWorldRotation,
                const G4ThreeVector& localToWorldTranslation) {
        if (physicalVolume == nullptr) {
            return;
        }

        const auto* logicalVolume = physicalVolume->GetLogicalVolume();
        if (logicalVolume == nullptr) {
            return;
        }

        if (WildcardMatches(pattern, logicalVolume->GetName())) {
            const auto* solid = logicalVolume->GetSolid();
            if (solid != nullptr) {
                AcceptanceBox box;
                box.physicalVolumeName = physicalVolume->GetName();
                box.logicalVolumeName = logicalVolume->GetName();
                SolidWorldBoundingBox(
                    solid,
                    localToWorldRotation,
                    localToWorldTranslation,
                    box.min,
                    box.max);
                acceptanceBoxes_.push_back(box);
            }
        }

        const auto daughterCount = logicalVolume->GetNoDaughters();
        for (G4int i = 0; i < daughterCount; ++i) {
            const auto* daughter = logicalVolume->GetDaughter(i);
            if (daughter == nullptr) {
                continue;
            }

            G4RotationMatrix daughterRotation = localToWorldRotation;
            if (const auto* objectRotation = daughter->GetObjectRotation()) {
                daughterRotation *= *objectRotation;
            }

            const G4ThreeVector daughterTranslation =
                localToWorldRotation * daughter->GetObjectTranslation() +
                localToWorldTranslation;

            visit(daughter, daughterRotation, daughterTranslation);
        }
    };

    visit(world, G4RotationMatrix(), G4ThreeVector());

    if (acceptanceBoxes_.empty()) {
        throw std::runtime_error(
            "No physical placements were found for CORSIKA acceptance logical volume pattern '" +
            std::string(acceptanceVolume_) +
            "'. Use an existing G4LogicalVolume name or a wildcard such as 'ScintillatorBarLV'.");
    }

    if (IsMasterThread()) {
        G4cout << "G4Cosmic: CORSIKA acceptance uses "
               << acceptanceBoxes_.size()
               << " physical placement(s) matching logical volume pattern '"
               << acceptanceVolume_ << "'." << G4endl;
    }
}

bool CORSIKAPrimaryGenerator::AcceptPrimary(
    const G4ThreeVector& position, const G4ThreeVector& direction) const
{
    if (acceptanceMode_ == "all") {
        return true;
    }

    RefreshAcceptanceVolumes();

    for (const auto& box : acceptanceBoxes_) {
        if (RayIntersectsBox(position, direction, box.min, box.max)) {
            return true;
        }
    }

    return false;
}

bool CORSIKAPrimaryGenerator::AcceptShower(const ShowerRecord& shower) const
{
    if (acceptanceMode_ == "all") {
        return true;
    }

    auto* particleTable = G4ParticleTable::GetParticleTable();

    for (const auto& particle : shower.particles) {
        auto* definition = particleTable->FindParticle(particle.pdg);
        if (definition == nullptr) {
            continue;
        }

        const G4ThreeVector momentum(particle.px, particle.py, particle.pz);
        if (momentum.mag2() <= 0.0) {
            continue;
        }

        if (AcceptPrimary(G4ThreeVector(particle.x, particle.y, particle.z),
                          momentum.unit())) {
            return true;
        }
    }

    return false;
}

const CORSIKAPrimaryGenerator::ShowerRecord&
CORSIKAPrimaryGenerator::SelectShower(G4int eventId) const
{
    if (eventId < 0) {
        throw std::runtime_error("CORSIKA generator received an event with a negative event id.");
    }
    if (showers_.empty()) {
        throw std::runtime_error("CORSIKA generator has no loaded showers.");
    }

    std::size_t startIndex = static_cast<std::size_t>(eventId);
    if (startIndex >= showers_.size()) {
        if (loop_ == 0) {
            throw std::runtime_error(
                "CORSIKA source ran out of showers. Enable /g4cosmic/corsika/loop 1, increase eventsPerBatch, or reduce /run/beamOn.");
        }
        startIndex %= showers_.size();
    }

    if (acceptanceMode_ == "all") {
        return showers_.at(startIndex);
    }

    RefreshAcceptanceVolumes();

    for (G4int trial = 0; trial < maxAcceptanceTrials_; ++trial) {
        const std::size_t absoluteIndex = startIndex + static_cast<std::size_t>(trial);
        std::size_t index = absoluteIndex;
        if (index >= showers_.size()) {
            if (loop_ == 0) {
                break;
            }
            index %= showers_.size();
        }

        const auto& shower = showers_.at(index);
        if (AcceptShower(shower)) {
            if (ShouldPrintVerbose(verbose_)) {
                G4cout << "G4Cosmic: CORSIKA event " << eventId
                       << " accepted shower index " << index
                       << " after " << (trial + 1) << " trial(s)." << G4endl;
            }
            return shower;
        }
    }

    throw std::runtime_error(
        "CORSIKA acceptance failed after " + std::to_string(maxAcceptanceTrials_) +
        " loaded/generated shower trial(s) for logical-volume pattern '" +
        std::string(acceptanceVolume_) +
        "'. Generate more showers, increase /g4cosmic/corsika/maxAcceptanceTrials, or choose a larger acceptance volume.");
}


void CORSIKAPrimaryGenerator::AppendMetadata(JsonWriter& json) const
{
    json.Write("type", "corsika");
    json.Write("generation_mode", std::string(generationMode_));
    json.Write("cache_file", std::string(cacheFile_));
    json.Write("cache_file_sha256", RunMetadata::Sha256File(std::string(cacheFile_)));
    json.Write("reuse_cache", reuseCache_ != 0);
    json.Write("regenerate", regenerate_ != 0);
    json.Write("events_per_batch", eventsPerBatch_);
    json.Write("runner", std::string(runner_));
    json.Write("runner_script", std::string(runnerScript_));
    if (!command_.empty()) {
        json.Write("legacy_command", std::string(command_));
    }
    json.Write("expanded_command", ExpandedCommand());

    json.Write("primary", std::string(primary_));
    json.Write("energy_mode", std::string(energyMode_));
    json.Write("min_energy_GeV", minEnergy_ / GeV);
    json.Write("max_energy_GeV", maxEnergy_ / GeV);
    json.Write("energy_GeV", monoEnergy_ / GeV);
    json.Write("spectral_index", spectralIndex_);
    json.Write("min_zenith_deg", minZenith_ / deg);
    json.Write("max_zenith_deg", maxZenith_ / deg);
    json.Write("min_azimuth_deg", minAzimuth_ / deg);
    json.Write("max_azimuth_deg", maxAzimuth_ / deg);

    json.Write("id_scheme", std::string(idScheme_));
    json.Write("position_unit", std::string(positionUnit_));
    json.Write("momentum_unit", std::string(momentumUnit_));
    json.Write("time_unit", std::string(timeUnit_));
    json.Write("loop", loop_ != 0);
    json.Write("acceptance_mode", std::string(acceptanceMode_));
    if (!acceptanceVolume_.empty()) {
        json.Write("acceptance_volume", std::string(acceptanceVolume_));
    }
    json.Write("max_acceptance_trials", maxAcceptanceTrials_);
    json.Write("loaded_showers", static_cast<int>(showers_.size()));
    json.Write("skipped_lines", skippedLines_);
    json.Write("cache_generated_this_process", gGeneratedBatchCount > 0);
    json.Write("generated_batch_count", gGeneratedBatchCount);
    if (!gGeneratedBatchCommand.empty()) {
        json.Write("last_generated_command", std::string(gGeneratedBatchCommand));
        json.Write("last_generated_cache_file", std::string(gGeneratedBatchCacheFile));
    }
}

void CORSIKAPrimaryGenerator::GeneratePrimaries(G4Event* event, RunAction* runAction)
{
    LoadIfNeeded();

    const auto eventId = event->GetEventID();
    const auto& shower = SelectShower(eventId);

    auto* particleTable = G4ParticleTable::GetParticleTable();
    std::size_t accepted = 0;

    for (const auto& particle : shower.particles) {
        auto* definition = particleTable->FindParticle(particle.pdg);
        if (definition == nullptr) {
            if (ShouldPrintVerbose(verbose_)) {
                G4cout << "G4Cosmic: skipping CORSIKA particle with PDG "
                       << particle.pdg << " because Geant4 does not know it." << G4endl;
            }
            continue;
        }

        const G4ThreeVector momentum(particle.px, particle.py, particle.pz);
        if (momentum.mag2() <= 0.0) {
            if (ShouldPrintVerbose(verbose_)) {
                G4cout << "G4Cosmic: skipping CORSIKA particle with zero momentum." << G4endl;
            }
            continue;
        }

        const G4double p = momentum.mag();
        const G4double mass = definition->GetPDGMass();
        const G4double totalEnergy = std::sqrt(p * p + mass * mass);
        const G4double kineticEnergy = totalEnergy - mass;
        const G4ThreeVector position(particle.x, particle.y, particle.z);
        const G4ThreeVector direction = momentum.unit();

        gun_->SetParticleDefinition(definition);
        gun_->SetParticleEnergy(kineticEnergy);
        gun_->SetParticlePosition(position);
        gun_->SetParticleMomentumDirection(direction);
        gun_->SetParticleTime(particle.time);

        if (runAction != nullptr) {
            PrimaryRecord primary;
            primary.eventID = eventId;
            primary.primaryIndex = static_cast<int>(accepted);
            primary.pdg = particle.pdg;
            primary.particleName = definition->GetParticleName();
            primary.kineticEnergy = kineticEnergy;
            primary.time = particle.time;
            primary.position = position;
            primary.direction = direction;
            runAction->WritePrimary(primary);
        }

        gun_->GeneratePrimaryVertex(event);
        ++accepted;
    }

    if (accepted == 0) {
        throw std::runtime_error(
            "CORSIKA source selected a shower with no particles usable by Geant4.");
    }

    if (ShouldPrintVerbose(verbose_)) {
        G4cout << "G4Cosmic: CORSIKA event " << eventId
               << " used input shower " << shower.inputEventId
               << " and generated " << accepted
               << " Geant4 primary particle(s)." << G4endl;
    }
}

} // namespace G4Cosmic
