#include "PrimaryGeneratorAction.hh"

#include "PrimaryRecord.hh"
#include "RunAction.hh"

#include "CRYGenerator.h"
#include "CRYParticle.h"
#include "CRYSetup.h"

#include "G4Event.hh"
#include "G4GenericMessenger.hh"
#include "G4LogicalVolume.hh"
#include "G4RotationMatrix.hh"
#include "G4TransportationManager.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VSolid.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4ios.hh"
#include "Randomize.hh"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <array>
#include <functional>
#include <vector>

#ifndef G4COSMIC_CRY_DATA_DIR
#error "G4COSMIC_CRY_DATA_DIR must be defined by CMake"
#endif

#ifndef G4COSMIC_CRY_SETUP_FILE
#error "G4COSMIC_CRY_SETUP_FILE must be defined by CMake"
#endif

namespace {

double CryRandom()
{
    return G4UniformRand();
}

std::string ReadTextFile(const std::string& path)
{
    std::ifstream input(path);

    if (!input) {
        throw std::runtime_error(
            "Unable to open CRY setup file: " + path);
    }

    std::ostringstream buffer;
    std::string line;

    while (std::getline(input, line)) {
        const auto first =
            line.find_first_not_of(" \t\r\n");

        // Ignore blank lines and comments.
        if (first == std::string::npos ||
            line[first] == '#') {
            continue;
        }

        // CRYSetup tokenizes using a literal space,
        // so join configuration lines using spaces.
        buffer << line << ' ';
    }

    return buffer.str();
}

bool RayIntersectsBox(const G4ThreeVector& origin,
                      const G4ThreeVector& direction,
                      const G4ThreeVector& boxMin,
                      const G4ThreeVector& boxMax)
{
    // Standard ray/AABB slab test.  t >= 0 restricts the test to the
    // forward-going part of the generated primary trajectory.
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

bool WildcardMatches(const std::string& pattern, const std::string& text)
{
    // Simple glob matcher for logical-volume names.  '*' matches any number
    // of characters and '?' matches one character.  If the pattern contains
    // no wildcard, this is an exact match.
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


PrimaryGeneratorAction::PrimaryGeneratorAction(RunAction* runAction)
    : runAction_(runAction),
      gun_(std::make_unique<G4ParticleGun>(1)),
      sourceMode_("cry"),
      generationZ_(1.90 * m)
{
    gun_->SetParticleDefinition(
        G4ParticleTable::GetParticleTable()->FindParticle("mu-"));
    gun_->SetParticleEnergy(4.0 * GeV);
    gun_->SetParticlePosition(G4ThreeVector(0.0, 0.0, 1.65 * m));
    gun_->SetParticleMomentumDirection(
        G4ThreeVector(0.04, -0.03, -1.0).unit());

    if (const char* mode = std::getenv("G4COSMIC_SOURCE")) {
        sourceMode_ = mode;
    }

    // Store angular configuration in Geant4 internal units.
    sampleMaxTheta_ = 30.0 * deg;

    ConfigureMessenger();

    // Do not initialize CRY here. Batch macros are executed after this
    // action is constructed, so eagerly initializing CRY makes gun-only
    // validation runs look like CRY runs. CRY is initialized lazily when
    // it is actually selected/used.
    if (sourceMode_ != "cry" && sourceMode_ != "gun" && sourceMode_ != "sample") {
        throw std::runtime_error(
            "Unknown G4COSMIC_SOURCE='" + sourceMode_ +
            "'. Use 'cry', 'gun', or 'sample'.");
    }
}


PrimaryGeneratorAction::~PrimaryGeneratorAction() =
    default;


void PrimaryGeneratorAction::ConfigureMessenger()
{
    primaryMessenger_ = std::make_unique<G4GenericMessenger>(
        this, "/g4cosmic/", "G4Cosmic primary-source controls");
    primaryMessenger_->DeclareMethod(
        "source", &PrimaryGeneratorAction::SetSource,
        "Primary source: cry, gun, or sample.");

    cryMessenger_ = std::make_unique<G4GenericMessenger>(
        this, "/g4cosmic/cry/", "G4Cosmic CRY controls");
    cryMessenger_->DeclareProperty("returnNeutrons", returnNeutrons_, "Return neutrons (0/1).");
    cryMessenger_->DeclareProperty("returnProtons", returnProtons_, "Return protons (0/1).");
    cryMessenger_->DeclareProperty("returnGammas", returnGammas_, "Return gammas (0/1).");
    cryMessenger_->DeclareProperty("returnElectrons", returnElectrons_, "Return electrons/positrons (0/1).");
    cryMessenger_->DeclareProperty("returnMuons", returnMuons_, "Return muons (0/1).");
    cryMessenger_->DeclareProperty("returnPions", returnPions_, "Return pions (0/1).");
    cryMessenger_->DeclareProperty("returnKaons", returnKaons_, "Return kaons (0/1).");
    cryMessenger_->DeclareProperty("subboxLength", subboxLength_, "Sampling-square side length in metres.");
    cryMessenger_->DeclareProperty("altitude", altitude_, "Altitude in metres (available CRY datasets only).");
    cryMessenger_->DeclareProperty("latitude", latitude_, "Geomagnetic latitude in degrees.");
    cryMessenger_->DeclareMethod("date", &PrimaryGeneratorAction::SetCRYDate,
                                 "Date in month-day-year form, e.g. 9-18-2026.");
    cryMessenger_->DeclareProperty("nParticlesMin", nParticlesMin_, "Minimum particles returned per event.");
    cryMessenger_->DeclareProperty("nParticlesMax", nParticlesMax_, "Maximum particles returned per event.");
    cryMessenger_->DeclareProperty("xoffset", xoffset_, "x offset in metres.");
    cryMessenger_->DeclareProperty("yoffset", yoffset_, "y offset in metres.");
    cryMessenger_->DeclareProperty("zoffset", zoffset_, "z offset in metres.");
    cryMessenger_->DeclareProperty("verbose", cryVerbose_, "Per-particle diagnostics (0/1).");
    cryMessenger_->DeclareMethod("acceptanceMode", &PrimaryGeneratorAction::SetCRYAcceptanceMode,
                                 "CRY geometric acceptance: all or volume.");
    cryMessenger_->DeclareMethod("acceptanceVolume", &PrimaryGeneratorAction::SetCRYAcceptanceVolume,
                                 "Logical-volume name or wildcard used when acceptanceMode is volume.");
    cryMessenger_->DeclareProperty("maxAcceptanceTrials", cryMaxAcceptanceTrials_,
                                   "Maximum CRY showers to redraw while satisfying volume acceptance.");
    cryMessenger_->DeclareMethod("apply", &PrimaryGeneratorAction::ApplyCRYConfiguration,
                                 "Rebuild CRY using the current settings.");

    sampleMessenger_ = std::make_unique<G4GenericMessenger>(
        this, "/g4cosmic/sample/", "Randomized single-primary ML sample controls");
    sampleMessenger_->DeclareMethod("particle", &PrimaryGeneratorAction::SetSampleParticle,
                                    "Particle name: mu-, mu+, proton, neutron, etc.");
    sampleMessenger_->DeclarePropertyWithUnit("minEnergy", "MeV", sampleMinEnergy_,
                                               "Minimum kinetic energy.");
    sampleMessenger_->DeclarePropertyWithUnit("maxEnergy", "MeV", sampleMaxEnergy_,
                                               "Maximum kinetic energy.");
    sampleMessenger_->DeclarePropertyWithUnit("halfWidthX", "mm", sampleHalfWidthX_,
                                               "Uniform source half-width in x.");
    sampleMessenger_->DeclarePropertyWithUnit("halfWidthY", "mm", sampleHalfWidthY_,
                                               "Uniform source half-width in y.");
    sampleMessenger_->DeclarePropertyWithUnit("maxTheta", "deg", sampleMaxTheta_,
                                               "Maximum zenith angle from vertically downward.");
    sampleMessenger_->DeclareProperty("logEnergy", sampleLogEnergy_,
                                      "Sample kinetic energy log-uniformly (1) or uniformly (0).");
}


void PrimaryGeneratorAction::SetSource(const G4String& source)
{
    if (source != "cry" && source != "gun" && source != "sample") {
        G4cout << "G4Cosmic: source must be 'cry', 'gun', or 'sample'." << G4endl;
        return;
    }
    sourceMode_ = source;
    if (sourceMode_ == "cry" && !cryGenerator_) {
        InitializeCRY();
    }
}


void PrimaryGeneratorAction::SetSampleParticle(const G4String& particle)
{
    auto* definition = G4ParticleTable::GetParticleTable()->FindParticle(particle);
    if (!definition) {
        G4cout << "G4Cosmic: unknown sample particle '" << particle << "'." << G4endl;
        return;
    }
    sampleParticle_ = particle;
}


void PrimaryGeneratorAction::SetCRYDate(const G4String& date)
{
    date_ = date;
}


void PrimaryGeneratorAction::SetCRYAcceptanceMode(const G4String& mode)
{
    if (mode != "all" && mode != "volume") {
        G4cout << "G4Cosmic: cry/acceptanceMode must be 'all' or 'volume'."
               << G4endl;
        return;
    }
    cryAcceptanceMode_ = mode;
}


void PrimaryGeneratorAction::SetCRYAcceptanceVolume(
    const G4String& logicalVolumeName)
{
    cryAcceptanceVolume_ = logicalVolumeName;
    cachedAcceptanceVolume_ = "";
    acceptanceBoxes_.clear();
}


void PrimaryGeneratorAction::RefreshCRYAcceptanceVolumes() const
{
    if (cryAcceptanceMode_ != "volume") {
        return;
    }

    if (cryAcceptanceVolume_.empty()) {
        throw std::runtime_error(
            "CRY acceptanceMode is 'volume', but no /g4cosmic/cry/acceptanceVolume was provided.");
    }

    if (cachedAcceptanceVolume_ == cryAcceptanceVolume_ &&
        !acceptanceBoxes_.empty()) {
        return;
    }

    acceptanceBoxes_.clear();
    cachedAcceptanceVolume_ = cryAcceptanceVolume_;

    auto* navigator =
        G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking();
    auto* world = navigator ? navigator->GetWorldVolume() : nullptr;

    if (world == nullptr) {
        throw std::runtime_error(
            "Unable to resolve Geant4 world volume for CRY acceptance lookup. "
            "Make sure /run/initialize has been called before /run/beamOn.");
    }

    const std::string pattern = cryAcceptanceVolume_;

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
            "No physical placements were found for CRY acceptance logical volume pattern '" +
            std::string(cryAcceptanceVolume_) +
            "'. Use an existing G4LogicalVolume name, or a wildcard such as '*ScintillatorLV_*'.");
    }

    G4cout << "G4Cosmic: CRY acceptance uses "
           << acceptanceBoxes_.size()
           << " physical placement(s) matching logical volume pattern '"
           << cryAcceptanceVolume_ << "'." << G4endl;
}


bool PrimaryGeneratorAction::AcceptCRYPrimary(
    const G4ThreeVector& position, const G4ThreeVector& direction) const
{
    if (cryAcceptanceMode_ == "all") {
        return true;
    }

    RefreshCRYAcceptanceVolumes();

    for (const auto& box : acceptanceBoxes_) {
        if (RayIntersectsBox(position, direction, box.min, box.max)) {
            return true;
        }
    }

    return false;
}


std::string PrimaryGeneratorAction::BuildCRYSetupText() const
{
    std::ostringstream out;
    out << "returnNeutrons " << returnNeutrons_ << ' '
        << "returnProtons " << returnProtons_ << ' '
        << "returnGammas " << returnGammas_ << ' '
        << "returnElectrons " << returnElectrons_ << ' '
        << "returnMuons " << returnMuons_ << ' '
        << "returnPions " << returnPions_ << ' '
        << "returnKaons " << returnKaons_ << ' '
        << "date " << date_ << ' '
        << "latitude " << latitude_ << ' '
        << "altitude " << altitude_ << ' '
        << "subboxLength " << subboxLength_ << ' '
        << "nParticlesMin " << nParticlesMin_ << ' '
        << "nParticlesMax " << nParticlesMax_ << ' '
        << "xoffset " << xoffset_ << ' '
        << "yoffset " << yoffset_ << ' '
        << "zoffset " << zoffset_ << ' ';
    return out.str();
}


void PrimaryGeneratorAction::ApplyCRYConfiguration()
{
    if (subboxLength_ <= 0.0) {
        G4cout << "G4Cosmic: cry/subboxLength must be > 0." << G4endl;
        return;
    }
    if (nParticlesMin_ < 0 || nParticlesMax_ < nParticlesMin_) {
        G4cout << "G4Cosmic: invalid CRY particle-count limits." << G4endl;
        return;
    }
    if (cryMaxAcceptanceTrials_ <= 0) {
        G4cout << "G4Cosmic: cry/maxAcceptanceTrials must be > 0." << G4endl;
        return;
    }
    cachedAcceptanceVolume_ = "";
    acceptanceBoxes_.clear();
    InitializeCRY();
}


void PrimaryGeneratorAction::InitializeCRY()
{
    // The macro-facing settings are authoritative.  cry_setup.txt remains a
    // readable record of the default configuration, while macros can override
    // the same values without editing files or starting the visualizer.
    const std::string setupText = BuildCRYSetupText();

    cryGenerator_.reset();
    crySetup_.reset();

    crySetup_ = std::make_unique<CRYSetup>(
        setupText, std::string(G4COSMIC_CRY_DATA_DIR));
    crySetup_->setRandomFunction(&CryRandom);
    cryGenerator_ = std::make_unique<CRYGenerator>(crySetup_.get());

    G4cout << G4endl
           << "========================================" << G4endl
           << " G4Cosmic CRY source initialized" << G4endl
           << " CRY data: " << G4COSMIC_CRY_DATA_DIR << G4endl
           << " date: " << date_ << G4endl
           << " latitude: " << latitude_ << G4endl
           << " altitude: " << altitude_ << " m" << G4endl
           << " subboxLength: " << subboxLength_ << " m" << G4endl
           << " particle range: " << nParticlesMin_ << ".." << nParticlesMax_ << G4endl
           << " Generation Z: " << generationZ_ / m << " m" << G4endl
           << " acceptanceMode: " << cryAcceptanceMode_ << G4endl
           << " acceptanceVolume: "
           << (cryAcceptanceVolume_.empty() ? G4String("<unset>") : cryAcceptanceVolume_)
           << G4endl
           << " maxAcceptanceTrials: " << cryMaxAcceptanceTrials_ << G4endl
           << "========================================" << G4endl;
}


void PrimaryGeneratorAction::GeneratePrimaries(
    G4Event* event)
{
    if (sourceMode_ == "gun") {
        GenerateGunEvent(event);
    }
    else if (sourceMode_ == "sample") {
        GenerateSampleEvent(event);
    }
    else {
        GenerateCRYEvent(event);
    }
}


void PrimaryGeneratorAction::GenerateGunEvent(
    G4Event* event)
{
    // Record exactly what is injected into Geant4 so the deterministic
    // debugging gun has the same ROOT truth information as CRY events.
    if (runAction_) {
        PrimaryRecord primary;
        primary.eventID = event->GetEventID();
        primary.primaryIndex = 0;
        primary.pdg = gun_->GetParticleDefinition()->GetPDGEncoding();
        primary.kineticEnergy = gun_->GetParticleEnergy();
        primary.time = gun_->GetParticleTime();
        primary.position = gun_->GetParticlePosition();
        primary.direction = gun_->GetParticleMomentumDirection().unit();
        runAction_->WritePrimary(primary);
    }

    gun_->GeneratePrimaryVertex(event);
}


void PrimaryGeneratorAction::GenerateSampleEvent(G4Event* event)
{
    if (sampleMinEnergy_ <= 0.0 || sampleMaxEnergy_ < sampleMinEnergy_) {
        throw std::runtime_error("Invalid /g4cosmic/sample energy range.");
    }
    if (sampleHalfWidthX_ < 0.0 || sampleHalfWidthY_ < 0.0 ||
        sampleMaxTheta_ < 0.0 || sampleMaxTheta_ >= 90.0 * deg) {
        throw std::runtime_error("Invalid /g4cosmic/sample geometry/angular range.");
    }

    auto* definition =
        G4ParticleTable::GetParticleTable()->FindParticle(sampleParticle_);
    if (!definition) {
        throw std::runtime_error("Unknown /g4cosmic/sample particle.");
    }

    // Energy is log-uniform by default so a single run covers stopping,
    // interacting, and through-going regimes without concentrating at the
    // high-energy end of a broad interval.
    G4double kineticEnergy = 0.0;
    if (sampleLogEnergy_ != 0 && sampleMaxEnergy_ > sampleMinEnergy_) {
        const double logMin = std::log(sampleMinEnergy_);
        const double logMax = std::log(sampleMaxEnergy_);
        kineticEnergy = std::exp(logMin + G4UniformRand() * (logMax - logMin));
    } else {
        kineticEnergy = sampleMinEnergy_ +
            G4UniformRand() * (sampleMaxEnergy_ - sampleMinEnergy_);
    }

    const G4double x = (2.0 * G4UniformRand() - 1.0) * sampleHalfWidthX_;
    const G4double y = (2.0 * G4UniformRand() - 1.0) * sampleHalfWidthY_;

    // Sample a downward cosmic-ray-like angular distribution.  For an
    // intensity proportional to cos(theta), the solid-angle PDF is
    // proportional to cos(theta) sin(theta), so sin^2(theta) is uniform.
    const double sinMax = std::sin(sampleMaxTheta_);
    const double sinTheta = std::sqrt(G4UniformRand()) * sinMax;
    const double theta = std::asin(sinTheta);
    const double phi = 2.0 * CLHEP::pi * G4UniformRand();
    const G4ThreeVector direction(
        std::sin(theta) * std::cos(phi),
        std::sin(theta) * std::sin(phi),
        -std::cos(theta));

    const G4ThreeVector position(x, y, generationZ_);

    gun_->SetParticleDefinition(definition);
    gun_->SetParticleEnergy(kineticEnergy);
    gun_->SetParticlePosition(position);
    gun_->SetParticleMomentumDirection(direction);
    gun_->SetParticleTime(0.0);

    if (runAction_) {
        PrimaryRecord primary;
        primary.eventID = event->GetEventID();
        primary.primaryIndex = 0;
        primary.pdg = definition->GetPDGEncoding();
        primary.kineticEnergy = kineticEnergy;
        primary.time = 0.0;
        primary.position = position;
        primary.direction = direction;
        runAction_->WritePrimary(primary);
    }

    gun_->GeneratePrimaryVertex(event);
}


void PrimaryGeneratorAction::GenerateCRYEvent(
    G4Event* event)
{
    // Lazy initialization lets a macro switch to /g4cosmic/source gun
    // without constructing an unused CRY generator first.
    if (!cryGenerator_) {
        InitializeCRY();
    }

    std::vector<CRYParticle*> particles;

    auto* particleTable =
        G4ParticleTable::GetParticleTable();

    // Volume acceptance is an enrichment mode: redraw whole CRY showers until
    // at least one valid primary intersects the selected logical-volume target.
    // "all" deliberately preserves the original CRY behavior.
    if (cryAcceptanceMode_ == "volume") {
        RefreshCRYAcceptanceVolumes();
    }

    std::size_t cryTrials = 0;

    while (true) {
        ++cryTrials;
        cryGenerator_->genEvent(&particles);

        if (cryAcceptanceMode_ == "all") {
            break;
        }

        bool showerAccepted = false;

        for (CRYParticle* cryParticle : particles) {
            if (!cryParticle) {
                continue;
            }

            G4ParticleDefinition* definition =
                particleTable->FindParticle(cryParticle->PDGid());

            if (!definition) {
                continue;
            }

            G4ThreeVector direction(
                cryParticle->u(),
                cryParticle->v(),
                cryParticle->w());

            if (direction.mag2() == 0.0) {
                continue;
            }

            direction = direction.unit();

            const G4ThreeVector position(
                cryParticle->x() * m,
                cryParticle->y() * m,
                generationZ_);

            if (AcceptCRYPrimary(position, direction)) {
                showerAccepted = true;
                break;
            }
        }

        if (showerAccepted) {
            break;
        }

        for (CRYParticle* cryParticle : particles) {
            delete cryParticle;
        }
        particles.clear();

        if (cryTrials >= static_cast<std::size_t>(cryMaxAcceptanceTrials_)) {
            throw std::runtime_error(
                "CRY acceptance failed after " +
                std::to_string(cryMaxAcceptanceTrials_) +
                " trials for logical-volume pattern '" +
                std::string(cryAcceptanceVolume_) +
                "'. Increase /g4cosmic/cry/maxAcceptanceTrials, increase the CRY subbox, or choose a larger acceptance volume.");
        }
    }

    // ---------------------------------------------------------------------
    // Diagnostic header
    // ---------------------------------------------------------------------

    if (cryVerbose_) G4cout
        << G4endl
        << "========================================"
        << G4endl
        << " CRY EVENT " << event->GetEventID()
        << G4endl
        << " Generated particles: "
        << particles.size()
        << G4endl
        << " CRY shower trials: "
        << cryTrials
        << G4endl
        << "========================================"
        << G4endl;

    std::size_t acceptedParticles = 0;

    for (std::size_t i = 0;
         i < particles.size();
         ++i) {

        CRYParticle* cryParticle =
            particles[i];

        if (!cryParticle) {
            if (cryVerbose_) G4cout
                << " [" << i
                << "] NULL CRY particle"
                << G4endl;

            continue;
        }

        const G4int pdg =
            cryParticle->PDGid();

        G4ParticleDefinition* definition =
            particleTable->FindParticle(pdg);

        // -------------------------------------------------------------
        // Preserve the raw CRY values separately so that the diagnostic
        // output lets us verify the CRY -> Geant4 conversion.
        // -------------------------------------------------------------

        const double cryX =
            cryParticle->x();

        const double cryY =
            cryParticle->y();

        const double cryU =
            cryParticle->u();

        const double cryV =
            cryParticle->v();

        const double cryW =
            cryParticle->w();

        const double cryKE =
            cryParticle->ke();

        const double cryTime =
            cryParticle->t();

        // -------------------------------------------------------------
        // Verified CRY -> Geant4 mapping.
        // CRY uses metres, MeV, seconds, and dimensionless direction cosines.
        // -------------------------------------------------------------

        const G4ThreeVector position(
            cryX * m,
            cryY * m,
            generationZ_);

        G4ThreeVector direction(
            cryU,
            cryV,
            cryW);

        // -------------------------------------------------------------
        // Print raw CRY information
        // -------------------------------------------------------------

        if (cryVerbose_) G4cout
            << std::fixed
            << std::setprecision(6);

        if (cryVerbose_) G4cout
            << " [" << i << "]"
            << G4endl;

        if (cryVerbose_) G4cout
            << "     PDG:        "
            << pdg;

        if (definition) {
            if (cryVerbose_) G4cout
                << " ("
                << definition->GetParticleName()
                << ")";
        }
        else {
            if (cryVerbose_) G4cout
                << " (UNKNOWN TO GEANT4)";
        }

        if (cryVerbose_) G4cout << G4endl;

        if (cryVerbose_) G4cout
            << "     CRY KE:     "
            << cryKE
            << G4endl;

        if (cryVerbose_) G4cout
            << "     CRY time:   "
            << cryTime
            << G4endl;

        if (cryVerbose_) G4cout
            << "     CRY x,y:    ("
            << cryX << ", "
            << cryY << ")"
            << G4endl;

        if (cryVerbose_) G4cout
            << "     CRY dir:    ("
            << cryU << ", "
            << cryV << ", "
            << cryW << ")"
            << G4endl;

        if (cryVerbose_) G4cout
            << "     |dir|^2:    "
            << direction.mag2()
            << G4endl;

        // -------------------------------------------------------------
        // Validate particle
        // -------------------------------------------------------------

        if (!definition) {
            if (cryVerbose_) G4cout
                << "     STATUS: SKIPPED - "
                << "PDG not known to Geant4"
                << G4endl
                << G4endl;

            delete cryParticle;
            continue;
        }

        if (direction.mag2() == 0.0) {
            if (cryVerbose_) G4cout
                << "     STATUS: SKIPPED - "
                << "zero momentum direction"
                << G4endl
                << G4endl;

            delete cryParticle;
            continue;
        }

        direction = direction.unit();

        // Geometric acceptance is applied only to CRY primaries.  It does
        // not inspect Geant4 interactions or detector response, so it cannot
        // leak simulation outcome information into generation.
        if (!AcceptCRYPrimary(position, direction)) {
            if (cryVerbose_) {
                G4cout
                    << "     STATUS: REJECTED - outside "
                    << cryAcceptanceMode_ << " acceptance";
                if (cryAcceptanceMode_ == "volume") {
                    G4cout << " for logical volume pattern '"
                           << cryAcceptanceVolume_ << "'";
                }
                G4cout << G4endl << G4endl;
            }
            delete cryParticle;
            continue;
        }

        // -------------------------------------------------------------
        // Print values that will actually be supplied to Geant4.
        // -------------------------------------------------------------

        if (cryVerbose_) G4cout
            << "     G4 position: ("
            << position.x() / m << ", "
            << position.y() / m << ", "
            << position.z() / m
            << ") m"
            << G4endl;

        if (cryVerbose_) G4cout
            << "     G4 dir:      ("
            << direction.x() << ", "
            << direction.y() << ", "
            << direction.z()
            << ")"
            << G4endl;

        if (cryVerbose_) G4cout
            << "     G4 KE:       "
            << cryKE
            << " MeV"
            << G4endl;

        if (cryVerbose_) G4cout
            << "     G4 time:     "
            << cryTime
            << " s"
            << G4endl;

        // -------------------------------------------------------------
        // Generate the Geant4 primary
        // -------------------------------------------------------------

        gun_->SetParticleDefinition(
            definition);

        gun_->SetParticleEnergy(
            cryKE * MeV);

        gun_->SetParticlePosition(
            position);

        gun_->SetParticleMomentumDirection(
            direction);

        // CRY documents t() in seconds.  Geant4 stores time in its
        // internal unit system, so the explicit * s conversion is required.
        gun_->SetParticleTime(
            cryTime * s);

        // Save the exact primary state supplied to Geant4.  In particular,
        // z_m records G4Cosmic's generation plane rather than CRY's source z.
        if (runAction_) {
            PrimaryRecord primary;
            primary.eventID = event->GetEventID();
            primary.primaryIndex = static_cast<int>(acceptedParticles);
            primary.pdg = pdg;
            primary.kineticEnergy = cryKE * MeV;
            primary.time = cryTime * s;
            primary.position = position;
            primary.direction = direction;
            runAction_->WritePrimary(primary);
        }

        gun_->GeneratePrimaryVertex(
            event);

        ++acceptedParticles;

        if (cryVerbose_) G4cout
            << "     STATUS: ACCEPTED"
            << G4endl
            << G4endl;

        delete cryParticle;
    }

    if (cryVerbose_) G4cout
        << "----------------------------------------"
        << G4endl
        << " CRY event "
        << event->GetEventID()
        << " summary"
        << G4endl
        << " CRY particles:      "
        << particles.size()
        << G4endl
        << " Geant4 primaries:   "
        << acceptedParticles
        << G4endl
        << "----------------------------------------"
        << G4endl;
}