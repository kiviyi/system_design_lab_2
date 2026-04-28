#include "fitness_storage.hpp"

#include <algorithm>
#include <utility>

#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/uuid4.hpp>

#include "api_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace fitness_tracker {

FitnessStorage::FitnessStorage(const components::ComponentConfig& config, const components::ComponentContext& context)
    : LoggableComponentBase(config, context) {}

User FitnessStorage::CreateUser(
    const std::string& username,
    const std::string& first_name,
    const std::string& last_name,
    const std::string& email,
    const std::string& password
) {
  ValidateUser(username, first_name, last_name, email, password);

  std::lock_guard lock(mutex_);
  for (const auto& [_, user] : users_by_id_) {
    if (user.username == username) {
      throw ApiException(server::http::HttpStatus::kConflict, "Username already exists");
    }
    if (user.email == email) {
      throw ApiException(server::http::HttpStatus::kConflict, "Email already exists");
    }
  }

  User user{
      .id = utils::generators::GenerateUuid(),
      .username = username,
      .first_name = first_name,
      .last_name = last_name,
      .email = email,
      .hashed_password = HashPassword(password),
  };
  users_by_id_.emplace(user.id, user);
  user_ids_by_username_[user.username] = user.id;
  return user;
}

std::string FitnessStorage::Login(const std::string& username, const std::string& password) {
  std::lock_guard lock(mutex_);
  const auto user_id_it = user_ids_by_username_.find(username);
  if (user_id_it == user_ids_by_username_.end()) {
    throw ApiException(server::http::HttpStatus::kUnauthorized, "Incorrect username or password");
  }

  const auto& user = users_by_id_.at(user_id_it->second);
  if (user.hashed_password != HashPassword(password)) {
    throw ApiException(server::http::HttpStatus::kUnauthorized, "Incorrect username or password");
  }

  const auto token = utils::generators::GenerateUuid();
  active_tokens_[token] = user.username;
  return token;
}

std::string FitnessStorage::Authorize(const server::http::HttpRequest& request) const {
  const auto header = request.GetHeader("Authorization");
  if (!header.starts_with(kBearerPrefix)) {
    throw ApiException(server::http::HttpStatus::kUnauthorized, "Could not validate credentials");
  }

  const auto token = std::string{header.substr(kBearerPrefix.size())};
  std::lock_guard lock(mutex_);
  const auto token_it = active_tokens_.find(token);
  if (token_it == active_tokens_.end()) {
    throw ApiException(server::http::HttpStatus::kUnauthorized, "Could not validate credentials");
  }
  return token_it->second;
}

std::vector<User> FitnessStorage::SearchUsers(
    const std::optional<std::string>& username,
    const std::optional<std::string>& first_name,
    const std::optional<std::string>& last_name
) const {
  if (!username && !first_name && !last_name) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Provide at least one search parameter");
  }

  std::vector<User> result;
  std::lock_guard lock(mutex_);
  for (const auto& [_, user] : users_by_id_) {
    if (username && user.username == *username) {
      result.push_back(user);
    } else if (first_name && last_name) {
      if (WildcardMatch(*first_name, user.first_name) && WildcardMatch(*last_name, user.last_name)) {
        result.push_back(user);
      }
    } else if (first_name && WildcardMatch(*first_name, user.first_name)) {
      result.push_back(user);
    } else if (last_name && WildcardMatch(*last_name, user.last_name)) {
      result.push_back(user);
    }
  }
  return result;
}

Exercise FitnessStorage::CreateExercise(const std::string& name, const std::string& description, const std::string& muscle_group) {
  if (name.empty() || name.size() > 200) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Exercise name must be between 1 and 200 characters");
  }
  if (description.size() > 1000) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Exercise description cannot exceed 1000 characters");
  }
  if (muscle_group.empty() || muscle_group.size() > 100) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Muscle group must be between 1 and 100 characters");
  }

  Exercise exercise{
      .id = utils::generators::GenerateUuid(),
      .name = name,
      .description = description,
      .muscle_group = muscle_group,
  };

  std::lock_guard lock(mutex_);
  exercises_by_id_.emplace(exercise.id, exercise);
  return exercise;
}

std::vector<Exercise> FitnessStorage::GetExercises() const {
  std::vector<Exercise> result;
  std::lock_guard lock(mutex_);
  result.reserve(exercises_by_id_.size());
  for (const auto& [_, exercise] : exercises_by_id_) {
    result.push_back(exercise);
  }
  return result;
}

Workout FitnessStorage::CreateWorkout(
    const std::string& user_id,
    const std::string& name,
    const std::optional<std::string>& date
) {
  if (name.empty() || name.size() > 200) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Workout name must be between 1 and 200 characters");
  }

  std::lock_guard lock(mutex_);
  EnsureUserExistsLocked(user_id);

  Workout workout{
      .id = utils::generators::GenerateUuid(),
      .user_id = user_id,
      .name = name,
      .date = date.value_or(CurrentDateIso()),
      .exercises = {},
  };
  workouts_by_id_.emplace(workout.id, workout);
  return workout;
}

