#pragma once

#include "G4Cosmic/PrimaryGenerator.hh"

#include "G4ThreeVector.hh"
#include "globals.hh"

#include <memory>
#include <string>
#include <vector>

class CRYGenerator;
class CRYSetup;
class G4GenericMessenger;
class G4ParticleGun;

namespace G4Cosmic {

class CRYPrimaryGenerator final : public PrimaryGenerator {
public:
    CRYPrimaryGenerator();
    ~CRYPrimaryGenerator() override;

    const char* Name() const override { return "cry"; }
    void GeneratePrimaries(G4Event* event, RunAction* runAction) override;

private:
    void ConfigureMessenger();
    void InitializeCRY();
    void ApplyConfiguration();
    void SetDate(const G4String& date);
    void SetAcceptanceMode(const G4String& mode);
    void SetAcceptanceVolume(const G4String& logicalVolumeName);
    bool AcceptPrimary(const G4ThreeVector& position, const G4ThreeVector& direction) const;
    void RefreshAcceptanceVolumes() const;
    std::string BuildSetupText() const;

    struct AcceptanceBox {
        G4String physicalVolumeName;
        G4String logicalVolumeName;
        G4ThreeVector min;
        G4ThreeVector max;
    };

    std::unique_ptr<G4ParticleGun> gun_;
    std::unique_ptr<CRYSetup> crySetup_;
    std::unique_ptr<CRYGenerator> cryGenerator_;
    std::unique_ptr<G4GenericMessenger> messenger_;

    G4double generationZ_ = 0.0;

    G4int returnNeutrons_ = 1;
    G4int returnProtons_ = 1;
    G4int returnGammas_ = 1;
    G4int returnElectrons_ = 1;
    G4int returnMuons_ = 1;
    G4int returnPions_ = 1;
    G4int returnKaons_ = 1;
    G4double subboxLength_ = 2.0;
    G4double altitude_ = 0.0;
    G4double latitude_ = 46.3;
    G4String date_ = "9-18-2026";
    G4int nParticlesMin_ = 1;
    G4int nParticlesMax_ = 1000000;
    G4double xoffset_ = 0.0;
    G4double yoffset_ = 0.0;
    G4double zoffset_ = 0.0;
    G4int cryVerbose_ = 0;
    G4String cryAcceptanceMode_ = "all";
    G4String cryAcceptanceVolume_ = "";
    G4int cryMaxAcceptanceTrials_ = 10000;

    mutable G4String cachedAcceptanceVolume_;
    mutable std::vector<AcceptanceBox> acceptanceBoxes_;
};

} // namespace G4Cosmic
