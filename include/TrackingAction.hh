#pragma once

#include "G4UserTrackingAction.hh"
#include "globals.hh"

class G4Track;
class RunAction;

class TrackingAction : public G4UserTrackingAction {
public:
  explicit TrackingAction(RunAction* runAction);
  ~TrackingAction() override = default;

  void PreUserTrackingAction(const G4Track* track) override;
  void PostUserTrackingAction(const G4Track* track) override;

private:
  RunAction* runAction_ = nullptr;
  G4double startKineticEnergy_ = 0.0;
};
