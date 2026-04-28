#include "api_utils.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <functional>
#include <iomanip>
#include <regex>
#include <sstream>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace fitness_tracker {

namespace {

std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

}  // namespace

ApiException::ApiException(server::http::HttpStatus status, std::string detail)
    : std::runtime_error(std::move(detail)), status_(status) {}

server::http::HttpStatus ApiException::GetStatus() const noexcept {
  return status_;
}

std::string HashPassword(const std::string& password) {
  return std::to_string(std::hash<std::string>{}(password));
}

std::string CurrentDateIso() {
  const auto now = std::chrono::system_clock::now();
  const auto now_time_t = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#ifdef _WIN32
  gmtime_s(&tm, &now_time_t);
#else
  gmtime_r(&now_time_t, &tm);
#endif

  std::ostringstream out;
  out << std::put_time(&tm, "%Y-%m-%d");
  return out.str();
}

bool IsValidEmail(const std::string& email) {
  static const std::regex kEmailPattern(R"(^[\w\.-]+@[\w\.-]+\.\w+$)");
  return std::regex_match(email, kEmailPattern);
}

bool WildcardMatch(std::string pattern, std::string value) {
  pattern = ToLower(std::move(pattern));
  value = ToLower(std::move(value));

  std::string regex_pattern = "^";
  for (char ch : pattern) {
    switch (ch) {
      case '*':
        regex_pattern += ".*";
        break;
      case '.':
      case '\\':
      case '+':
      case '?':
      case '^':
      case '$':
      case '(':
      case ')':
      case '[':
      case ']':
      case '{':
      case '}':
      case '|':
        regex_pattern.push_back('\\');
        regex_pattern.push_back(ch);
        break;
      default:
        regex_pattern.push_back(ch);
        break;
    }
  }
  regex_pattern += "$";
  return std::regex_match(value, std::regex(regex_pattern));
}

std::string UrlDecode(std::string_view value) {
  std::string result;
  result.reserve(value.size());

  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '+') {
      result.push_back(' ');
    } else if (value[i] == '%' && i + 2 < value.size()) {
      const auto hex = std::string{value.substr(i + 1, 2)};
      result.push_back(static_cast<char>(std::stoi(hex, nullptr, 16)));
      i += 2;
    } else {
      result.push_back(value[i]);
    }
  }

  return result;
}

std::unordered_map<std::string, std::string> ParseFormUrlEncoded(std::string_view body) {
  std::unordered_map<std::string, std::string> result;
  std::size_t start = 0;

  while (start <= body.size()) {
    const auto end = body.find('&', start);
    const auto token = body.substr(start, end == std::string_view::npos ? body.size() - start : end - start);
    const auto separator = token.find('=');
    if (separator != std::string_view::npos) {
      result.emplace(UrlDecode(token.substr(0, separator)), UrlDecode(token.substr(separator + 1)));
    }

    if (end == std::string_view::npos) break;
    start = end + 1;
  }

  return result;
}

formats::json::Value ErrorJson(std::string_view detail) {
  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["detail"] = std::string{detail};
  return builder.ExtractValue();
}

std::optional<std::string> OptionalString(const formats::json::Value& json, const std::string& key) {
  if (!json.HasMember(key)) return std::nullopt;
  return json[key].As<std::string>();
}

}  // namespace fitness_tracker

USERVER_NAMESPACE_END
