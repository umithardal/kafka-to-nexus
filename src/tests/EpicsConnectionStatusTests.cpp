#include "../schemas/ep00/FlatbufferReader.h"
#include "../schemas/ep00/ep00_rw.h"
#include "HDFWriterModule.h"
#include <gtest/gtest.h>

namespace FileWriter {
namespace Schemas {
namespace ep00 {
using nlohmann::json;

class ep00Tests : public ::testing::Test {
public:
  void SetUp() override {
    try {
      FileWriter::FlatbufferReaderRegistry::Registrar<
          FileWriter::Schemas::ep00::FlatbufferReader>
          RegisterIt("ep00");
    } catch (...) {
    }
    File = hdf5::file::create(TestFileName, hdf5::file::AccessFlags::TRUNCATE);
    RootGroup = File.root();
    UsedGroup = RootGroup.create_group(NXLogGroup);
  }
  void TearDown() override { File.close(); };

  std::string NXLogGroup{"SomeParentName"};
  std::string TestFileName{"SomeTestFile.hdf5"};
  hdf5::file::File File;
  hdf5::node::Group RootGroup;
  hdf5::node::Group UsedGroup;
};

TEST_F(ep00Tests, file_init_ok) {
  {
    ep00::HDFWriterModule Writer;
    EXPECT_TRUE(Writer.init_hdf(UsedGroup, "{}") ==
                HDFWriterModule_detail::InitResult::OK);
  }
  ASSERT_TRUE(RootGroup.has_group(NXLogGroup));
  auto TestGroup = RootGroup.get_group(NXLogGroup);
  EXPECT_TRUE(TestGroup.has_dataset("alarm_status"));
  EXPECT_TRUE(TestGroup.has_dataset("alarm_time"));
}

TEST_F(ep00Tests, reopen_file) {
  ep00::HDFWriterModule Writer;
  EXPECT_FALSE(Writer.reopen(UsedGroup) ==
               HDFWriterModule_detail::InitResult::OK);
}

TEST_F(ep00Tests, InitFileFail) {
  ep00::HDFWriterModule Writer;
  EXPECT_TRUE(Writer.init_hdf(UsedGroup, "{}") ==
              HDFWriterModule_detail::InitResult::OK);
  EXPECT_FALSE(Writer.init_hdf(UsedGroup, "{}") ==
               HDFWriterModule_detail::InitResult::OK);
}

TEST_F(ep00Tests, ReopenFileSuccess) {
  ep00::HDFWriterModule Writer;
  EXPECT_TRUE(Writer.init_hdf(UsedGroup, "{}") ==
              HDFWriterModule_detail::InitResult::OK);
  EXPECT_TRUE(Writer.reopen(UsedGroup) ==
              HDFWriterModule_detail::InitResult::OK);
}
} // namespace ep00
} // namespace Schemas
} // namespace FileWriter
