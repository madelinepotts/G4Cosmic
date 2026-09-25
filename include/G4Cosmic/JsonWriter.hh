#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace G4Cosmic {

/**
 * \brief Small dependency-free JSON writer used for run metadata sidecars.
 *
 * It intentionally supports only the operations G4Cosmic needs so the framework
 * can write provenance without adding a large JSON dependency.
 */
class JsonWriter {
public:
  explicit JsonWriter(std::ostream& out, int indentSpaces = 2);

  void BeginObject();
  void BeginObject(const std::string& name);
  void EndObject();

  void BeginArray(const std::string& name);
  void BeginArrayValue();
  void EndArray();

  void Write(const std::string& name, const std::string& value);
  void Write(const std::string& name, const char* value);
  void Write(const std::string& name, bool value);
  void Write(const std::string& name, int value);
  void Write(const std::string& name, long long value);
  void Write(const std::string& name, double value);

  void WriteValue(const std::string& value);
  void WriteValue(bool value);
  void WriteValue(int value);
  void WriteValue(long long value);
  void WriteValue(double value);

  static std::string Escape(const std::string& value);

private:
  enum class ContextType { Object, Array };
  struct Context {
    ContextType type;
    bool first = true;
  };

  void PrefixNamedValue(const std::string& name);
  void PrefixArrayValue();
  void NewLineAndIndent();
  void WriteQuoted(const std::string& value);

  std::ostream& out_;
  int indentSpaces_ = 2;
  std::vector<Context> stack_;
};

}  // namespace G4Cosmic
