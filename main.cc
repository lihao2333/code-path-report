#include "obstacle_code_path_report.h"
#include <iostream>

class Obstacle {
public:
  Obstacle(const std::string &name) : name_(name) {}
  void Sleep(int time) {
    common::CodeBlockExecution execution =
        common::CodeBlockExecution::LastActiveExecution(name_)
            ->AddChildExecution("Sleep", false);
    execution.AddDetail(std::to_string(time));
  }
  void Walk() {
    common::CodeBlockExecution execution =
        common::CodeBlockExecution::LastActiveExecution(name_)
            ->AddChildExecution("Walk", false);
    Sleep(2);
  }

private:
  std::string name_ = "";
};

int main() {
  common::ObstacleCodePathReport::ClearReports();
  common::ObstacleCodePathReport::ChooseReport("obstacle_map");

  Obstacle obstacle_1("obs_1");
  Obstacle obstacle_2("obs_2");

  common::CodeBlockExecution execution =
      common::CodeBlockExecution::LastActiveExecution("main")
          ->AddChildExecution("main", true);
  obstacle_1.Sleep(10);
  obstacle_2.Sleep(20);
  obstacle_1.Walk();
  execution.set_code_block_condition_satisfied(true);

  proto::ObstacleCodePathReport obstacle_code_path_report_pb;
  common::ObstacleCodePathReport::Instance()->SerializeToPb(
      &obstacle_code_path_report_pb);
  std::cout << obstacle_code_path_report_pb.DebugString();

  return 0;
}