// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "BrokerSettings.h"
#include "ConsumerRebalanceCb.h"
#include "KafkaEventCb.h"
#include "PollStatus.h"
#include <chrono>
#include <librdkafka/rdkafkacpp.h>
#include <memory>

namespace FileWriter {
struct Msg;
}

namespace KafkaW {

class ConsumerInterface {
public:
  ConsumerInterface() = default;
  virtual ~ConsumerInterface() = default;
  virtual void addTopic(std::string const &Topic) = 0;
  virtual void addTopicAtTimestamp(std::string const &Topic,
                                   std::chrono::milliseconds StartTime) = 0;
  virtual std::unique_ptr<std::pair<PollStatus, FileWriter::Msg>> poll() = 0;
  virtual bool topicPresent(const std::string &Topic) = 0;
  virtual std::vector<int32_t>
  queryTopicPartitions(const std::string &TopicName) = 0;
};

class Consumer : public ConsumerInterface {
public:
  /// The constructor
  ///
  /// \param RdConsumer The RdKafka Consumer to wrap.
  /// \param RdConf The Configuration for the RdKafka Consumer.
  /// \param EventCb Callback for each time the consumer is polled.
  Consumer(std::unique_ptr<RdKafka::KafkaConsumer> RdConsumer,
           std::unique_ptr<RdKafka::Conf> RdConf,
           std::unique_ptr<KafkaEventCb> EventCb);
  Consumer(Consumer &&) = delete;
  Consumer(Consumer const &) = delete;
  ~Consumer() override;
  /// Adds topic to consumer at latest offset.
  ///
  /// \param Topic The name of the topic to add.
  void addTopic(std::string const &Topic) override;
  /// Adds topic to consumer, consuming from a specified timestamp
  ///
  /// \param Topic The name of the topic to add.
  /// \param StartTime Start timestamp to consume from.
  void addTopicAtTimestamp(std::string const &Topic,
                           std::chrono::milliseconds StartTime) override;
  /// Checks if a topic is present on the broker.
  /// This is used to prevent passing around a pointer to an iterator of an
  /// RdKafka::Topic outside of KafkaW and instead returns a bool.
  /// \param TopicName Name of the topic to check.
  /// \return True or false depending on whether the topic exists or not.
  bool topicPresent(const std::string &TopicName) override;
  /// Get a list of partition numbers in a topic.
  ///
  /// \param Topic The name of the topic to query.
  /// \return List of partition numbers on topic.
  std::vector<int32_t> queryTopicPartitions(const std::string &Topic) override;
  /// Polls for any new messages.
  ///
  /// \return Any new messages consumed.
  std::unique_ptr<std::pair<PollStatus, FileWriter::Msg>> poll() override;

protected:
  std::unique_ptr<RdKafka::KafkaConsumer> KafkaConsumer;

private:
  const RdKafka::TopicMetadata *findTopic(const std::string &Topic);
  std::unique_ptr<RdKafka::Conf> Conf;
  BrokerSettings ConsumerBrokerSettings;
  void updateMetadata();
  std::shared_ptr<RdKafka::Metadata> KafkaMetadata;
  int id = 0;
  std::unique_ptr<KafkaEventCb> EventCallback;
  void assignToPartitions(const std::string &Topic,
                          const std::vector<RdKafka::TopicPartition *>
                              &TopicPartitionsWithOffsets) const;
  std::vector<RdKafka::TopicPartition *>
  queryWatermarkOffsets(const std::string &Topic);
  bool metadataCall();
  SharedLogger Logger = spdlog::get("filewriterlogger");
};
} // namespace KafkaW
