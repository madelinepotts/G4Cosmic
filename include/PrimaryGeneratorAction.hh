#pragma once

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

#include <map>
#include <memory>

class G4Event;
class G4GenericMessenger;
class RunAction;

namespace G4Cosmic {
class PrimaryGenerator;
}

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
    explicit PrimaryGeneratorAction(RunAction* runAction);
    ~PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* event) override;

private:
    void ConfigureMessenger();
    void SetSource(const G4String& source);
    void RegisterGenerator(std::unique_ptr<G4Cosmic::PrimaryGenerator> generator);

    RunAction* runAction_ = nullptr;
    std::unique_ptr<G4GenericMessenger> messenger_;
    std::map<G4String, std::unique_ptr<G4Cosmic::PrimaryGenerator>> generators_;
    G4Cosmic::PrimaryGenerator* activeGenerator_ = nullptr;
    G4String sourceMode_ = "cry";
};
