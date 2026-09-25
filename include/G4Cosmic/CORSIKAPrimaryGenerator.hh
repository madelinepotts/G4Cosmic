#pragma once

#include "G4Cosmic/PrimaryGenerator.hh"

#include "G4String.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

#include <memory>
#include <string>
#include <vector>

class G4Event;
class G4GenericMessenger;
class G4ParticleGun;

namespace G4Cosmic {

class CORSIKAPrimaryGenerator final : public PrimaryGenerator {
public:
    CORSIKAPrimaryGenerator();
    ~CORSIKAPrimaryGenerator() override;

    const char* Name() const override { return "corsika"; }
    void GeneratePrimaries(G4Event* event, RunAction* runAction) override;

private:
    struct ParticleRecord {
        G4int inputId = 0;
        G4int pdg = 0;
        G4double x = 0.0;
        G4double y = 0.0;
        G4double z = 0.0;
        G4double px = 0.0;
        G4double py = 0.0;
        G4double pz = 0.0;
        G4double time = 0.0;
    };

    struct ShowerRecord {
        G4int inputEventId = 0;
        std::vector<ParticleRecord> particles;
    };

    struct AcceptanceBox {
        G4String physicalVolumeName;
        G4String logicalVolumeName;
        G4ThreeVector min;
        G4ThreeVector max;
    };

    void ConfigureMessenger();
    void ApplyConfiguration();
    void SetGenerationMode(const G4String& mode);
    void SetInputMode(const G4String& mode); // compatibility alias
    void SetCacheFile(const G4String& fileName);
    void SetFileName(const G4String& fileName); // compatibility alias
    void SetCommand(const G4String& command); // legacy single-token command; prefer runner/runnerScript
    void SetRunner(const G4String& executable);
    void SetRunnerScript(const G4String& script);
    void SetPrimary(const G4String& primary);
    void SetEnergyMode(const G4String& mode);
    void SetIdScheme(const G4String& scheme);
    void SetPositionUnit(const G4String& unitName);
    void SetMomentumUnit(const G4String& unitName);
    void SetTimeUnit(const G4String& unitName);
    void SetAcceptanceMode(const G4String& mode);
    void SetAcceptanceVolume(const G4String& logicalVolumeName);
    void Reload();

    void LoadIfNeeded() const;
    void GenerateBatchFile() const;
    std::vector<ShowerRecord> ReadShowerFile(const G4String& fileName, G4int& skippedLines) const;
    void ReplaceLoadedShowers(std::vector<ShowerRecord>&& showers, G4int skippedLines) const;
    std::string ExpandedCommand() const;
    std::string BuildDefaultRunnerCommand() const;

    G4int ConvertInputId(G4int inputId) const;
    G4double PositionScale() const;
    G4double MomentumScale() const;
    G4double TimeScale() const;

    bool AcceptPrimary(const G4ThreeVector& position, const G4ThreeVector& direction) const;
    bool AcceptShower(const ShowerRecord& shower) const;
    const ShowerRecord& SelectShower(G4int eventId) const;
    void RefreshAcceptanceVolumes() const;

    std::unique_ptr<G4ParticleGun> gun_;
    std::unique_ptr<G4GenericMessenger> messenger_;

    // File mode parses an existing G4Cosmic/CORSIKA text .dat shower list.
    // Batch mode runs a CORSIKA 8 wrapper once to fill the cache file.
    G4String generationMode_ = "batch";
    G4String cacheFile_ = "corsika_cache.dat";
    G4String command_;
    G4String runner_ = "python";
    G4String runnerScript_ = "examples/corsika/demo_external_corsika_runner.py";
    G4int eventsPerBatch_ = 1000;
    G4int reuseCache_ = 0;
    G4int regenerate_ = 0;

    // Shower-generation knobs forwarded to external CORSIKA 8 runners through
    // command-token expansion. They also document the intended macro API even
    // when the wrapper has not been replaced with a real CORSIKA 8 application.
    G4String primary_ = "proton";
    G4String energyMode_ = "powerLaw";
    G4double minEnergy_ = 0.0;
    G4double maxEnergy_ = 0.0;
    G4double monoEnergy_ = 0.0;
    G4double spectralIndex_ = 2.7;
    G4double minZenith_ = 0.0;
    G4double maxZenith_ = 0.0;
    G4double minAzimuth_ = 0.0;
    G4double maxAzimuth_ = 0.0;

    G4String idScheme_ = "corsika";
    G4String positionUnit_ = "m";
    G4String momentumUnit_ = "GeV";
    G4String timeUnit_ = "ns";
    G4int loop_ = 1;
    G4int verbose_ = 0;

    G4String acceptanceMode_ = "all";
    G4String acceptanceVolume_;
    G4int maxAcceptanceTrials_ = 10000;

    mutable G4String loadedCacheFile_;
    mutable G4String loadedGenerationMode_;
    mutable G4String loadedIdScheme_;
    mutable G4String loadedPositionUnit_;
    mutable G4String loadedMomentumUnit_;
    mutable G4String loadedTimeUnit_;
    mutable std::vector<ShowerRecord> showers_;
    mutable G4int skippedLines_ = 0;

    mutable G4String cachedCommand_;
    mutable G4String cachedCacheFile_;
    mutable G4int cachedEventsPerBatch_ = 0;
    mutable G4int generatedBatchCount_ = 0;

    mutable G4String cachedAcceptanceVolume_;
    mutable std::vector<AcceptanceBox> acceptanceBoxes_;
};

} // namespace G4Cosmic
