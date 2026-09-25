#include "PrimaryGeneratorAction.hh"

#include "G4Cosmic/CRYPrimaryGenerator.hh"
#include "G4Cosmic/GunPrimaryGenerator.hh"
#include "G4Cosmic/PrimaryGenerator.hh"
#include "G4Cosmic/SamplePrimaryGenerator.hh"

#include "G4Event.hh"
#include "G4GenericMessenger.hh"
#include "G4ios.hh"
#include "G4Threading.hh"

#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace {
bool IsMasterThread() {
#ifdef G4MULTITHREADED
    return G4Threading::IsMasterThread();
#else
    return true;
#endif
}
}

PrimaryGeneratorAction::PrimaryGeneratorAction(RunAction* runAction)
    : runAction_(runAction)
{
    if (const char* mode = std::getenv("G4COSMIC_SOURCE")) {
        sourceMode_ = mode;
    }

    RegisterGenerator(std::make_unique<G4Cosmic::CRYPrimaryGenerator>());
    RegisterGenerator(std::make_unique<G4Cosmic::GunPrimaryGenerator>());
    RegisterGenerator(std::make_unique<G4Cosmic::SamplePrimaryGenerator>());

    ConfigureMessenger();
    SetSource(sourceMode_);

    if (activeGenerator_ == nullptr) {
        throw std::runtime_error(
            "No active G4Cosmic primary generator was configured.");
    }
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() = default;

void PrimaryGeneratorAction::RegisterGenerator(
    std::unique_ptr<G4Cosmic::PrimaryGenerator> generator)
{
    if (!generator) {
        return;
    }

    const G4String name = generator->Name();
    generators_[name] = std::move(generator);
}

void PrimaryGeneratorAction::ConfigureMessenger()
{
    messenger_ = std::make_unique<G4GenericMessenger>(
        this, "/g4cosmic/", "G4Cosmic primary-source controls");
    messenger_->DeclareMethod(
        "source", &PrimaryGeneratorAction::SetSource,
        "Primary source: cry, gun, or sample.");
}

void PrimaryGeneratorAction::SetSource(const G4String& source)
{
    auto found = generators_.find(source);
    if (found == generators_.end()) {
        if (IsMasterThread()) {
            G4cout << "G4Cosmic: unknown source '" << source
                   << "'. Available sources:";
            for (const auto& item : generators_) {
                G4cout << " " << item.first;
            }
            G4cout << G4endl;
        }
        return;
    }

    sourceMode_ = source;
    activeGenerator_ = found->second.get();
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    if (activeGenerator_ == nullptr) {
        throw std::runtime_error(
            "No active G4Cosmic primary generator was configured.");
    }

    activeGenerator_->GeneratePrimaries(event, runAction_);
}
