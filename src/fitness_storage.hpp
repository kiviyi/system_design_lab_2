#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <userver/components/loggable_component_base.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/server/http/http_request.hpp>

#include "api_types.hpp"

USERVER_NAMESPACE_BEGIN

namespace fitness_tracker {

class FitnessStorage final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "fitness-storage";

  FitnessStorage(const components::ComponentConfig& config, const components::ComponentContext& context);

  User CreateUser(
      const std::string& username,
      const std::string& first_name,
      const std::string& last_name,
      const std::string& email,
      const std::string& password
  );

  std::string Login(const std::string& username, const std::string& password);
  std::string Authorize(const server::http::HttpRequest& request) const;

  std::vector<User> SearchUsers(
      const std::optional<std::string>& username,
      const std::optional<std::string>& first_name,
      const std::optional<std::string>& last_name
  ) const;

  Exercise CreateExercise(const std::string& name, const std::string& description, const std::string& muscle_group);
  std::vector<Exercise> GetExercises() const;

  Workout CreateWorkout(const std::string& user_id, const std::string& name, const std::optional<std::string>& date);
  Workout AddExerciseToWorkout(
      const std::string& user_id,
      const std::string& workout_id,
      const std::string& exercise_id,
      int sets,
      int reps,
      double weight
  );
  std::vector<Workout> GetUserWorkouts(const std::string& user_id) const;
  formats::json::Value GetWorkoutStats(
      const std::string& user_id,
      const std::string& start_date,
      const std::string& end_date
  ) const;

  formats::json::Value UserToJson(const User& user) const;
  formats::json::Value ExerciseToJson(const Exercise& exercise) const;
  formats::json::Value WorkoutToJson(const Workout& workout) const;

 private:
  void ValidateUser(
      const std::string& username,
      const std::string& first_name,
      const std::string& last_name,
      const std::string& email,
      const std::string& password
  ) const;
  void EnsureUserExistsLocked(const std::string& user_id) const;

  mutable std::mutex mutex_;
  std::unordered_map<std::string, User> users_by_id_;
  std::unordered_map<std::string, std::string> user_ids_by_username_;
  std::unordered_map<std::string, Exercise> exercises_by_id_;
  std::unordered_map<std::string, Workout> workouts_by_id_;
  std::unordered_map<std::string, std::string> active_tokens_;
};

}  // namespace fitness_tracker

USERVER_NAMESPACE_END
