#include <userver/utils/daemon_run.hpp>

#include "handlers.hpp"

int main(int argc, char* argv[]) {
  return USERVER_NAMESPACE::utils::DaemonMain(
      argc,
      argv,
      USERVER_NAMESPACE::fitness_tracker::MakeFitnessComponentList()
  );
}
