#pragma once

#include "G4UserEventAction.hh"

class G4Event;
class RunAction;

class EventAction : public G4UserEventAction {
public:
  explicit EventAction(RunAction* runAction);
  ~EventAction() override = default;

  void EndOfEventAction(const G4Event*) override;

private:
  RunAction* runAction_ = nullptr;
};
