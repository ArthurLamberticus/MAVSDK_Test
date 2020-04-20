//Recup rotor pwn

#include <chrono>
#include <cstdint>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <iostream>
#include <thread>
#include <future>

using namespace mavsdk;

#define RATE_POS 1.0


void handle_error(bool success, std::string str)
{
  if (!success) {
    std::cerr << "Error : " << str << std::endl;
    exit(1);
  }
}

void suscribe_to_infos(std::shared_ptr<Telemetry> telemetry)
{
  const Telemetry::Result set_rate_pos_result =
    telemetry->set_rate_position(1.0);
  handle_error(set_rate_pos_result == Telemetry::Result::SUCCESS,
               "set_rate_pos_result");

  auto prom = std::make_shared<std::promise<Telemetry::Result>>();
  auto set_rate_aos_future = prom->get_future();
  telemetry->set_rate_actuator_output_status_async(3.0,
  	     [prom] (Telemetry::Result result)
  	     {
  	       prom->set_value(result);
  	     });
  const Telemetry::Result set_rate_aos_result = set_rate_aos_future.get();
  if (set_rate_aos_result != Telemetry::Result::SUCCESS) {
    std::cout << Telemetry::result_str(set_rate_aos_result) << std::endl;
  }
  handle_error(set_rate_aos_result == Telemetry::Result::SUCCESS,
	       "set_rate_aos_result");

  telemetry->actuator_output_status_async([] (Telemetry::ActuatorOutputStatus aos) {
      std::cout << "AOS active : " << aos.active << std::endl;
      for (int i = 0; i < 32; i++) {
	std::cout << "AOS " << i << " : " << aos.actuator[i] << std::endl;
      }
    });
  Telemetry::ActuatorOutputStatus aos = telemetry->actuator_output_status();
  std::cout << "AOS active : " << aos.active << std::endl;
  std::cout << "info set" << std::endl;
}

void arm_drone(std::shared_ptr<Action> action,
	       std::shared_ptr<Telemetry> telemetry)
{
  while (telemetry->health_all_ok() != true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  const Action::Result arm_result = action->arm();
  handle_error(arm_result == Action::Result::SUCCESS, "Arm failed");
}

void do_some_actions(std::shared_ptr<Action> action,
		     std::shared_ptr<Telemetry> telemetry)
{
  auto prom = std::make_shared<std::promise<void>>();
  auto prom_takeoff = std::make_shared<std::promise<Action::Result>>();
  auto future_alt = prom->get_future();
  auto future_result = prom_takeoff->get_future();

  std::this_thread::sleep_for(std::chrono::seconds(1));
  action->set_takeoff_altitude(10.0);
  telemetry->position_async([prom](Telemetry::Position position) {
      std::cout << "Alt :" << position.relative_altitude_m
		<< std::endl;
      if (position.relative_altitude_m > 9.5) {
	prom->set_value();
      }
    });
  action->takeoff_async([prom_takeoff](Action::Result result) {     
      std::cout << "takeoff" << std::endl;
      prom_takeoff->set_value(result);
    });
  const Action::Result takeoff_result = future_result.get();
  handle_error(takeoff_result == Action::Result::SUCCESS, "takeoff cmd");
  future_alt.get();

  Telemetry::ActuatorOutputStatus aos =	telemetry->actuator_output_status();
  for (int i = 0; i < 32; i++) {
    std::cout << "AOS " << i << " : " << aos.actuator[i] << std::endl;
  }
  
  const Action::Result land_result = action->land();
  handle_error(land_result == Action::Result::SUCCESS, "land cmd");
  while (telemetry->armed()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void monitor_drone(Mavsdk& dc)
{
  System& system = dc.system();
  auto telemetry = std::make_shared<Telemetry>(system);
  auto action = std::make_shared<Action>(system);

  suscribe_to_infos(telemetry);
  arm_drone(action, telemetry);
  do_some_actions(action, telemetry);
}

int main(int ac, char** av)
{
  Mavsdk dc;
  ConnectionResult connection_result;

  connection_result = dc.add_any_connection("udp://0.0.0.0:14540");
  handle_error(connection_result == ConnectionResult::SUCCESS,
	       "Connection");
  while (!dc.is_connected()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  std::cout << "Connected" << std::endl;
  monitor_drone(dc);
  return 0;
}
