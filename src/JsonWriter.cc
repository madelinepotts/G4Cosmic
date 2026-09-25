#include "G4Cosmic/JsonWriter.hh"

#include <cmath>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace G4Cosmic {

JsonWriter::JsonWriter(std::ostream& out, int indentSpaces)
    : out_(out), indentSpaces_(indentSpaces) {}

void JsonWriter::NewLineAndIndent() {
  out_ << '\n';
  const auto depth = static_cast<int>(stack_.size());
  for (int i = 0; i < depth * indentSpaces_; ++i) {
    out_ << ' ';
  }
}

void JsonWriter::PrefixNamedValue(const std::string& name) {
  if (stack_.empty() || stack_.back().type != ContextType::Object) {
    throw std::runtime_error("JsonWriter named value outside object.");
  }

  if (!stack_.back().first) {
    out_ << ',';
  }
  stack_.back().first = false;
  NewLineAndIndent();
  WriteQuoted(name);
  out_ << ": ";
}

void JsonWriter::PrefixArrayValue() {
  if (stack_.empty() || stack_.back().type != ContextType::Array) {
    throw std::runtime_error("JsonWriter array value outside array.");
  }

  if (!stack_.back().first) {
    out_ << ',';
  }
  stack_.back().first = false;
  NewLineAndIndent();
}

void JsonWriter::BeginObject() {
  if (!stack_.empty()) {
    PrefixArrayValue();
  }
  out_ << '{';
  stack_.push_back({ContextType::Object, true});
}

void JsonWriter::BeginObject(const std::string& name) {
  PrefixNamedValue(name);
  out_ << '{';
  stack_.push_back({ContextType::Object, true});
}

void JsonWriter::EndObject() {
  if (stack_.empty() || stack_.back().type != ContextType::Object) {
    throw std::runtime_error("JsonWriter EndObject without object.");
  }

  const bool wasEmpty = stack_.back().first;
  stack_.pop_back();
  if (!wasEmpty) {
    NewLineAndIndent();
  }
  out_ << '}';
}

void JsonWriter::BeginArray(const std::string& name) {
  PrefixNamedValue(name);
  out_ << '[';
  stack_.push_back({ContextType::Array, true});
}

void JsonWriter::BeginArrayValue() {
  PrefixArrayValue();
  out_ << '[';
  stack_.push_back({ContextType::Array, true});
}

void JsonWriter::EndArray() {
  if (stack_.empty() || stack_.back().type != ContextType::Array) {
    throw std::runtime_error("JsonWriter EndArray without array.");
  }

  const bool wasEmpty = stack_.back().first;
  stack_.pop_back();
  if (!wasEmpty) {
    NewLineAndIndent();
  }
  out_ << ']';
}

void JsonWriter::WriteQuoted(const std::string& value) {
  out_ << '"' << Escape(value) << '"';
}

void JsonWriter::Write(const std::string& name, const std::string& value) {
  PrefixNamedValue(name);
  WriteQuoted(value);
}

void JsonWriter::Write(const std::string& name, const char* value) {
  Write(name, std::string(value == nullptr ? "" : value));
}

void JsonWriter::Write(const std::string& name, bool value) {
  PrefixNamedValue(name);
  out_ << (value ? "true" : "false");
}

void JsonWriter::Write(const std::string& name, int value) {
  PrefixNamedValue(name);
  out_ << value;
}

void JsonWriter::Write(const std::string& name, long long value) {
  PrefixNamedValue(name);
  out_ << value;
}

void JsonWriter::Write(const std::string& name, double value) {
  PrefixNamedValue(name);
  if (std::isfinite(value)) {
    out_ << std::setprecision(12) << value;
  } else {
    out_ << "null";
  }
}

void JsonWriter::WriteValue(const std::string& value) {
  PrefixArrayValue();
  WriteQuoted(value);
}

void JsonWriter::WriteValue(bool value) {
  PrefixArrayValue();
  out_ << (value ? "true" : "false");
}

void JsonWriter::WriteValue(int value) {
  PrefixArrayValue();
  out_ << value;
}

void JsonWriter::WriteValue(long long value) {
  PrefixArrayValue();
  out_ << value;
}

void JsonWriter::WriteValue(double value) {
  PrefixArrayValue();
  if (std::isfinite(value)) {
    out_ << std::setprecision(12) << value;
  } else {
    out_ << "null";
  }
}

std::string JsonWriter::Escape(const std::string& value) {
  std::ostringstream escaped;
  for (const unsigned char c : value) {
    switch (c) {
      case '"': escaped << "\\\""; break;
      case '\\': escaped << "\\\\"; break;
      case '\b': escaped << "\\b"; break;
      case '\f': escaped << "\\f"; break;
      case '\n': escaped << "\\n"; break;
      case '\r': escaped << "\\r"; break;
      case '\t': escaped << "\\t"; break;
      default:
        if (c < 0x20) {
          escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                  << static_cast<int>(c) << std::dec << std::setfill(' ');
        } else {
          escaped << static_cast<char>(c);
        }
        break;
    }
  }
  return escaped.str();
}

}  // namespace G4Cosmic
