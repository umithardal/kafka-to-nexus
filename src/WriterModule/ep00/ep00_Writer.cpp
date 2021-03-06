#include "ep00_Writer.h"
#include "FlatbufferMessage.h"
#include "WriterRegistrar.h"
#include <ep00_epics_connection_info_generated.h>

namespace WriterModule {
namespace ep00 {

void ep00_Writer::parse_config(std::string const &ConfigurationStream) {
  // This writer module has no additional options to parse
  UNUSED_ARG(ConfigurationStream);
}

InitResult ep00_Writer::reopen(hdf5::node::Group &HDFGroup) {
  auto Open = NeXusDataset::Mode::Open;
  try {
    TimestampDataset = NeXusDataset::ConnectionStatusTime(HDFGroup, Open);
    StatusDataset = NeXusDataset::ConnectionStatus(HDFGroup, Open);
  } catch (std::exception &E) {
    Logger->error(
        "Failed to reopen datasets in HDF file with error message: \"{}\"",
        std::string(E.what()));
    return InitResult::ERROR;
  }
  return InitResult::OK;
}

InitResult ep00_Writer::init_hdf(hdf5::node::Group &HDFGroup,
                                 std::string const &) {
  auto Create = NeXusDataset::Mode::Create;
  try {
    NeXusDataset::ConnectionStatusTime(HDFGroup,
                                       Create); // NOLINT(bugprone-unused-raii)
    NeXusDataset::ConnectionStatus(HDFGroup,
                                   Create); // NOLINT(bugprone-unused-raii)
  } catch (std::exception const &E) {
    auto message = hdf5::error::print_nested(E);
    Logger->error("ep00 could not init HDFGroup: {}  trace: {}",
                  static_cast<std::string>(HDFGroup.link().path()), message);
    return InitResult::ERROR;
  }
  return InitResult::OK;
}

void ep00_Writer::write(FileWriter::FlatbufferMessage const &Message) {
  auto FlatBuffer = GetEpicsConnectionInfo(Message.data());
  std::string const Status = EnumNameEventType(FlatBuffer->type());
  StatusDataset.appendStringElement(Status);
  auto FBTimestamp = FlatBuffer->timestamp();
  TimestampDataset.appendElement(FBTimestamp);
}

static WriterModule::Registry::Registrar<ep00_Writer> RegisterWriter("ep00",
                                                                     "ep00");

} // namespace ep00
} // namespace WriterModule
