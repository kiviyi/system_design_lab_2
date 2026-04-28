#pragma once

#include <string>
#include <vector>

#include <userver/components/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace fitness_tracker {

struct User {
  std::string id;
  std::string username;
  std::string first_name;
  std::string last_name;
  std::string email;
  std::string hashed_password;
};

struct Exercise {
  std::string id;
  std::string name;
  std::string description;
  std::string muscle_group;
};

struct WorkoutExercise {
  std::string exercise_id;
  std::string exercise_name;
  int sets;
  int reps;
  double weight;
};

struct Workout {
  std::string id;
  std::string user_id;
  std::string name;
  std::string date;
  std::vector<WorkoutExercise> exercises;
};

}  // namespace fitness_tracker

USERVER_NAMESPACE_END
