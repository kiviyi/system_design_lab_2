#include "handlers.hpp"

#include <optional>
#include <string>

#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>

#include "api_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace fitness_tracker {

HandlerBase::HandlerBase(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerBase(config, context), storage_(context.FindComponent<FitnessStorage>()) {}

std::string HandlerBase::Execute(
    const server::http::HttpRequest& request,
    const std::function<formats::json::Value()>& action
) const {
  try {
    return formats::json::ToString(action());
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return formats::json::ToString(ErrorJson(ex.what()));
  } catch (const std::exception& ex) {
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kBadRequest);
    return formats::json::ToString(ErrorJson(ex.what()));
  }
}

RegisterHandler::RegisterHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerJsonBase(config, context), storage_(context.FindComponent<FitnessStorage>()) {}

formats::json::Value RegisterHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    const auto user = storage_.CreateUser(
        RequireField<std::string>(request_json, "username"),
        RequireField<std::string>(request_json, "first_name"),
        RequireField<std::string>(request_json, "last_name"),
        RequireField<std::string>(request_json, "email"),
        RequireField<std::string>(request_json, "password")
    );
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kCreated);
    return storage_.UserToJson(user);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

std::string TokenHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    const auto form = ParseFormUrlEncoded(request.RequestBody());
    const auto username_it = form.find("username");
    const auto password_it = form.find("password");
    if (username_it == form.end() || password_it == form.end()) {
      throw ApiException(server::http::HttpStatus::kBadRequest, "username and password are required");
    }

    formats::json::ValueBuilder builder(formats::common::Type::kObject);
    builder["access_token"] = storage_.Login(username_it->second, password_it->second);
    builder["token_type"] = std::string{kTokenType};
    return builder.ExtractValue();
  });
}

CreateUserHandler::CreateUserHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : HttpHandlerJsonBase(config, context), storage_(context.FindComponent<FitnessStorage>()) {}

formats::json::Value CreateUserHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    const auto user = storage_.CreateUser(
        RequireField<std::string>(request_json, "username"),
        RequireField<std::string>(request_json, "first_name"),
        RequireField<std::string>(request_json, "last_name"),
        RequireField<std::string>(request_json, "email"),
        RequireField<std::string>(request_json, "password")
    );
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kCreated);
    return storage_.UserToJson(user);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

std::string SearchUsersHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    formats::json::ValueBuilder builder(formats::common::Type::kArray);
    const auto users = storage_.SearchUsers(
        request.HasArg("username") ? std::make_optional(request.GetArg("username")) : std::nullopt,
        request.HasArg("first_name") ? std::make_optional(request.GetArg("first_name")) : std::nullopt,
        request.HasArg("last_name") ? std::make_optional(request.GetArg("last_name")) : std::nullopt
    );
    for (const auto& user : users) {
      builder.PushBack(storage_.UserToJson(user));
    }
    return builder.ExtractValue();
  });
}

CreateExerciseHandler::CreateExerciseHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : HttpHandlerJsonBase(config, context), storage_(context.FindComponent<FitnessStorage>()) {}

formats::json::Value CreateExerciseHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    storage_.Authorize(request);
    const auto exercise = storage_.CreateExercise(
        RequireField<std::string>(request_json, "name"),
        OptionalString(request_json, "description").value_or(""),
        RequireField<std::string>(request_json, "muscle_group")
    );
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kCreated);
    return storage_.ExerciseToJson(exercise);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

std::string GetExercisesHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    formats::json::ValueBuilder builder(formats::common::Type::kArray);
    for (const auto& exercise : storage_.GetExercises()) {
      builder.PushBack(storage_.ExerciseToJson(exercise));
    }
    return builder.ExtractValue();
  });
}

CreateWorkoutHandler::CreateWorkoutHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : HttpHandlerJsonBase(config, context), storage_(context.FindComponent<FitnessStorage>()) {}

formats::json::Value CreateWorkoutHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    storage_.Authorize(request);
    const auto workout = storage_.CreateWorkout(
        request.GetPathArg("user_id"),
        RequireField<std::string>(request_json, "name"),
        OptionalString(request_json, "date")
    );
    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kCreated);
    return storage_.WorkoutToJson(workout);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

AddExerciseToWorkoutHandler::AddExerciseToWorkoutHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : HttpHandlerJsonBase(config, context), storage_(context.FindComponent<FitnessStorage>()) {}

formats::json::Value AddExerciseToWorkoutHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    storage_.Authorize(request);
    const auto workout = storage_.AddExerciseToWorkout(
        request.GetPathArg("user_id"),
        request.GetPathArg("workout_id"),
        RequireField<std::string>(request_json, "exercise_id"),
        RequireField<int>(request_json, "sets"),
        RequireField<int>(request_json, "reps"),
        request_json.HasMember("weight") ? request_json["weight"].As<double>() : 0.0
    );
    return storage_.WorkoutToJson(workout);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

std::string GetUserWorkoutsHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    storage_.Authorize(request);
    formats::json::ValueBuilder builder(formats::common::Type::kArray);
    for (const auto& workout : storage_.GetUserWorkouts(request.GetPathArg("user_id"))) {
      builder.PushBack(storage_.WorkoutToJson(workout));
    }
    return builder.ExtractValue();
  });
}

std::string GetWorkoutStatsHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    storage_.Authorize(request);
    if (!request.HasArg("start_date") || !request.HasArg("end_date")) {
      throw ApiException(server::http::HttpStatus::kBadRequest, "start_date and end_date are required");
    }
    return storage_.GetWorkoutStats(
        request.GetPathArg("user_id"),
        request.GetArg("start_date"),
        request.GetArg("end_date")
    );
  });
}

components::ComponentList MakeFitnessComponentList() {
  return components::MinimalServerComponentList()
      .Append<FitnessStorage>()
      .Append<RegisterHandler>()
      .Append<TokenHandler>()
      .Append<CreateUserHandler>()
      .Append<SearchUsersHandler>()
      .Append<CreateExerciseHandler>()
      .Append<GetExercisesHandler>()
      .Append<CreateWorkoutHandler>()
      .Append<AddExerciseToWorkoutHandler>()
      .Append<GetUserWorkoutsHandler>()
      .Append<GetWorkoutStatsHandler>();
}

}  // namespace fitness_tracker

USERVER_NAMESPACE_END
