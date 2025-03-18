#pragma once

#include "serialize_sizes.h"

#define PROTO_FIELD_SIZE(message, field, type, data) NInPlaceProto::TFieldSizes::Get##type(message::k##field##FieldNumber, data)
#define PROTO_FIELD_UPDATE_SIZE(type, oldValue, newValue) NInPlaceProto::TFieldSizes::Update##type(oldValue, newValue)
#define PROTO_FIELD_HEADER_SIZE(message, field, type, data) NInPlaceProto::TFieldSizes::Get##type##Header(message::k##field##FieldNumber, data)
