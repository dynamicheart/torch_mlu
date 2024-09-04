// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <set>
#include <deque>

#include "include/IActivityProfiler.h"
#include "include/Config.h"
#include "src/output_base.h"

namespace libkineto {

class MockProfilerSession: public IActivityProfilerSession {

  public:
    explicit MockProfilerSession() {}

    void start() override {
      start_count++;
      status_ = TraceStatus::RECORDING;
    }

    void stop() override {
      stop_count++;
      status_ = TraceStatus::PROCESSING;
    }

    std::vector<std::string> errors() override {
      return {};
    }

    void processTrace(ActivityLogger& logger) override;

    void set_test_activities(std::deque<GenericTraceActivity>&& acs) {
      test_activities_ = std::move(acs);
    }

    std::unique_ptr<CpuTraceBuffer> getTraceBuffer() override;

    int start_count = 0;
    int stop_count = 0;
  private:
    std::deque<GenericTraceActivity> test_activities_;
};


class MockActivityProfiler: public IActivityProfiler {

 public:
  explicit MockActivityProfiler(std::deque<GenericTraceActivity>& activities);

  const std::string& name() const override;

  const std::set<ActivityType>& availableActivities() const override;

  std::unique_ptr<IActivityProfilerSession> configure(
      const std::set<ActivityType>& activity_types,
      const KINETO_NAMESPACE::Config& config) override;

  std::unique_ptr<IActivityProfilerSession> configure(
      int64_t ts_ms,
      int64_t duration_ms,
      const std::set<ActivityType>& activity_types,
      const KINETO_NAMESPACE::Config& config) override;

 private:
  std::deque<GenericTraceActivity> test_activities_;
};

} // namespace libkineto
