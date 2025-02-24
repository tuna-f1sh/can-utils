#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <linux/can.h>

#include "mdflib_c_wrapper.h"

#pragma region C++

#include "mdf/mdffactory.h"
#include "mdf/mdfwriter.h"
#include "mdf/ifilehistory.h"
#include "mdf/idatagroup.h"
#include "mdf/canmessage.h"
using namespace mdf;

Mdf4FileHandle mdf4_canlog_create(const char* filepath) {
	auto* writer = MdfFactory::CreateMdfWriterEx(MdfWriterType::MdfBusLogger);
	writer->Init(filepath);
	auto* header = writer->Header();
	header->Author("Specialized Bicycle Components");
	header->Department("TURBO Future");
	header->Description("SBC-CAN candump log");
	header->Project("Yutu Logger");
	auto* history = header->CreateFileHistory();
	history->Description("SBC-CAN candump log");
	history->ToolName("candump sbc fork");
	history->ToolVendor("Specialized Europe GmbH");
	history->ToolVersion("1.0");
	history->UserName("John Whittington");

	writer->BusType(MdfBusType::CAN);
	writer->StorageType(MdfStorageType::MlsdStorage);
	writer->MaxLength(8);

	if (!writer->CreateBusLogConfiguration()) {
		fprintf(stderr, "Failed to create bus log configuration\n");
		return nullptr;
	}

	writer->PreTrigTime(0.0);
	writer->CompressData(false);

	if (!writer->InitMeasurement()) {
		fprintf(stderr, "Failed to init measurement\n");
		return nullptr;
	}

	return (Mdf4FileHandle) writer;
}

int mdf4_canlog_write(Mdf4FileHandle handle, struct Message* message) {
	auto* writer = (mdf::MdfWriter*) handle;
	/*auto* header = MdfWriterGetHeader(writer);*/
	/*auto* last_dg = MdfHeaderGetLastDataGroup(header);*/
	auto* header = writer->Header();
	auto* last_dg = header->LastDataGroup();
	mdf::IChannelGroup* can_data_frame;

	if (message->id & CAN_RTR_FLAG) {
		can_data_frame = last_dg->GetChannelGroup("CAN_RemoteFrame");
	} else if (message->id & CAN_ERR_FLAG) {
		can_data_frame = last_dg->GetChannelGroup("CAN_ErrorFrame");
	} else {
		can_data_frame = last_dg->GetChannelGroup("CAN_DataFrame");
	}

	if (can_data_frame == nullptr) {
		fprintf(stderr, "Failed to get CAN_DataFrame channel group\n");
		return -1;
	}

	CanMessage msg;
	msg.MessageId(message->id & CAN_EFF_MASK);
	msg.ExtendedId(message->id & CAN_EFF_FLAG);
	msg.BusChannel(message->channel);
	msg.DataBytes(std::vector<uint8_t>(message->data, message->data + message->dlc));

	if (writer->StartTime() == 0) {
		writer->StartMeasurement(message->timestamp);
	}


	writer->SaveCanMessage(*can_data_frame, message->timestamp, msg);

	return 0;
}

int mdf4_canlog_close(Mdf4FileHandle handle, uint64_t tick_time) {
	auto* writer = (mdf::MdfWriter*) handle;
	writer->StopMeasurement(tick_time);
	writer->FinalizeMeasurement();

	return 0;
}

#pragma endregion
