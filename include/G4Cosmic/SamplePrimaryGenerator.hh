#pragma once

#include "G4Cosmic/PrimaryGenerator.hh"

#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

#include <memory>
#include <vector>

class G4GenericMessenger;
class G4ParticleGun;
class G4VSolid;

namespace G4Cosmic {

class SamplePrimaryGenerator final : public PrimaryGenerator {
public:
    SamplePrimaryGenerator();
    ~SamplePrimaryGenerator() override;

    const char* Name() const override { return "sample"; }
    void GeneratePrimaries(G4Event* event, RunAction* runAction) override;

private:
    struct SourcePlacement {
        G4String physicalVolumeName;
        G4String logicalVolumeName;
        const G4VSolid* solid = nullptr;
        G4RotationMatrix localToWorldRotation;
        G4ThreeVector localToWorldTranslation;
        G4ThreeVector localMin;
        G4ThreeVector localMax;
    };

    void ConfigureMessenger();
    void SetParticle(const G4String& particle);
    void SetPositionMode(const G4String& mode);
    void SetSourceVolume(const G4String& logicalVolumeName);
    void RefreshSourceVolumes() const;
    G4ThreeVector SampleVolumePosition() const;
    G4ThreeVector SampleLocalPointInside(const SourcePlacement& placement) const;
    G4ThreeVector SampleLocalSurfacePoint(const SourcePlacement& placement) const;
    G4ThreeVector SampleIsotropicDirection() const;
    G4ThreeVector SampleDirection() const;
    G4double SampleKineticEnergy() const;

    std::unique_ptr<G4ParticleGun> gun_;
    std::unique_ptr<G4GenericMessenger> messenger_;

    G4String particle_ = "mu-";
    G4double minEnergy_ = 30.0;      // MeV
    G4double maxEnergy_ = 10000.0;   // MeV
    G4double maxTheta_ = 30.0;       // deg, converted in constructor
    G4int logEnergy_ = 1;

    // The sample generator is volume-based by default. The source position is
    // sampled either inside the selected logical volume or on its surface.
    G4String positionMode_ = "volume";
    G4String sourceVolume_ = "";
    G4int maxPositionTrials_ = 10000;

    mutable G4String cachedSourceVolume_;
    mutable std::vector<SourcePlacement> sourcePlacements_;
};

} // namespace G4Cosmic
