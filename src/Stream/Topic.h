// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "Kafka/BrokerSettings.h"
#include "Kafka/ConsumerFactory.h"
#include "Metrics/Registrar.h"
#include "Partition.h"
#include "Stream/MessageWriter.h"
#include "ThreadedExecutor.h"
#include "logger.h"
#include <chrono>
#include <vector>

namespace Stream {

class Topic {
public:
  Topic(Kafka::BrokerSettings const &Settings, std::string const &Topic,
        SrcToDst Map, MessageWriter *Writer, Metrics::Registrar &RegisterMetric,
        time_point StartTime, duration StartTimeLeeway, time_point StopTime,
        duration StopTimeLeeway,
        std::unique_ptr<Kafka::ConsumerFactoryInterface> CreateConsumers =
            std::make_unique<Kafka::ConsumerFactory>());

  /// \brief Must be called after the constructor.
  /// \note This function exist in order to make unit testing possible.
  void start();

  void setStopTime(std::chrono::system_clock::time_point StopTime);

  bool isDone() { return IsDone.load(); };

  virtual ~Topic() = default;

protected:
  std::atomic_bool IsDone{false};
  Kafka::BrokerSettings KafkaSettings;
  std::string TopicName;
  SrcToDst DataMap;
  MessageWriter *WriterPtr;
  time_point StartConsumeTime;
  duration StartLeeway;
  time_point StopConsumeTime;
  duration StopLeeway;
  duration CurrentMetadataTimeOut;
  Metrics::Registrar Registrar;

  // This intermediate function is required for unit testing.
  virtual void initMetadataCalls(Kafka::BrokerSettings const &Settings,
                                 std::string const &Topic);

  virtual void getPartitionsForTopic(Kafka::BrokerSettings const &Settings,
                                     std::string const &Topic);

  virtual void getOffsetsForPartitions(Kafka::BrokerSettings const &Settings,
                                       std::string const &Topic,
                                       std::vector<int> const &Partitions);

  virtual void
  createStreams(Kafka::BrokerSettings const &Settings, std::string const &Topic,
                std::vector<std::pair<int, int64_t>> const &PartitionOffsets);

  virtual std::vector<std::pair<int, int64_t>>
  getOffsetForTimeInternal(std::string const &Broker, std::string const &Topic,
                           std::vector<int> const &Partitions, time_point Time,
                           duration TimeOut) const;

  virtual std::vector<int>
  getPartitionsForTopicInternal(std::string const &Broker,
                                std::string const &Topic,
                                duration TimeOut) const;

  void checkIfDone();
  virtual void checkIfDoneTask();

  std::vector<std::unique_ptr<Partition>> ConsumerThreads;
  std::unique_ptr<Kafka::ConsumerFactoryInterface> ConsumerCreator;
  ThreadedExecutor Executor; // Must be last
};
} // namespace Stream
