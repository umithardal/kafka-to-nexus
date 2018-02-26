#pragma once

#include "BrokerSettings.h"
#include "Msg.h"
#include <atomic>
#include <functional>
#include <librdkafka/rdkafka.h>

namespace KafkaW {

class ProducerTopic;

class ProducerMsg {
public:
  virtual ~ProducerMsg();
  virtual void deliveryOk();
  virtual void deliveryError();
  uchar *data;
  uint32_t size;
};

struct ProducerStats {
  std::atomic<uint64_t> produced{0};
  std::atomic<uint32_t> produce_fail{0};
  std::atomic<uint32_t> local_queue_full{0};
  std::atomic<uint64_t> produce_cb{0};
  std::atomic<uint64_t> produce_cb_fail{0};
  std::atomic<uint64_t> poll_served{0};
  std::atomic<uint64_t> msg_too_large{0};
  std::atomic<uint64_t> produced_bytes{0};
  std::atomic<uint32_t> out_queue{0};
  ProducerStats();
  ProducerStats(ProducerStats const &);
};

class Producer {
public:
  typedef ProducerTopic Topic;
  typedef ProducerMsg Msg;
  typedef ProducerStats Stats;
  Producer(BrokerSettings ProducerBrokerSettings_);
  Producer(Producer const &) = delete;
  Producer(Producer &&x);
  ~Producer();
  void pollWhileOutputQueueFilled();
  void poll();
  uint64_t total_produced();
  uint64_t outputQueueLength();
  static void cb_delivered(rd_kafka_t *rk, rd_kafka_message_t const *msg,
                           void *opaque);
  static void cb_error(rd_kafka_t *rk, int err_i, char const *reason,
                       void *opaque);
  static int cb_stats(rd_kafka_t *rk, char *json, size_t json_len,
                      void *opaque);
  static void cb_log(rd_kafka_t const *rk, int level, char const *fac,
                     char const *buf);
  static void cb_throttle(rd_kafka_t *rk, char const *broker_name,
                          int32_t broker_id, int throttle_time_ms,
                          void *opaque);
  rd_kafka_t *rd_kafka_ptr() const;
  std::function<void(rd_kafka_message_t const *msg)> on_delivery_ok;
  std::function<void(rd_kafka_message_t const *msg)> on_delivery_failed;
  std::function<void(Producer *, rd_kafka_resp_err_t)> on_error;
  // Currently it's nice to have acces to these two for statistics:
  BrokerSettings ProducerBrokerSettings;
  rd_kafka_t *rk = nullptr;
  std::atomic<uint64_t> total_produced_{0};
  Stats stats;

private:
  int id = 0;
};
}
