#pragma once

class G4Event;
class RunAction;

namespace G4Cosmic {

class PrimaryGenerator {
public:
    virtual ~PrimaryGenerator() = default;

    virtual const char* Name() const = 0;
    virtual void GeneratePrimaries(G4Event* event, RunAction* runAction) = 0;
};

} // namespace G4Cosmic
