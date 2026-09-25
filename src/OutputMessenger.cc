#include "OutputMessenger.hh"
#include "OutputConfig.hh"
#include "G4StateManager.hh"
#include "G4Threading.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithAString.hh"
#include "G4ios.hh"

namespace {
bool IsMasterThread() {
#ifdef G4MULTITHREADED
    return G4Threading::IsMasterThread();
#else
    return true;
#endif
}
}

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

    reducedHitsCommand_ = new G4UIcmdWithABool("/g4cosmic/output/reducedHits", this);
    reducedHitsCommand_->SetGuidance("Enable or disable reduced sensitive-volume hit trees.");
    reducedHitsCommand_->SetGuidance("Default: false. Reduced trees are named <logical_volume>_reduced.");
    reducedHitsCommand_->SetParameterName("enabled", false);
    reducedHitsCommand_->AvailableForStates(G4State_PreInit, G4State_Idle);

    reduceByCommand_ = new G4UIcmdWithAString("/g4cosmic/output/reduceBy", this);
    reduceByCommand_->SetGuidance("Set the reduced-hit grouping mode.");
    reduceByCommand_->SetGuidance("Supported values: copyNo, particleCopyNo, physicalVolume.");
    reduceByCommand_->SetGuidance("Aliases: particleTypeCopyNo, pdgCopyNo, volume.");
    reduceByCommand_->SetParameterName("mode", false);
    reduceByCommand_->AvailableForStates(G4State_PreInit, G4State_Idle);
}
OutputMessenger::~OutputMessenger() {
    delete fileCommand_;
    delete trackEndCommand_;
    delete reducedHitsCommand_;
    delete reduceByCommand_;
}

void OutputMessenger::SetNewValue(G4UIcommand* command, G4String value) {
    if (command == fileCommand_) {
        OutputConfig::SetFileName(value);
        if (IsMasterThread()) {
            G4cout << "G4Cosmic output ROOT file: " << OutputConfig::GetFileName() << G4endl;
        }
        return;
    }

    if (command == trackEndCommand_) {
        OutputConfig::SetWriteTrackEnd(trackEndCommand_->GetNewBoolValue(value));
        if (IsMasterThread()) {
            G4cout << "G4Cosmic optional track_end tree: "
                   << (OutputConfig::GetWriteTrackEnd() ? "enabled" : "disabled")
                   << G4endl;
        }
        return;
    }

    if (command == reducedHitsCommand_) {
        OutputConfig::SetWriteReducedHits(reducedHitsCommand_->GetNewBoolValue(value));
        if (IsMasterThread()) {
            G4cout << "G4Cosmic reduced hit trees: "
                   << (OutputConfig::GetWriteReducedHits() ? "enabled" : "disabled")
                   << G4endl;
        }
        return;
    }

    if (command == reduceByCommand_) {
        const auto before = OutputConfig::GetReducedHitMode();
        OutputConfig::SetReducedHitMode(value);
        const auto after = OutputConfig::GetReducedHitMode();
        if (after == before && value != before) {
            if (IsMasterThread()) {
                G4cout << "G4Cosmic warning: unknown reduced-hit mode '" << value
                       << "'. Keeping mode '" << after << "'." << G4endl;
            }
        } else {
            if (IsMasterThread()) {
                G4cout << "G4Cosmic reduced-hit mode: " << after << G4endl;
            }
        }
    }
}
