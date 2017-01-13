#pragma once
#include <string>
#include "TimeDifferenceFromMessage.h"

namespace BrightnESS {
namespace FileWriter {

/// %Result of a call to `process_message`.
/// Can be extended later for more detailed reporting.
class ProcessMessageResult {
public:
inline bool is_OK() { return res == 0; }
inline bool is_ERR() { return res == -1; }
private:
char res = -1;
};

/// Represents a sourcename on a topic.
/// The sourcename can be empty.
/// This is meant for highest efficiency on topics which are exclusively used for only one sourcename.
class DemuxTopic : public TimeDifferenceFromMessage {
public:
DemuxTopic(std::string topic);
std::string const & topic();
/// To be called by FileMaster when a new message is available for this source
ProcessMessageResult process_message(void * msg_data, int msg_size);
DT time_difference_from_message(void * msg_data, int msg_size);
private:
std::string _topic;
std::string _source;
};

}
}
