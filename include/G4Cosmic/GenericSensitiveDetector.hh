#pragma once

#include "G4VSensitiveDetector.hh"

class G4Step;
class G4TouchableHistory;

namespace G4Cosmic {

class GenericSensitiveDetector : public G4VSensitiveDetector {
public:
  explicit GenericSensitiveDetector(const G4String& name);
  G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
};

}  // namespace G4Cosmic
