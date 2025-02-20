#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <linux/can.h>

#include "mdflib_c_wrapper.h"

#pragma region C

#include <mdflibrary/MdfExport.h>
using namespace MdfLibrary;
using namespace MdfLibrary::ExportFunctions;

Mdf4FileHandle mdf4_canlog_create(const char* filepath) {
	auto* Writer = MdfWriterInit(MdfWriterType::MdfBusLogger, filepath);
	auto* Header = MdfWriterGetHeader(Writer);
	MdfHeaderSetAuthor(Header, "Specialized Bicycle Components");
	MdfHeaderSetDepartment(Header, "TURBO Future");
	MdfHeaderSetDescription(Header, "SBC-CAN candump log");
	MdfHeaderSetProject(Header, "Yutu Logger");
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

	if (!MdfWriterInitMeasurement(Writer)) {
		fprintf(stderr, "Failed to init measurement\n");
		return nullptr;
	}

	return (Mdf4FileHandle) Writer;
}

int mdf4_canlog_write(Mdf4FileHandle handle, struct Message* message) {
	auto* writer = (mdf::MdfWriter*) handle;
	auto* header = MdfWriterGetHeader(writer);
	auto* last_dg = MdfHeaderGetLastDataGroup(header);
	mdf::IChannelGroup* can_data_frame;

	if (message->id & CAN_RTR_FLAG) {
		can_data_frame = MdfDataGroupGetChannelGroupByName(last_dg, "CAN_RemoteFrame");
	} else if (message->id & CAN_ERR_FLAG) {
		can_data_frame = MdfDataGroupGetChannelGroupByName(last_dg, "CAN_ErrorFrame");
	} else {
		can_data_frame = MdfDataGroupGetChannelGroupByName(last_dg, "CAN_DataFrame");
	}

	if (can_data_frame == nullptr) {
		fprintf(stderr, "Failed to get CAN_DataFrame channel group\n");
		return -1;
	}

	auto* msg = CanMessageInit();
	CanMessageSetMessageId(msg, message->id & CAN_EFF_MASK);
	CanMessageSetExtendedId(msg, message->id & CAN_EFF_FLAG);
	CanMessageSetBusChannel(msg, message->channel);
	CanMessageSetDataBytes(msg, message->data, message->dlc);

	if (MdfWriterGetStartTime(writer) == 0) {
		MdfWriterStartMeasurement(writer, message->timestamp);
	}

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
