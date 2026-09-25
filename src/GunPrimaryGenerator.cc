#include "G4Cosmic/GunPrimaryGenerator.hh"

#include "PrimaryRecord.hh"
#include "RunAction.hh"

#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"

namespace G4Cosmic {

GunPrimaryGenerator::GunPrimaryGenerator()
    : gun_(std::make_unique<G4ParticleGun>(1))
{
    gun_->SetParticleDefinition(
        G4ParticleTable::GetParticleTable()->FindParticle("mu-"));
    gun_->SetParticleEnergy(4.0 * GeV);
    gun_->SetParticlePosition(G4ThreeVector(0.0, 0.0, 1.65 * m));
    gun_->SetParticleMomentumDirection(
        G4ThreeVector(0.04, -0.03, -1.0).unit());
}

GunPrimaryGenerator::~GunPrimaryGenerator() = default;

void GunPrimaryGenerator::GeneratePrimaries(G4Event* event, RunAction* runAction)
{
    if (runAction != nullptr) {
        PrimaryRecord primary;
        primary.eventID = event->GetEventID();
        primary.primaryIndex = 0;
        primary.pdg = gun_->GetParticleDefinition()->GetPDGEncoding();
        primary.particleName = gun_->GetParticleDefinition()->GetParticleName();
        primary.kineticEnergy = gun_->GetParticleEnergy();
        primary.time = gun_->GetParticleTime();
        primary.position = gun_->GetParticlePosition();
        primary.direction = gun_->GetParticleMomentumDirection().unit();
        runAction->WritePrimary(primary);
    }

    gun_->GeneratePrimaryVertex(event);
}

} // namespace G4Cosmic
