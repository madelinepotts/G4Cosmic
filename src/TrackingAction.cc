#include "TrackingAction.hh"

#include "OutputConfig.hh"
#include "RunAction.hh"
#include "TrackEndRecord.hh"

#include "G4Event.hh"
#include "G4Material.hh"
#include "G4ParticleDefinition.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4TouchableHandle.hh"
#include "G4VPhysicalVolume.hh"
#include "G4LogicalVolume.hh"
#include "G4VProcess.hh"

TrackingAction::TrackingAction(RunAction* runAction)
    : runAction_(runAction) {}

void TrackingAction::PreUserTrackingAction(const G4Track* track) {
  if (track == nullptr) {
    startKineticEnergy_ = 0.0;
    return;
  }
  startKineticEnergy_ = track->GetKineticEnergy();
}

void TrackingAction::PostUserTrackingAction(const G4Track* track) {
  if (!OutputConfig::GetWriteTrackEnd() || runAction_ == nullptr || track == nullptr) {
    return;
  }

  TrackEndRecord record;

  if (const auto* runManager = G4RunManager::GetRunManager()) {
    if (const auto* event = runManager->GetCurrentEvent()) {
      record.eventID = event->GetEventID();
    }
  }

  record.trackID = track->GetTrackID();
  record.parentID = track->GetParentID();

  if (const auto* particle = track->GetParticleDefinition()) {
    record.pdg = particle->GetPDGEncoding();
    record.particleName = particle->GetParticleName();
  }

  record.startKineticEnergyMeV = startKineticEnergy_ / MeV;
  record.endKineticEnergyMeV = track->GetKineticEnergy() / MeV;

  const auto& position = track->GetPosition();
  record.xMm = position.x() / mm;
  record.yMm = position.y() / mm;
  record.zMm = position.z() / mm;

  record.trackLengthMm = track->GetTrackLength() / mm;
  record.globalTimeNs = track->GetGlobalTime() / ns;
  record.stopped = track->GetKineticEnergy() <= 1.0 * eV;

  if (const auto* step = track->GetStep()) {
    if (const auto* post = step->GetPostStepPoint()) {
      if (const auto* process = post->GetProcessDefinedStep()) {
        record.endProcess = process->GetProcessName();
      }

      if (const auto* material = post->GetMaterial()) {
        record.material = material->GetName();
      }

      const auto touch = post->GetTouchableHandle();
      if (touch) {
        if (const auto* physical = touch->GetVolume()) {
          record.physicalVolume = physical->GetName();
          if (const auto* logical = physical->GetLogicalVolume()) {
            record.logicalVolume = logical->GetName();
          }
        }
      }
    }
  }

  if (record.material.empty() && track->GetMaterial() != nullptr) {
    record.material = track->GetMaterial()->GetName();
  }

  runAction_->RecordTrackEnd(record);
}
