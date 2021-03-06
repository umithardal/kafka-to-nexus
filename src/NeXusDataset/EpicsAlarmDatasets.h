// SPDX-License-Identifier: BSD-2-Clause
//
// This code has been produced by the European Spallation Source
// and its partner institutes under the BSD 2 Clause License.
//
// See LICENSE.md at the top level for license information.
//
// Screaming Udder!                              https://esss.se

#pragma once

#include "NeXusDataset.h"

namespace NeXusDataset {

class AlarmStatus : public FixedSizeString {
public:
  AlarmStatus() = default;
  AlarmStatus(hdf5::node::Group const &Parent, Mode CMode,
              size_t StringSize = 20, size_t ChunkSize = 1024)
      : FixedSizeString(Parent, "alarm_status", CMode, StringSize, ChunkSize){};
};

class AlarmSeverity : public FixedSizeString {
public:
  AlarmSeverity() = default;
  AlarmSeverity(hdf5::node::Group const &Parent, Mode CMode,
                size_t StringSize = 20, size_t ChunkSize = 1024)
      : FixedSizeString(Parent, "alarm_severity", CMode, StringSize,
                        ChunkSize){};
};

class AlarmTime : public ExtensibleDataset<std::uint64_t> {
public:
  AlarmTime() = default;
  AlarmTime(hdf5::node::Group const &Parent, Mode CMode,
            size_t ChunkSize = 1024);
};

} // namespace NeXusDataset
