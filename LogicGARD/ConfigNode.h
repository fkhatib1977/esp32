#pragma once
#include <ArduinoJson.h>
#include <string>
#include <vector>
#include "BaseComponent.h"

class ConfigNode : public BaseComponent {
public:
  ConfigNode(JsonVariantConst variant) : data(variant) {}

  template<typename T>
  T get(const std::string& path) const {
    JsonVariantConst v = traverse(path);
    if (v.isNull()) {
      abort();
    }
    if (!v.is<T>()) {
      abort();
    }
    return v.as<T>();
  }

  std::vector<ConfigNode> getArray(const std::string& path) const {
    std::vector<ConfigNode> result;
    JsonVariantConst v = traverse(path);
    if (v.is<JsonArrayConst>()) {
      for (JsonVariantConst item : v.as<JsonArrayConst>()) {
        result.emplace_back(item);
      }
    }
    return result;
  }

  bool has(const std::string& path) const {
    JsonVariantConst v = traverse(path);
    return !v.isNull();
  }

  ConfigNode getNode(const std::string& path) const {
    return ConfigNode(traverse(path));
  }

  ConfigNode getChild(const std::string& key) const {
    return ConfigNode(data[key]);
  }

  JsonVariantConst getRaw() const {
    return data;
  }

private:
  JsonVariantConst data;

  JsonVariantConst traverse(const std::string& path) const {
    JsonVariantConst current = data;
    size_t start = 0, end;
    while ((end = path.find('.', start)) != std::string::npos) {
      std::string key = path.substr(start, end - start);
      current = current[key];
      if (current.isNull()) return JsonVariantConst();
      start = end + 1;
    }
    std::string finalKey = path.substr(start);
    return current[finalKey];
  }

  template<typename T>
  const char* typeName() const {
    if (std::is_same<T, std::string>::value || std::is_same<T, String>::value) return "string";
    if (std::is_same<T, bool>::value) return "bool";
    if (std::is_same<T, int>::value || std::is_same<T, int32_t>::value) return "int";
    if (std::is_same<T, uint8_t>::value) return "uint8_t";
    if (std::is_same<T, uint32_t>::value) return "uint32_t";
    if (std::is_same<T, float>::value) return "float";
    return "unknown";
  }
};