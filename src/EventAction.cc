#include "EventAction.hh"
#include "RunAction.hh"

EventAction::EventAction(RunAction* runAction) : runAction_(runAction) {}

void EventAction::EndOfEventAction(const G4Event*) {
  if (runAction_ != nullptr) {
    runAction_->FlushReducedHits();
  }
}