Workout FitnessStorage::AddExerciseToWorkout(
    const std::string& user_id,
    const std::string& workout_id,
    const std::string& exercise_id,
    int sets,
    int reps,
    double weight
) {
  if (sets < 1 || sets > 100) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Sets must be between 1 and 100");
  }
  if (reps < 1 || reps > 1000) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Reps must be between 1 and 1000");
  }
  if (weight < 0) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Weight cannot be negative");
  }

  std::lock_guard lock(mutex_);
  EnsureUserExistsLocked(user_id);

  const auto workout_it = workouts_by_id_.find(workout_id);
  if (workout_it == workouts_by_id_.end()) {
    throw ApiException(server::http::HttpStatus::kNotFound, "Workout not found");
  }

  const auto exercise_it = exercises_by_id_.find(exercise_id);
  if (exercise_it == exercises_by_id_.end()) {
    throw ApiException(server::http::HttpStatus::kNotFound, "Exercise not found");
  }

  auto& workout = workout_it->second;
  if (workout.user_id != user_id) {
    throw ApiException(server::http::HttpStatus::kForbidden, "Workout does not belong to this user");
  }

  workout.exercises.push_back(WorkoutExercise{
      .exercise_id = exercise_id,
      .exercise_name = exercise_it->second.name,
      .sets = sets,
      .reps = reps,
      .weight = weight,
  });
  return workout;
}

std::vector<Workout> FitnessStorage::GetUserWorkouts(const std::string& user_id) const {
  std::vector<Workout> result;
  std::lock_guard lock(mutex_);
  EnsureUserExistsLocked(user_id);

  for (const auto& [_, workout] : workouts_by_id_) {
    if (workout.user_id == user_id) {
      result.push_back(workout);
    }
  }

  std::sort(result.begin(), result.end(), [](const Workout& lhs, const Workout& rhs) {
    return lhs.date > rhs.date;
  });
  return result;
}

formats::json::Value FitnessStorage::GetWorkoutStats(
    const std::string& user_id,
    const std::string& start_date,
    const std::string& end_date
) const {
  std::lock_guard lock(mutex_);
  EnsureUserExistsLocked(user_id);

  int total_workouts = 0;
  int total_exercises = 0;
  int total_sets = 0;
  int total_reps = 0;
  formats::json::ValueBuilder workouts_builder(formats::common::Type::kArray);

  for (const auto& [_, workout] : workouts_by_id_) {
    if (workout.user_id != user_id || workout.date < start_date || workout.date > end_date) {
      continue;
    }

    ++total_workouts;
    workouts_builder.PushBack(WorkoutToJson(workout));
    total_exercises += static_cast<int>(workout.exercises.size());
    for (const auto& exercise : workout.exercises) {
      total_sets += exercise.sets;
      total_reps += exercise.sets * exercise.reps;
    }
  }

  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["user_id"] = user_id;
  builder["start_date"] = start_date;
  builder["end_date"] = end_date;
  builder["total_workouts"] = total_workouts;
  builder["total_exercises"] = total_exercises;
  builder["total_sets"] = total_sets;
  builder["total_reps"] = total_reps;
  builder["workouts"] = workouts_builder.ExtractValue();
  return builder.ExtractValue();
}

formats::json::Value FitnessStorage::UserToJson(const User& user) const {
  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["id"] = user.id;
  builder["username"] = user.username;
  builder["first_name"] = user.first_name;
  builder["last_name"] = user.last_name;
  builder["email"] = user.email;
  return builder.ExtractValue();
}

formats::json::Value FitnessStorage::ExerciseToJson(const Exercise& exercise) const {
  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["id"] = exercise.id;
  builder["name"] = exercise.name;
  builder["description"] = exercise.description;
  builder["muscle_group"] = exercise.muscle_group;
  return builder.ExtractValue();
}

formats::json::Value FitnessStorage::WorkoutToJson(const Workout& workout) const {
  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["id"] = workout.id;
  builder["user_id"] = workout.user_id;
  builder["name"] = workout.name;
  builder["date"] = workout.date;

  formats::json::ValueBuilder exercises_builder(formats::common::Type::kArray);
  for (const auto& exercise : workout.exercises) {
    formats::json::ValueBuilder exercise_builder(formats::common::Type::kObject);
    exercise_builder["exercise_id"] = exercise.exercise_id;
    exercise_builder["exercise_name"] = exercise.exercise_name;
    exercise_builder["sets"] = exercise.sets;
    exercise_builder["reps"] = exercise.reps;
    exercise_builder["weight"] = exercise.weight;
    exercises_builder.PushBack(exercise_builder.ExtractValue());
  }

  builder["exercises"] = exercises_builder.ExtractValue();
  return builder.ExtractValue();
}

void FitnessStorage::ValidateUser(
    const std::string& username,
    const std::string& first_name,
    const std::string& last_name,
    const std::string& email,
    const std::string& password
) const {
  if (username.size() < 3 || username.size() > 50) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Username must be between 3 and 50 characters");
  }
  if (first_name.empty() || first_name.size() > 100) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "First name must be between 1 and 100 characters");
  }
  if (last_name.empty() || last_name.size() > 100) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Last name must be between 1 and 100 characters");
  }
  if (!IsValidEmail(email)) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Invalid email");
  }
  if (password.size() < 6) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Password must be at least 6 characters");
  }
}

void FitnessStorage::EnsureUserExistsLocked(const std::string& user_id) const {
  if (!users_by_id_.contains(user_id)) {
    throw ApiException(server::http::HttpStatus::kNotFound, "User not found");
  }
}

}  // namespace fitness_tracker

USERVER_NAMESPACE_END
