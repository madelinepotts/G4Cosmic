#pragma once

class G4Event;
class RunAction;

namespace G4Cosmic {

class JsonWriter;

class PrimaryGenerator {
public:
    virtual ~PrimaryGenerator() = default;

    virtual const char* Name() const = 0;
    virtual void GeneratePrimaries(G4Event* event, RunAction* runAction) = 0;

    // Sources may append source-specific run provenance to the JSON sidecar.
    // The core calls this inside the top-level "source" object.
    virtual void AppendMetadata(JsonWriter& json) const;
};

} // namespace G4Cosmic
