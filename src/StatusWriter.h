/// \file StatusWriter reads the information on the current status of a
/// StreamMaster, such as number of received messages, number of
/// errors and execution time and about each Streamer managed by the
/// StreamMaster such as message frequency and throughput. These
/// data are then serialized as a JSON message.

#pragma once

#include <nlohmann/json.hpp>

namespace FileWriter {
namespace Status {

class StreamMasterInfo;
class MessageInfo;

class StatusWriter {
public:
  StatusWriter() = default;
  void setJobId(const std::string &JobId);
  void write(StreamMasterInfo &Information);
  void write(MessageInfo &Information, const std::string &Topic);
  std::string getJson();

private:
  nlohmann::json json{{"type", "stream_master_status"},
                      {"next_message_eta_ms", 0},
                      {"job_id", 0}};
};

} // namespace Status
} // namespace FileWriter
