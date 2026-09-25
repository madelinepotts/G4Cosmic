#include "G4Cosmic/PrimaryGenerator.hh"

#include "G4Cosmic/JsonWriter.hh"

namespace G4Cosmic {

void PrimaryGenerator::AppendMetadata(JsonWriter& json) const {
  json.Write("type", Name());
}

}  // namespace G4Cosmic
