#pragma once

#include "G4UImessenger.hh"

class G4UIcmdWithABool;
class G4UIcmdWithAString;
class G4UIcommand;

class OutputMessenger : public G4UImessenger {
public:
    OutputMessenger();
    ~OutputMessenger() override;

    void SetNewValue(G4UIcommand* command, G4String value) override;

private:
    G4UIcmdWithAString* fileCommand_ = nullptr;
    G4UIcmdWithABool* trackEndCommand_ = nullptr;
    G4UIcmdWithABool* reducedHitsCommand_ = nullptr;
    G4UIcmdWithAString* reduceByCommand_ = nullptr;
    G4UIcmdWithABool* printMacroCommandsCommand_ = nullptr;
};
