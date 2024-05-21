// ============================================================================
// Copyright (C) 2022 Xiaomi EV Company Limited. All rights reserved.
//
// XIAOMI CONFIDENTIAL
//
// Xiaomi EV Company Limited retains all intellectual property and proprietary
// rights in and to this software and related documentation and any
// modifications thereto ("Software"). Any reproduction, disclosure or
// distribution of this Software is strictly prohibited.
//
// Permitted Users may only access, use or edit the Software based on their work
// duties. Permitted Users shall refer to the current employees in Xiaomi EV
// Autonomous Driving Group who have entered into Non-disclosure Agreement with
// Xiaomi, and have been duly authorized to access the Software.
// ============================================================================
#ifndef OBSTACLE_CODE_PATH_REPORT_H_
#define OBSTACLE_CODE_PATH_REPORT_H_

#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>


#include "obstacle_code_path_report.pb.h"

namespace common{

class ConcreteCodeBlockExecution
    : public std::enable_shared_from_this<ConcreteCodeBlockExecution> {
 public:
  using CodeBlockExecutionPb =
      proto::CodeBlockExecution;

  ConcreteCodeBlockExecution() = default;
  explicit ConcreteCodeBlockExecution(const std::string& obstacle_id,
                             const std::string& name,
                             const bool is_code_block_conditional)
      : obstacle_id_(obstacle_id),
        name_(name),
        is_code_block_conditional_(is_code_block_conditional) {}
  ~ConcreteCodeBlockExecution() {
    details_.clear();
    std::vector<std::string>().swap(details_);
    child_executions_.clear();
    std::list<ConcreteCodeBlockExecution>().swap(child_executions_);
  }

  bool set_code_block_condition_satisfied(bool code_block_condition_satisfied);

  std::string name() const { return name_; }

  bool has_detail() const { return !details_.empty(); }
  ConcreteCodeBlockExecution* AddDetail(const std::string& detail_to_add);
  std::string DetailString() const;

  ConcreteCodeBlockExecution* parent_execution();
  void set_parent_execution(ConcreteCodeBlockExecution* parent_execution);

  ConcreteCodeBlockExecution* AddChildExecution(const std::string& obstacle_id,
                                                const std::string& name,
                                                bool is_code_block_conditional);

  void SerializeToPb(CodeBlockExecutionPb* const code_block_execution_pb) const;

  std::string ExecutionReportText(int indent_level) const;

  std::string obstacle_id() const { return obstacle_id_; }
  void set_obstacle_id(const std::string& obstacle_id) {
    obstacle_id_ = obstacle_id;
  }
  double start_time_ms = 0.0;
  double end_time_ms = 0.0;
  double duration_ms() const { return end_time_ms - start_time_ms; }

 private:
  std::string obstacle_id_;
  std::string name_;
  bool is_code_block_conditional_ = false;
  bool code_block_condition_satisfied_ = false;
  std::vector<std::string> details_;
  ConcreteCodeBlockExecution* parent_execution_ = nullptr;
  std::list<ConcreteCodeBlockExecution> child_executions_;
};

class CodeBlockExecution {
 public:
  CodeBlockExecution() = default;
  explicit CodeBlockExecution(ConcreteCodeBlockExecution& code_block_execution);
  ~CodeBlockExecution();

  static CodeBlockExecution* LastActiveExecution(const std::string& obs_id);

  CodeBlockExecution AddChildExecution(const std::string& name,
                                       bool is_code_block_conditional);

  bool set_code_block_condition_satisfied(
      const bool code_block_condition_satisfied);

  ConcreteCodeBlockExecution* AddDetail(const std::string& detail);

 private:
  ConcreteCodeBlockExecution* concrete_code_block_execution_ = nullptr;
};

class ObstacleCodePathReport {
 public:
  using ObstacleCodePathReportPb =
      proto::ObstacleCodePathReport;
  using CodeBlockExecutionPb =
      proto::CodeBlockExecution;
  using ObstacleWiseRootExecutionsPb =
      google::protobuf::Map<std::string, CodeBlockExecutionPb>;

  std::string name() const { return name_; }
  void set_name(const std::string name) { name_ = name; }

  std::string ReportString() const;

  ConcreteCodeBlockExecution* GetOrCreateRootExecution(
      const std::string& obs_id);

  void SerializeToPb(
      ObstacleCodePathReportPb* const obstacle_code_path_report_pb) const;
  void Clear();

  std::map<std::string, CodeBlockExecution> obs_id_to_last_active_execution_;
  std::map<std::string, ConcreteCodeBlockExecution>
      obstacle_wise_root_executions_;

  static ObstacleCodePathReport* Instance();

  ObstacleCodePathReport();
  static void ChooseReport(const std::string& report_name);
  static std::vector<std::string> report_names();
  static void ClearReports();
  static std::string current_report_name();
  static std::shared_ptr<std::string> current_report_name_;

 private:
  static std::map<std::string, ObstacleCodePathReport> report_map_;

  std::string name_;
};
}

#endif 
