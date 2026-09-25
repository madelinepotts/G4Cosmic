#include "OutputMessenger.hh"
#include "OutputConfig.hh"
#include "G4StateManager.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithAString.hh"
#include "G4ios.hh"

OutputMessenger::OutputMessenger() {
    fileCommand_ = new G4UIcmdWithAString("/g4cosmic/output/file", this);
    fileCommand_->SetGuidance("Set the ROOT output filename for this run.");
    fileCommand_->SetParameterName("filename", false);
    fileCommand_->AvailableForStates(G4State_PreInit, G4State_Idle);

    trackEndCommand_ = new G4UIcmdWithABool("/g4cosmic/output/trackEnd", this);
    trackEndCommand_->SetGuidance("Enable or disable the optional track_end ROOT tree.");
    trackEndCommand_->SetGuidance("Default: false. When enabled, G4Cosmic records generic track termination truth.");
    trackEndCommand_->SetParameterName("enabled", false);
    trackEndCommand_->AvailableForStates(G4State_PreInit, G4State_Idle);
}
OutputMessenger::~OutputMessenger() {
    delete fileCommand_;
    delete trackEndCommand_;
}

void OutputMessenger::SetNewValue(G4UIcommand* command, G4String value) {
    if (command == fileCommand_) {
        OutputConfig::SetFileName(value);
        G4cout << "G4Cosmic output ROOT file: " << OutputConfig::GetFileName() << G4endl;
        return;
    }

    if (command == trackEndCommand_) {
        OutputConfig::SetWriteTrackEnd(trackEndCommand_->GetNewBoolValue(value));
        G4cout << "G4Cosmic optional track_end tree: "
               << (OutputConfig::GetWriteTrackEnd() ? "enabled" : "disabled")
               << G4endl;
    }
}
