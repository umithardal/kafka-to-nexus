#include "../json.h"
#include "AddReader.h"
#include "FlatbufferMessage.h"
#include "schemas/ns10/NicosCacheReader.h"
#include "schemas/ns10/NicosCacheWriter.h"
#include <flatbuffers/flatbuffers.h>
#include <fstream>
#include <gtest/gtest.h>

namespace FileWriter {
namespace Schemas {
namespace ns10 {
#include "ns10_cache_entry_generated.h"
} // namespace ns10
} // namespace Schemas
} // namespace FileWriter

using FileWriter::Schemas::ns10::CacheReader;
using FileWriter::Schemas::ns10::CacheWriter;

FileWriter::FlatbufferMessage
createFlatbufferMessageFromJson(nlohmann::json Json) {
  double Time = 1.0;
  double Ttl = 1.0;
  uint8_t Expired = 0;
  std::string Key("");
  std::string Value("");

  if (auto Val = find<double>("time", Json)) {
    Time = Val.inner();
  }
  if (auto Val = find<std::string>("key", Json)) {
    Key = Val.inner();
  }
  if (auto Val = find<std::string>("value", Json)) {
    Value = Val.inner();
  }
  if (auto Val = find<double>("ttl", Json)) {
    Ttl = Val.inner();
  }
  if (auto Val = find<uint8_t>("expired", Json)) {
    Expired = Val.inner();
  }

  auto Builder = flatbuffers::FlatBufferBuilder();
  auto FBKey = Builder.CreateString(Key);
  auto FBValue = Builder.CreateString(Value);

  FileWriter::Schemas::ns10::CacheEntryBuilder CEBuilder(Builder);

  CEBuilder.add_key(FBKey);
  CEBuilder.add_time(Time);
  CEBuilder.add_ttl(Ttl);
  CEBuilder.add_expired(Expired);
  CEBuilder.add_value(FBValue);

  FinishCacheEntryBuffer(Builder, CEBuilder.Finish());

  auto Message = FileWriter::FlatbufferMessage(
      reinterpret_cast<char *>(Builder.GetBufferPointer()), Builder.GetSize());

  return Message;
}

void registerSchema() {
  try {
    FileWriter::FlatbufferReaderRegistry::Registrar<CacheReader> RegisterIt(
        "ns10");
  } catch (...) {
  }
  try {
    FileWriter::HDFWriterModuleRegistry::Registrar<CacheWriter> RegisterIt(
        "ns10");
  } catch (...) {
  }
}

class NicosCacheReaderTest : public ::testing::Test {
public:
  void SetUp() override {
    registerSchema();

    nlohmann::json BufferJson = R"({
      "key": "nicos/device/parameter",
      "writer_module": "ns10",
      "time": 123.456,
      "value": "a string"
    })"_json;

    Message = std::make_unique<FileWriter::FlatbufferMessage>(
        createFlatbufferMessageFromJson(BufferJson));
  };

  void TearDown() override{};
  std::unique_ptr<FileWriter::FlatbufferMessage> Message;
};

TEST_F(NicosCacheReaderTest, ReaderReturnValues) {
  CacheReader SomeReader;
  EXPECT_TRUE(Message->isValid());
  EXPECT_EQ(Message->getSourceName(), std::string("nicos/device/parameter"));
  EXPECT_EQ(Message->getTimestamp(), 123.456 * 1e9);
}

class NicosCacheWriterTest : public ::testing::Test {

public:
  void SetUp() override {
    registerSchema();

    hdf5::property::FileCreationList fcpl;
    hdf5::property::FileAccessList fapl;
    MemoryDriver(fapl);
    File = hdf5::file::create(TestFileName, hdf5::file::AccessFlags::TRUNCATE,
                              fcpl, fapl);

    RootGroup = File.root();
    UsedGroup = RootGroup.create_group(NXLogGroup);
  };

  void TearDown() override { File.close(); };

  std::string TestFileName{"SomeTestFile.hdf5"};
  std::string NXLogGroup{"SomeParentName"};
  hdf5::file::File File;
  hdf5::node::Group RootGroup;
  hdf5::node::Group UsedGroup;
  hdf5::file::MemoryDriver MemoryDriver;
};

class CacheWriterF : public CacheWriter {
public:
  using CacheWriter::ChunkSize;
  using CacheWriter::CueInterval;
  using CacheWriter::CueTimestamp;
  using CacheWriter::CueTimestampIndex;
  using CacheWriter::Sourcename;
  using CacheWriter::Timestamp;
  using CacheWriter::Values;
};

TEST_F(NicosCacheWriterTest, WriterReturnValues) {
  CacheWriter SomeWriter;
  EXPECT_TRUE(SomeWriter.init_hdf(UsedGroup, "{}") ==
              FileWriter::HDFWriterModule_detail::InitResult::OK);
  EXPECT_TRUE(SomeWriter.reopen(UsedGroup) ==
              FileWriter::HDFWriterModule_detail::InitResult::OK);
  EXPECT_EQ(SomeWriter.flush(), 0);
  EXPECT_EQ(SomeWriter.close(), 0);
}

