#pragma once

#include "G4VSensitiveDetector.hh"

namespace WarpTrackExample {

class SensitiveDetector : public G4VSensitiveDetector {
public:
  explicit SensitiveDetector(const G4String& name);
  G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
};

}  // namespace WarpTrackExample
