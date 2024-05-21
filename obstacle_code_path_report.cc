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
#include "obstacle_code_path_report.h"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>


namespace common{

// ObstacleCodePathReport.
void ObstacleCodePathReport::ChooseReport(const std::string &report_name) {
  current_report_name_ = std::make_shared<std::string>(report_name);
}

ObstacleCodePathReport *ObstacleCodePathReport::Instance() {
  if (report_map_.find(current_report_name()) == report_map_.end()) {
    report_map_[current_report_name()] = ObstacleCodePathReport();
    report_map_[current_report_name()].set_name(current_report_name());
  }
  return &report_map_[current_report_name()];
}

std::vector<std::string> ObstacleCodePathReport::report_names() {
  std::vector<std::string> report_names;
  for (auto it = report_map_.begin(); it != report_map_.end(); it++) {
    report_names.push_back(it->first);
  }
  return report_names;
}

void ObstacleCodePathReport::ClearReports() {
  for (auto it = report_map_.begin(); it != report_map_.end(); it++) {
    it->second.Clear();
  }
  report_map_.clear();
  std::map<std::string, ObstacleCodePathReport>().swap(report_map_);
}

std::map<std::string, ObstacleCodePathReport>
    ObstacleCodePathReport::report_map_;
std::shared_ptr<std::string> ObstacleCodePathReport::current_report_name_ =
    nullptr;

ObstacleCodePathReport::ObstacleCodePathReport() = default;

std::string ObstacleCodePathReport::ReportString() const {
  std::stringstream report_stream;
  report_stream << "--------------ObstacleCodePathReport[" << name_
                << "]---------------\n";
  for (auto iter = obstacle_wise_root_executions_.begin();
       iter != obstacle_wise_root_executions_.end(); iter++) {
    const std::string &obs_id = iter->first;
    const ConcreteCodeBlockExecution &obstacle_root_execution = iter->second;
    report_stream << "[" << obs_id << "]\n";
    report_stream << obstacle_root_execution.ExecutionReportText(0);
    report_stream << "\n";
  }
  return report_stream.str();
}

void ObstacleCodePathReport::SerializeToPb(
    ObstacleCodePathReportPb *const obstacle_code_path_report_pb) const {
  obstacle_code_path_report_pb->set_name(name_);
  ObstacleWiseRootExecutionsPb &obstacle_wise_root_executions_pb =
      *(obstacle_code_path_report_pb->mutable_obstacle_wise_root_executions());
  for (auto itr = obstacle_wise_root_executions_.begin();
       itr != obstacle_wise_root_executions_.end(); itr++) {
    const std::string &obs_id = itr->first;
    const ConcreteCodeBlockExecution &obstacle_root_execution = itr->second;
    obstacle_root_execution.SerializeToPb(
        obstacle_wise_root_executions_pb[obs_id].add_child_executions());
  }
}

void ObstacleCodePathReport::Clear() {
  obstacle_wise_root_executions_.clear();
  std::map<std::string, ConcreteCodeBlockExecution>().swap(
      obstacle_wise_root_executions_);
  obs_id_to_last_active_execution_.clear();
  std::map<std::string, CodeBlockExecution>().swap(
      obs_id_to_last_active_execution_);
}

std::string ObstacleCodePathReport::current_report_name() {
  if (current_report_name_ == nullptr) {
    current_report_name_ = std::make_shared<std::string>("Default");
  }
  return *current_report_name_;
}

// CodeBlockExecution.
CodeBlockExecution::CodeBlockExecution(
    ConcreteCodeBlockExecution &code_block_execution)
    : concrete_code_block_execution_(&code_block_execution) {
  LastActiveExecution(code_block_execution.obstacle_id())
      ->concrete_code_block_execution_ = concrete_code_block_execution_;
  concrete_code_block_execution_->start_time_ms =
      std::chrono::duration<double>(
          std::chrono::system_clock::now().time_since_epoch())
          .count() *
      1000.0;
}

CodeBlockExecution::~CodeBlockExecution() {
  if (this->concrete_code_block_execution_ == nullptr) {
    return;
  }
  concrete_code_block_execution_->end_time_ms =
      std::chrono::duration<double>(
          std::chrono::system_clock::now().time_since_epoch())
          .count() *
      1000.0;
  CodeBlockExecution *last_active_execution =
      LastActiveExecution(this->concrete_code_block_execution_->obstacle_id());
  if (last_active_execution->concrete_code_block_execution_ != nullptr) {
    last_active_execution->concrete_code_block_execution_ =
        last_active_execution->concrete_code_block_execution_
            ->parent_execution();
  }
}

CodeBlockExecution
CodeBlockExecution::AddChildExecution(const std::string &name,
                                      bool is_code_block_conditional) {

  assert(this->concrete_code_block_execution_);
  ConcreteCodeBlockExecution *child_concrete_code_block_execution =
      concrete_code_block_execution_->AddChildExecution(
          this->concrete_code_block_execution_->obstacle_id(), name,
          is_code_block_conditional);
  assert(child_concrete_code_block_execution);
  return CodeBlockExecution(*child_concrete_code_block_execution);
}

bool CodeBlockExecution::set_code_block_condition_satisfied(
    const bool code_block_condition_satisfied) {
  concrete_code_block_execution_->set_code_block_condition_satisfied(
      code_block_condition_satisfied);
  return code_block_condition_satisfied;
}

ConcreteCodeBlockExecution *
CodeBlockExecution::AddDetail(const std::string &detail) {
  return concrete_code_block_execution_->AddDetail(detail);
}

CodeBlockExecution *
CodeBlockExecution::LastActiveExecution(const std::string &obs_id) {
  ObstacleCodePathReport::Instance();
  auto &obs_id_to_last_active_execution =
      ObstacleCodePathReport::Instance()->obs_id_to_last_active_execution_;
  if (obs_id_to_last_active_execution.find(obs_id) ==
      obs_id_to_last_active_execution.end()) {
    obs_id_to_last_active_execution[obs_id] = CodeBlockExecution();
  }
  if (obs_id_to_last_active_execution[obs_id].concrete_code_block_execution_ ==
      nullptr) {
    obs_id_to_last_active_execution[obs_id].concrete_code_block_execution_ =
        ObstacleCodePathReport::Instance()->GetOrCreateRootExecution(obs_id);
    assert(
        obs_id_to_last_active_execution[obs_id].concrete_code_block_execution_);
  }
  assert(
      obs_id_to_last_active_execution[obs_id].concrete_code_block_execution_);
  return &obs_id_to_last_active_execution[obs_id];
}

bool ConcreteCodeBlockExecution::set_code_block_condition_satisfied(
    bool code_block_condition_satisfied) {
  code_block_condition_satisfied_ = code_block_condition_satisfied;
  return code_block_condition_satisfied;
}

ConcreteCodeBlockExecution *ConcreteCodeBlockExecution::parent_execution() {
  return parent_execution_;
}

void ConcreteCodeBlockExecution::set_parent_execution(
    ConcreteCodeBlockExecution *parent_execution) {
  parent_execution_ = parent_execution;
}

ConcreteCodeBlockExecution *
ConcreteCodeBlockExecution::AddChildExecution(const std::string &obstacle_id,
                                              const std::string &name,
                                              bool is_code_block_conditional) {
  ConcreteCodeBlockExecution child_execution =
      ConcreteCodeBlockExecution(obstacle_id, name, is_code_block_conditional);
  if (this->name_ != "Root") {
    child_execution.set_parent_execution(this);
  }
  child_executions_.push_back(child_execution);
  return &child_executions_.back();
}

std::string ConcreteCodeBlockExecution::DetailString() const {
  std::string summary_detail;
  for (size_t idx = 0; idx < details_.size(); idx++) {
    summary_detail += details_[idx];
    if (idx != details_.size() - 1) {
      summary_detail += "\n";
    }
  }
  return summary_detail;
}

ConcreteCodeBlockExecution *
ConcreteCodeBlockExecution::AddDetail(const std::string &detail_to_add) {
  details_.push_back(detail_to_add);
  return this;
}

std::string
ConcreteCodeBlockExecution::ExecutionReportText(int indent_level) const {
  std::stringstream line_stream;
  if (name_ != "Root") {
    std::string tab_string;
    for (int i = 0; i < indent_level; i++) {
      tab_string += "\t";
    }
    line_stream << tab_string << name_;
    if (is_code_block_conditional_) {
      line_stream << "(" << code_block_condition_satisfied_ << ")";
    }
    if (!details_.empty()) {
      line_stream << "{" << DetailString() << "}";
    }
  }
  for (const ConcreteCodeBlockExecution &child_execution : child_executions_) {
    if (!line_stream.str().empty()) {
      line_stream << "\n";
    }
    line_stream << child_execution.ExecutionReportText(indent_level + 1);
  }
  return line_stream.str();
}

void ConcreteCodeBlockExecution::SerializeToPb(
    CodeBlockExecutionPb *const code_block_execution_pb) const {
  code_block_execution_pb->set_name(name_);
  code_block_execution_pb->set_is_code_block_conditional(
      is_code_block_conditional_);
  if (is_code_block_conditional_) {
    code_block_execution_pb->set_code_block_condition_satisfied(
        code_block_condition_satisfied_);
  }
  if (!details_.empty()) {
    for (const std::string &detail : details_) {
      code_block_execution_pb->add_details(detail);
    }
  }
  code_block_execution_pb->set_duration_ms(static_cast<float>(duration_ms()));
  if (!child_executions_.empty()) {
    for (const ConcreteCodeBlockExecution &child_execution :
         child_executions_) {
      child_execution.SerializeToPb(
          code_block_execution_pb->add_child_executions());
    }
  }
}

ConcreteCodeBlockExecution *
ObstacleCodePathReport::GetOrCreateRootExecution(const std::string &obs_id) {
  if (obstacle_wise_root_executions_.find(obs_id) ==
      obstacle_wise_root_executions_.end()) {
    obstacle_wise_root_executions_[obs_id] =
        ConcreteCodeBlockExecution(obs_id, "Root", false);
  }
  return &obstacle_wise_root_executions_[obs_id];
}

}