TEST_F(NicosCacheWriterTest, WriterInitCreateGroupTest) {
  CacheWriter SomeWriter;
  SomeWriter.init_hdf(UsedGroup, "{}");

  EXPECT_TRUE(UsedGroup.has_dataset("cue_index"));
  EXPECT_TRUE(UsedGroup.has_dataset("value"));
  EXPECT_TRUE(UsedGroup.has_dataset("time"));
  EXPECT_TRUE(UsedGroup.has_dataset("cue_timestamp_zero"));
  bool FoundAttribute{false};
  for (const auto &Attribute : UsedGroup.attributes) {
    if (Attribute.name() == "NX_class") {
      std::string ClassValue;
      Attribute.read(ClassValue);
      if (ClassValue == "NXlog") {
        FoundAttribute = true;
      }
    }
  }
  EXPECT_TRUE(FoundAttribute);
}

TEST_F(NicosCacheWriterTest, WriterConfiguration) {
  nlohmann::json JsonConfig = R"({
    "source" : "nicos/device/parameter",
    "cue_interval": 1024,
    "chunk_size": 128
  })"_json;

  CacheWriterF Writer;
  Writer.parse_config(JsonConfig.dump(), "");
  EXPECT_EQ(Writer.Sourcename, JsonConfig["source"]);
  EXPECT_EQ(Writer.ChunkSize.at(0), JsonConfig["chunk_size"].get<uint64_t>());
  EXPECT_EQ(Writer.CueInterval, JsonConfig["cue_interval"].get<int>());
}

TEST_F(NicosCacheWriterTest, WriteTimeStamp) {
  nlohmann::json JsonConfig = R"({
    "source" : "nicos/device/parameter"
  })"_json;

  CacheWriterF Writer;
  Writer.parse_config(JsonConfig.dump(), "");

  Writer.init_hdf(UsedGroup, "{}");
  Writer.reopen(UsedGroup);

  nlohmann::json BufferJson = R"({
    "key": "nicos/device/parameter",
    "writer_module": "ns10",
    "time": 123.456,
    "value": "a string"
  })"_json;

  FileWriter::FlatbufferMessage Message(
      createFlatbufferMessageFromJson(BufferJson));

  Writer.write(Message);

  uint64_t storedTs{11111};
  Writer.Timestamp.read(storedTs);
  EXPECT_EQ(storedTs, 123456000000ul);
}

TEST_F(NicosCacheWriterTest, WriteValues) {
  nlohmann::json JsonConfig = R"({
    "source" : "nicos/device/parameter"
  })"_json;

  CacheWriterF Writer;
  Writer.parse_config(JsonConfig.dump(), "");

  Writer.init_hdf(UsedGroup, "{}");
  Writer.reopen(UsedGroup);

  nlohmann::json BufferJson = R"({
    "key": "nicos/device/parameter",
    "writer_module": "ns10",
    "time": 123.456,
    "value": "a string"
  })"_json;

  FileWriter::FlatbufferMessage Message(
      createFlatbufferMessageFromJson(BufferJson));

  Writer.write(Message);

  std::string storedValues;
  Writer.Values.read(storedValues);
  EXPECT_EQ(BufferJson["value"], storedValues);
}

TEST_F(NicosCacheWriterTest, IgnoreMessagesFromDifferentSource) {
  nlohmann::json JsonConfig = R"({
    "source" : "nicos/device/parameter"
  })"_json;

  CacheWriterF Writer;
  Writer.parse_config(JsonConfig.dump(), "");

  Writer.init_hdf(UsedGroup, "{}");
  Writer.reopen(UsedGroup);

  nlohmann::json BufferJson = R"({
    "key": "nicos/device2/parameter",
    "writer_module": "ns10",
    "time": 123.456,
    "value": "a string"
  })"_json;

  FileWriter::FlatbufferMessage Message(
      createFlatbufferMessageFromJson(BufferJson));

  Writer.write(Message);

  std::uint64_t storedTs;
  std::string storedValues;
  EXPECT_ANY_THROW(Writer.Timestamp.read(storedTs));
  EXPECT_ANY_THROW(Writer.Values.read(storedValues));
}

TEST_F(NicosCacheWriterTest, UpdateCueIndex) {
  nlohmann::json JsonConfig = R"({
    "source" : "nicos/device/parameter",
    "cue_interval": 10
  })"_json;

  CacheWriterF Writer;
  Writer.parse_config(JsonConfig.dump(), "");

  Writer.init_hdf(UsedGroup, "{}");
  Writer.reopen(UsedGroup);

  nlohmann::json BufferJson = R"({
    "key": "nicos/device/parameter",
    "writer_module": "ns10",
    "time": 123.456,
    "value": "a string"
  })"_json;

  for (uint64_t i = 0; i < 10; ++i) {
    FileWriter::FlatbufferMessage Message(
        createFlatbufferMessageFromJson(BufferJson));
    Writer.write(Message);
  }

  uint32_t Index;
  EXPECT_NO_THROW(Writer.CueTimestampIndex.read(Index));
}