#pragma once

#include <functional>
#include <string>
#include <string_view>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

#include "fitness_storage.hpp"

USERVER_NAMESPACE_BEGIN

namespace fitness_tracker {

class HandlerBase : public server::handlers::HttpHandlerBase {
 public:
  HandlerBase(const components::ComponentConfig& config, const components::ComponentContext& context);

 protected:
  std::string Execute(
      const server::http::HttpRequest& request,
      const std::function<formats::json::Value()>& action
  ) const;

  FitnessStorage& storage_;
};

class RegisterHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-register";

  RegisterHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&
  ) const override;

 private:
  FitnessStorage& storage_;
};

class TokenHandler final : public HandlerBase {
 public:
  static constexpr std::string_view kName = "handler-token";

  using HandlerBase::HandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class CreateUserHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-users-create";

  CreateUserHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&
  ) const override;

 private:
  FitnessStorage& storage_;
};

class SearchUsersHandler final : public HandlerBase {
 public:
  static constexpr std::string_view kName = "handler-users-search";

  using HandlerBase::HandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class CreateExerciseHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-exercises-create";

  CreateExerciseHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&
  ) const override;

 private:
  FitnessStorage& storage_;
};

class GetExercisesHandler final : public HandlerBase {
 public:
  static constexpr std::string_view kName = "handler-exercises-list";

  using HandlerBase::HandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class CreateWorkoutHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-workouts-create";

  CreateWorkoutHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&
  ) const override;

 private:
  FitnessStorage& storage_;
};

class AddExerciseToWorkoutHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-workouts-add-exercise";

  AddExerciseToWorkoutHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&
  ) const override;

 private:
  FitnessStorage& storage_;
};

class GetUserWorkoutsHandler final : public HandlerBase {
 public:
  static constexpr std::string_view kName = "handler-workouts-list";

  using HandlerBase::HandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

class GetWorkoutStatsHandler final : public HandlerBase {
 public:
  static constexpr std::string_view kName = "handler-workouts-stats";

  using HandlerBase::HandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&
  ) const override;
};

components::ComponentList MakeFitnessComponentList();

}  // namespace fitness_tracker

USERVER_NAMESPACE_END
