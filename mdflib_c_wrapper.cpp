#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#include "mdflib_c_wrapper.h"

#pragma region C

#include <mdflibrary/MdfExport.h>
using namespace MdfLibrary;
using namespace MdfLibrary::ExportFunctions;
/*#include <mdflibrary/MdfChannelObserver.h>*/
/*#include <mdflibrary/MdfReader.h>*/
/*#include <mdflibrary/MdfWriter.h>*/

Mdf4FileHandle mdf4_canlog_create(const char* filepath, uint64_t tick_time) {
  auto* Writer = MdfWriterInit(MdfWriterType::MdfBusLogger, filepath);
  auto* Header = MdfWriterGetHeader(Writer);
  auto* History = MdfHeaderCreateFileHistory(Header);
  MdfFileHistorySetDescription(History, "SBC-CAN candump log");
  MdfFileHistorySetToolName(History, "candump sbc fork");
  MdfFileHistorySetToolVendor(History, "Specialized Europe GmbH");
  MdfFileHistorySetToolVersion(History, "1.0");
  MdfFileHistorySetUserName(History, "John Whittington");

  MdfWriterSetBusType(Writer, MdfBusType::CAN);
  MdfWriterSetStorageType(Writer, MdfStorageType::MlsdStorage);
  MdfWriterSetMaxLength(Writer, 8);
  if (!MdfWriterCreateBusLogConfiguration(Writer)) {
    fprintf(stderr, "Failed to create bus log configuration\n");
    return nullptr;
  }
  MdfWriterSetPreTrigTime(Writer, 0.0);
  MdfWriterSetCompressData(Writer, false);

  MdfWriterInitMeasurement(Writer);
  MdfWriterStartMeasurement(Writer, tick_time);

  return (Mdf4FileHandle) Writer;
}

int mdf4_canlog_write(Mdf4FileHandle handle, struct Message* message) {
  auto* writer = (mdf::MdfWriter*) handle;
  auto* header = MdfWriterGetHeader(writer);
  auto* last_dg = MdfHeaderGetLastDataGroup(header);
  auto* can_data_frame = MdfDataGroupGetChannelGroupByName(last_dg, "CAN_DataFrame");

  if (can_data_frame == nullptr) {
    fprintf(stderr, "Failed to get CAN_DataFrame channel group\n");
    return -1;
  }

  auto* msg = CanMessageInit();
  CanMessageSetMessageId(msg, message->id);
  CanMessageSetExtendedId(msg, true);
  CanMessageSetBusChannel(msg, 1);
  CanMessageSetDataBytes(msg, message->data, message->dlc);

  MdfWriterSaveCanMessage(writer, can_data_frame, message->timestamp, msg);
  return 0;
}

int mdf4_canlog_close(Mdf4FileHandle handle, uint64_t tick_time) {
  auto* writer = (mdf::MdfWriter*) handle;
  MdfWriterStopMeasurement(writer, tick_time);
  MdfWriterFinalizeMeasurement(writer);
  return 0;
}

#pragma endregion
