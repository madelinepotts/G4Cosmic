#pragma once

class G4Event;
class RunAction;

namespace G4Cosmic {

class JsonWriter;

/**
 * \brief Runtime-selectable primary-source backend interface.
 *
 * PrimaryGenerator implementations are registered with PrimaryGeneratorAction
 * and selected by macros such as `/g4cosmic/source cry` or
 * `/g4cosmic/source corsika`.  Each implementation is responsible for turning
 * its configured source model into Geant4 primaries for one event.
 */
class PrimaryGenerator {
public:
    virtual ~PrimaryGenerator() = default;

    /// Macro-facing source name, for example `cry`, `corsika`, `gun`, or `sample`.
    virtual const char* Name() const = 0;

    /// Generate all primaries for the current Geant4 event.
    virtual void GeneratePrimaries(G4Event* event, RunAction* runAction) = 0;

    /**
     * \brief Append source-specific run provenance to the JSON sidecar.
     *
     * The core calls this inside the top-level `source` object.  Implementations
     * should record configuration that is meaningful for reproducing the source
     * behavior without adding detector-specific assumptions.
     */
    virtual void AppendMetadata(JsonWriter& json) const;
};

} // namespace G4Cosmic
