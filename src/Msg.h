#pragma once

#include "logger.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <librdkafka/rdkafkacpp.h>
#include <memory>

namespace FileWriter {

struct MessageMetaData {
  std::chrono::milliseconds Timestamp{0};
  RdKafka::MessageTimestamp::MessageTimestampType TimestampType{
      RdKafka::MessageTimestamp::MessageTimestampType::
          MSG_TIMESTAMP_NOT_AVAILABLE};
  int64_t Offset{0};
};

struct Msg {
  static Msg owned(char const *Data, size_t Bytes) {
    auto TempDataPtr = std::make_unique<char[]>(Bytes);
    std::memcpy(reinterpret_cast<void *>(TempDataPtr.get()), Data, Bytes);
    return {std::move(TempDataPtr), Bytes};
  }

  char const *data() const {
    if (DataPtr == nullptr) {
      getLogger()->error("error at type: {}", -1);
    }
    return DataPtr.get();
  }

  size_t size() const {
    if (DataPtr == nullptr) {
      getLogger()->error("error at type: {}", -1);
    }
    return Size;
  }

  std::unique_ptr<char[]> DataPtr{nullptr};
  size_t Size{0};
  MessageMetaData MetaData;
};

} // namespace FileWriter
