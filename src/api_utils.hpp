#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace fitness_tracker {

constexpr std::string_view kBearerPrefix = "Bearer ";
constexpr std::string_view kTokenType = "bearer";

class ApiException : public std::runtime_error {
 public:
  ApiException(server::http::HttpStatus status, std::string detail);

  server::http::HttpStatus GetStatus() const noexcept;

 private:
  server::http::HttpStatus status_;
};

std::string HashPassword(const std::string& password);
std::string CurrentDateIso();
bool IsValidEmail(const std::string& email);
bool WildcardMatch(std::string pattern, std::string value);
std::string UrlDecode(std::string_view value);
std::unordered_map<std::string, std::string> ParseFormUrlEncoded(std::string_view body);
formats::json::Value ErrorJson(std::string_view detail);
std::optional<std::string> OptionalString(const formats::json::Value& json, const std::string& key);

template <typename T>
T RequireField(const formats::json::Value& json, const std::string& key) {
  if (!json.HasMember(key)) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Missing field: " + key);
  }
  return json[key].As<T>();
}

}  // namespace fitness_tracker

USERVER_NAMESPACE_END
