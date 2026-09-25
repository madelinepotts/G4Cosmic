#pragma once

#include "G4Cosmic/PrimaryGenerator.hh"

#include <memory>

class G4ParticleGun;

namespace G4Cosmic {

class GunPrimaryGenerator final : public PrimaryGenerator {
public:
    GunPrimaryGenerator();
    ~GunPrimaryGenerator() override;

    const char* Name() const override { return "gun"; }
    void GeneratePrimaries(G4Event* event, RunAction* runAction) override;

private:
    std::unique_ptr<G4ParticleGun> gun_;
};

} // namespace G4Cosmic
