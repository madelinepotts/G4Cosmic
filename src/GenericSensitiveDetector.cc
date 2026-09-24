#include "G4Cosmic/GenericSensitiveDetector.hh"

#include "G4Event.hh"
#include "G4EventManager.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4TouchableHistory.hh"
#include "G4Track.hh"

#include "HitRecord.hh"
#include "RunAction.hh"

namespace G4Cosmic {

GenericSensitiveDetector::GenericSensitiveDetector(const G4String& name)
    : G4VSensitiveDetector(name) {}

G4bool GenericSensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory*) {
  if (step == nullptr) {
    return false;
  }

  const auto edep = step->GetTotalEnergyDeposit();
  if (edep <= 0.0) {
    return false;
  }

  auto* pre = step->GetPreStepPoint();
  auto* post = step->GetPostStepPoint();
  auto* track = step->GetTrack();

  if (pre == nullptr || post == nullptr || track == nullptr) {
    return false;
  }

  HitRecord hit;

  if (const auto* event = G4EventManager::GetEventManager()->GetConstCurrentEvent()) {
    hit.eventID = event->GetEventID();
  }

  const auto touch = pre->GetTouchableHandle();
  if (touch) {
    hit.channelID = touch->GetCopyNumber();
  }

  hit.trackID = track->GetTrackID();
  hit.parentID = track->GetParentID();

  if (const auto* particle = track->GetParticleDefinition()) {
    hit.pdg = particle->GetPDGEncoding();
  }

  hit.edep = edep;
  hit.time = pre->GetGlobalTime();
  hit.position = 0.5 * (pre->GetPosition() + post->GetPosition());

  const auto* runAction = dynamic_cast<const ::RunAction*>(
      G4RunManager::GetRunManager()->GetUserRunAction());
  if (runAction != nullptr) {
    runAction->WriteHit(hit);
  }

  return true;
}

}  // namespace G4Cosmic
