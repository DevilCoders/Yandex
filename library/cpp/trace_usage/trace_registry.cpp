#include "global_registry.h"
#include "trace_registry.h"

#include <library/cpp/trace_usage/protos/event.pb.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/system/sys_alloc.h>
#include <util/system/thread.h>

using google::protobuf::internal::WireFormatLite;
using google::protobuf::io::CodedOutputStream;

namespace NTraceUsage {
    TReportBlank::TReportBlank(size_t size)
        : Start((ui8*)y_allocate(size))
        , Current(Start)
        , End(Start + size)
    {
    }
    TReportBlank::~TReportBlank() {
        y_deallocate(Start);
    }

    ui8* TReportBlank::GetCurrent(size_t requiredSize) noexcept {
        if (Y_UNLIKELY(Current + requiredSize > End)) {
            const size_t currentShift = Current - Start;
            const size_t realLen = (End - Start) + requiredSize;
            const size_t len = Max<size_t>(FastClp2(realLen), realLen);
            Start = (ui8*)y_reallocate(Start, len);
            Current = Start + currentShift;
            End = Start + len;
        }
        return Current;
    }

    static constexpr int commonDataSize = 29; // 1 tag byte + 1 len byte + 1 tag byte + 8 byte ms data + 1 tag byte + 8 byte thread id + 1 tag byte + 8 byte context id (worst case)

    ui8* ITraceRegistry::WriteCommonData(ui8* start, TMaybe<ui64> reportContext) noexcept {
        const bool hasReportContext = reportContext.Defined();

        const ui32 tagCommonEventData = WireFormatLite::MakeTag(TEventReportProto::kCommonEventDataFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        // tags
        const ui32 tagMicroSecondsTime = WireFormatLite::MakeTag(TCommonEventData::kMicroSecondsTimeFieldNumber, WireFormatLite::WIRETYPE_FIXED64);
        const ui32 tagThreadId = WireFormatLite::MakeTag(TCommonEventData::kThreadIdFieldNumber, WireFormatLite::WIRETYPE_FIXED64);
        const ui32 tagContextId = WireFormatLite::MakeTag(TCommonEventData::kContextIdFieldNumber, WireFormatLite::WIRETYPE_FIXED64);
        const ui32 commonEventDataTagSize = 2 + hasReportContext; // tagMicroSecondsTime, tagThreadId, tagContextid

        // data
        const ui32 fixedFieldsSize = sizeof(ui64) * (2 + hasReportContext); // 24 >= microSeconds + threadId + ContextId

        const ui32 commonEventDataSize = commonEventDataTagSize + fixedFieldsSize; // <= 26

        const ui64 microSeconds = TInstant::Now().MicroSeconds();
        const ui64 threadId = TThread::CurrentThreadId();

        ui8* end = start;
        *end++ = tagCommonEventData;
        *end++ = commonEventDataSize;
        // CommonEventData
        *end++ = tagMicroSecondsTime;
        end = WireFormatLite::WriteFixed64NoTagToArray(microSeconds, end); // 8 bytes

        *end++ = tagThreadId;
        end = WireFormatLite::WriteFixed64NoTagToArray(threadId, end); // 8 bytes

        if (hasReportContext) {
            *end++ = tagContextId;
            end = WireFormatLite::WriteFixed64NoTagToArray(*reportContext, end); // 8 bytes
        }
        return end;
    }

    void ITraceRegistry::WriteDummyEvent(const ui32 fieldNumber) noexcept {
        ui8 buf[commonDataSize + 2]; // commonDataSize + 1 tag byte + 1 len byte
        const ui32 eventTag = WireFormatLite::MakeTag(fieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        ui8* const start = buf;
        ui8* end = WriteCommonData(start, Nothing());
        *end++ = eventTag;
        *end++ = 0; // zero length of message
        ConsumeReport(TArrayRef<const ui8>(start, end));
    }

    void ITraceRegistry::ReportSmallFunctionEvent(TStringBuf functionName, const ui32 fieldNumber) noexcept {
        ui8 buf[commonDataSize + 128]; // 128 = 1 tag byte + 1 len byte + 1 tag byte + 1 len byte + "short functionName"
        const ui32 tagFunctionScope = WireFormatLite::MakeTag(fieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 tagFunction = WireFormatLite::MakeTag(TFunctionScopeEvent::kFunctionFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 functionNameLength = functionName.size();

        const ui32 totalFunctionScopeSize = 2 + functionNameLength;

        ui8* const start = buf;
        ui8* current = WriteCommonData(start, Nothing());

        *current++ = tagFunctionScope;
        *current++ = totalFunctionScopeSize;

        // FunctionScope
        *current++ = tagFunction;
        *current++ = functionNameLength;

        std::char_traits<char>::copy((char*)current, functionName.data(), functionName.size());
        current += functionName.size();

        ConsumeReport(TArrayRef<const ui8>(start, current));
    }

    void ITraceRegistry::ReportFunctionEvent(TStringBuf functionName, const ui32 fieldNumber) noexcept {
        const ui32 tagFunctionScope = WireFormatLite::MakeTag(fieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 tagFunction = WireFormatLite::MakeTag(TFunctionScopeEvent::kFunctionFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 functionNameLength = functionName.size();

        const ui32 totalFunctionScopeSize = 1 + // tagFunction
                                            // variable width
                                            CodedOutputStream::VarintSize32(functionNameLength) +
                                            functionNameLength;

        size_t totalSize =
            commonDataSize +
            1 + // tagFunctionScope
            CodedOutputStream::VarintSize32(totalFunctionScopeSize) +
            totalFunctionScopeSize;

        TReportBlank blank = AcquireReportBlank();
        ui8* current = blank.GetCurrent(totalSize);
        current = WriteCommonData(current, Nothing());

        *current++ = tagFunctionScope;
        current = CodedOutputStream::WriteVarint32ToArray(totalFunctionScopeSize, current); // 1-3 bytes
        // FunctionScope
        *current++ = tagFunction;
        current = CodedOutputStream::WriteVarint32ToArray(functionNameLength, current); // 1-3 bytes
        std::char_traits<char>::copy((char*)current, functionName.data(), functionName.size());
        current += functionName.size();

        blank.SetCurrent(current);
        ReleaseReportBlank(std::move(blank));
    }

    namespace {
        ui8* WriteContextDeclarationEvent(ui8* const start, ui64 contextId) {
            auto* end = start;
            constexpr ui32 contextDeclarationEventSize = 1 + 8; // tag, id
            *end++ = contextDeclarationEventSize;
            const ui32 contextIdTag = WireFormatLite::MakeTag(TContextDeclarationEvent::kContextIdFieldNumber,
                                                              WireFormatLite::WIRETYPE_FIXED64);
            *end++ = contextIdTag;
            end = WireFormatLite::WriteFixed64NoTagToArray(contextId, end);
            return end;
        }
    }

    void ITraceRegistry::ReportChildContextCreation(ui64 parentContext, ui64 childContext) noexcept {
        ui8 buf[commonDataSize + 11]; // 11 = 1 byte tag + 1 byte size + 1 byte tag + 8 byte child context id
        ui8* const start = buf;

        ui8* end = WriteCommonData(start, parentContext);

        const ui32 childContextDeclTag = WireFormatLite::MakeTag(TEventReportProto::kChildContextDeclarationFieldNumber,
                                                                 WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        *end++ = childContextDeclTag;

        end = WriteContextDeclarationEvent(end, childContext);

        ConsumeReport(TArrayRef<const ui8>(start, end));
    }

    void ITraceRegistry::ReportOpenFunction(TStringBuf functionName) noexcept {
        if (Y_LIKELY(functionName.size() < 125)) {
            ReportSmallFunctionEvent(functionName, TEventReportProto::kStartFunctionScopeFieldNumber);
        } else {
            ReportFunctionEvent(functionName, TEventReportProto::kStartFunctionScopeFieldNumber);
        }
    }
    void ITraceRegistry::ReportCloseFunction(TStringBuf functionName) noexcept {
        if (Y_LIKELY(functionName.size() < 125)) {
            ReportSmallFunctionEvent(functionName, TEventReportProto::kCloseFunctionScopeFieldNumber);
        } else {
            ReportFunctionEvent(functionName, TEventReportProto::kCloseFunctionScopeFieldNumber);
        }
    }

    void ITraceRegistry::ReportStartAcquiringMutex() noexcept {
        WriteDummyEvent(TEventReportProto::kStartAcquiringMutexFieldNumber);
    }
    void ITraceRegistry::ReportAcquiredMutex() noexcept {
        WriteDummyEvent(TEventReportProto::kAcquiredMutexFieldNumber);
    }
    void ITraceRegistry::ReportStartReleasingMutex() noexcept {
        WriteDummyEvent(TEventReportProto::kStartReleasingMutexFieldNumber);
    }
    void ITraceRegistry::ReportReleasedMutex() noexcept {
        WriteDummyEvent(TEventReportProto::kReleasedMutexFieldNumber);
    }

    void ITraceRegistry::ReportStartWaitEvent() noexcept {
        WriteDummyEvent(TEventReportProto::kStartWaitEventFieldNumber);
    }
    void ITraceRegistry::ReportFinishWaitEvent() noexcept {
        WriteDummyEvent(TEventReportProto::kFinishWaitEventFieldNumber);
    }

    ITraceRegistry::TReportConstructor::TReportConstructor(TMaybe<ui64> reportContext)
        : TReportConstructor(TGlobalRegistryGuard::GetCurrentRegistry(), reportContext)
    {
    }

    ITraceRegistry::TReportConstructor::TReportConstructor(TIntrusivePtr<ITraceRegistry> registry,
                                                           TMaybe<ui64> reportContext)
        : Registry(std::move(registry))
        , Blank(Registry ? Registry->AcquireReportBlank() : TReportBlank())
    {
        if (Registry) {
            ui8* current = Blank.GetCurrent(commonDataSize);
            Blank.SetCurrent(ITraceRegistry::WriteCommonData(current, reportContext));
        }
    }

    void ITraceRegistry::TReportConstructor::DropRegistry() {
        Registry = nullptr;
    }

    ITraceRegistry::TReportConstructor::TReportConstructor(TReportConstructor&& other) noexcept {
        DoSwap(*this, other);
        // is not an optimization, makes ~TReportConstructor effectively noop
        other.DropRegistry();
    }

    ITraceRegistry::TReportConstructor::~TReportConstructor() {
        if (Registry) {
            Registry->ReleaseReportBlank(std::move(Blank));
        }
    }

    void ITraceRegistry::TReportConstructor::AddTagImpl(TStringBuf name) noexcept {
        const ui32 tagEventParams = WireFormatLite::MakeTag(TEventReportProto::kEventParamsFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        // tags
        const ui32 tagName = WireFormatLite::MakeTag(TEventParam::kNameFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 nameLength = name.size();
        const ui32 totalEventParamLength =
            // Name
            1 + // tagName
            CodedOutputStream::VarintSize32(nameLength) +
            nameLength;
        const size_t totalEventParamFieldLength =
            // header
            1 + // tagEventParams
            CodedOutputStream::VarintSize32(totalEventParamLength) +
            totalEventParamLength;

        ui8* current = Blank.GetCurrent(totalEventParamFieldLength);
        // header
        *current++ = tagEventParams;
        current = CodedOutputStream::WriteVarint32ToArray(totalEventParamLength, current);
        {
            // Name
            *current++ = tagName;
            current = CodedOutputStream::WriteVarint32ToArray(nameLength, current);
            std::char_traits<char>::copy((char*)current, name.data(), name.size());
            current += name.size();
        }
        Blank.SetCurrent(current);
    }
    void ITraceRegistry::TReportConstructor::AddParamImpl(TStringBuf name, TStringBuf value) noexcept {
        const ui32 tagEventParams = WireFormatLite::MakeTag(TEventReportProto::kEventParamsFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        // tags
        const ui32 tagName = WireFormatLite::MakeTag(TEventParam::kNameFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 tagStringValue = WireFormatLite::MakeTag(TEventParam::kStringValueFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 nameLength = name.size();
        const ui32 valueLength = value.size();
        const ui32 totalEventParamLength =
            // Name
            1 + // tagName
            CodedOutputStream::VarintSize32(nameLength) +
            nameLength +
            // StringValue
            1 + // tagStringValue
            CodedOutputStream::VarintSize32(valueLength) +
            valueLength;
        const ui32 totalEventParamFieldLength =
            // header
            1 + // tagEventParams
            CodedOutputStream::VarintSize32(totalEventParamLength) +
            totalEventParamLength;
        ui8* current = Blank.GetCurrent(totalEventParamFieldLength);
        // header
        *current++ = tagEventParams;
        current = CodedOutputStream::WriteVarint32ToArray(totalEventParamLength, current);
        {
            // Name
            *current++ = tagName;
            current = CodedOutputStream::WriteVarint32ToArray(nameLength, current);
            std::char_traits<char>::copy((char*)current, name.data(), name.size());
            current += name.size();
        }
        {
            // Value
            *current++ = tagStringValue;
            current = CodedOutputStream::WriteVarint32ToArray(valueLength, current);
            std::char_traits<char>::copy((char*)current, value.data(), value.size());
            current += value.size();
        }
        Blank.SetCurrent(current);
    }
    void ITraceRegistry::TReportConstructor::AddParamImpl(TStringBuf name, ui64 value) noexcept {
        const ui32 tagEventParams = WireFormatLite::MakeTag(TEventReportProto::kEventParamsFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        // tags
        const ui32 tagName = WireFormatLite::MakeTag(TEventParam::kNameFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 tagUintValue = WireFormatLite::MakeTag(TEventParam::kUintValueFieldNumber, WireFormatLite::WIRETYPE_VARINT);
        const ui32 nameLength = name.size();
        const ui32 totalEventParamLength =
            // Name
            1 + // tagName
            CodedOutputStream::VarintSize32(nameLength) +
            nameLength +
            // UintValue
            1 + // tagUintValue
            CodedOutputStream::VarintSize64(value);
        const ui32 totalEventParamFieldLength =
            // header
            1 + // tagEventParams
            CodedOutputStream::VarintSize32(totalEventParamLength) +
            totalEventParamLength;
        ui8* current = Blank.GetCurrent(totalEventParamFieldLength);
        // header
        *current++ = tagEventParams;
        current = CodedOutputStream::WriteVarint32ToArray(totalEventParamLength, current);
        {
            // Name
            *current++ = tagName;
            current = CodedOutputStream::WriteVarint32ToArray(nameLength, current);
            std::char_traits<char>::copy((char*)current, name.data(), name.size());
            current += name.size();
        }
        {
            // Value
            *current++ = tagUintValue;
            current = CodedOutputStream::WriteVarint64ToArray(value, current);
        }
        Blank.SetCurrent(current);
    }
    void ITraceRegistry::TReportConstructor::AddParamImpl(TStringBuf name, i64 value) noexcept {
        const ui32 tagEventParams = WireFormatLite::MakeTag(TEventReportProto::kEventParamsFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        // tags
        const ui32 tagName = WireFormatLite::MakeTag(TEventParam::kNameFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 tagSintValue = WireFormatLite::MakeTag(TEventParam::kSintValueFieldNumber, WireFormatLite::WIRETYPE_VARINT);
        const ui32 nameLength = name.size();
        ui64 zigZagValue = WireFormatLite::ZigZagEncode64(value);
        const ui32 totalEventParamLength =
            // Name
            1 + // tagName
            CodedOutputStream::VarintSize32(nameLength) +
            nameLength +
            // SintValue
            1 + // tagSintValue
            CodedOutputStream::VarintSize64(zigZagValue);
        const ui32 totalEventParamFieldLength =
            // header
            1 + // tagEventParams
            CodedOutputStream::VarintSize32(totalEventParamLength) +
            totalEventParamLength;
        ui8* current = Blank.GetCurrent(totalEventParamFieldLength);
        // header
        *current++ = tagEventParams;
        current = CodedOutputStream::WriteVarint32ToArray(totalEventParamLength, current);
        {
            // Name
            *current++ = tagName;
            current = CodedOutputStream::WriteVarint32ToArray(nameLength, current);
            std::char_traits<char>::copy((char*)current, name.data(), name.size());
            current += name.size();
        }
        {
            // Value
            *current++ = tagSintValue;
            current = CodedOutputStream::WriteVarint64ToArray(zigZagValue, current);
        }
        Blank.SetCurrent(current);
    }
    void ITraceRegistry::TReportConstructor::AddParamImpl(TStringBuf name, float value) noexcept {
        const ui32 tagEventParams = WireFormatLite::MakeTag(TEventReportProto::kEventParamsFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        // tags
        const ui32 tagName = WireFormatLite::MakeTag(TEventParam::kNameFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 tagFloatValue = WireFormatLite::MakeTag(TEventParam::kFloatValueFieldNumber, WireFormatLite::WIRETYPE_FIXED32);
        const ui32 nameLength = name.size();
        const ui32 totalEventParamLength =
            // Name
            1 + // tagName
            CodedOutputStream::VarintSize32(nameLength) +
            nameLength +
            // FloatValue
            CodedOutputStream::VarintSize32(tagFloatValue) +
            sizeof(value);
        const ui32 totalEventParamFieldLength =
            // header
            1 + // tagEventParams
            CodedOutputStream::VarintSize32(totalEventParamLength) +
            totalEventParamLength;
        ui8* current = Blank.GetCurrent(totalEventParamFieldLength);
        // header
        *current++ = tagEventParams;
        current = CodedOutputStream::WriteVarint32ToArray(totalEventParamLength, current);
        {
            // Name
            *current++ = tagName;
            current = CodedOutputStream::WriteVarint32ToArray(nameLength, current);
            std::char_traits<char>::copy((char*)current, name.data(), name.size());
            current += name.size();
        }
        {
            // Value
            *current++ = tagFloatValue;
            current = WireFormatLite::WriteFloatNoTagToArray(value, current);
        }
        Blank.SetCurrent(current);
    }
    void ITraceRegistry::TReportConstructor::AddParamImpl(TStringBuf name, double value) noexcept {
        const ui32 tagEventParams = WireFormatLite::MakeTag(TEventReportProto::kEventParamsFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        // tags
        const ui32 tagName = WireFormatLite::MakeTag(TEventParam::kNameFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 tagDoubleValue = WireFormatLite::MakeTag(TEventParam::kDoubleValueFieldNumber, WireFormatLite::WIRETYPE_FIXED64);
        const ui32 nameLength = name.size();
        const ui32 totalEventParamLength =
            // Name
            1 + // tagName
            CodedOutputStream::VarintSize32(nameLength) +
            nameLength +
            // DoubleValue
            1 + // tagDoubleValue
            sizeof(value);
        const ui32 totalEventParamFieldLength =
            // header
            1 + // tagEventParams
            CodedOutputStream::VarintSize32(totalEventParamLength) +
            totalEventParamLength;
        ui8* current = Blank.GetCurrent(totalEventParamFieldLength);
        // header
        *current++ = tagEventParams;
        current = CodedOutputStream::WriteVarint32ToArray(totalEventParamLength, current);
        {
            // Name
            *current++ = tagName;
            current = CodedOutputStream::WriteVarint32ToArray(nameLength, current);
            std::char_traits<char>::copy((char*)current, name.data(), name.size());
            current += name.size();
        }
        {
            // Value
            *current++ = tagDoubleValue;
            current = WireFormatLite::WriteDoubleNoTagToArray(value, current);
        }
        Blank.SetCurrent(current);
    }
    void ITraceRegistry::TReportConstructor::AddFactorsImpl(const TStringBuf* names, const float* values, size_t size) noexcept {
        const ui32 tagEventParams = WireFormatLite::MakeTag(TEventReportProto::kEventParamsFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        // tags
        const ui32 tagName = WireFormatLite::MakeTag(TEventParam::kNameFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 tagFloatValue = WireFormatLite::MakeTag(TEventParam::kFloatValueFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
        const ui32 valueSize = sizeof(float) * size;
        ui32 totalEventParamLength =
            1 * size + // size tagName tags
            1 +        // tagFloatValue
            CodedOutputStream::VarintSize32(valueSize) +
            valueSize;
        for (size_t i = 0; i < size; ++i) {
            const ui32 nameLength = names[i].size();
            totalEventParamLength +=
                CodedOutputStream::VarintSize32(nameLength) +
                nameLength;
        }
        const ui32 totalEventParamFieldLength =
            // header
            1 + // tagEventParams
            CodedOutputStream::VarintSize32(totalEventParamLength) +
            totalEventParamLength;
        ui8* current = Blank.GetCurrent(totalEventParamFieldLength);
        // header
        *current++ = tagEventParams;
        current = CodedOutputStream::WriteVarint32ToArray(totalEventParamLength, current);
        for (size_t i = 0; i < size; ++i) {
            // Names
            const TStringBuf& name = names[i];
            const ui32 nameLength = name.size();
            *current++ = tagName;
            current = CodedOutputStream::WriteVarint32ToArray(nameLength, current);
            std::char_traits<char>::copy((char*)current, name.data(), name.size());
            current += name.size();
        }
        {
            // Values
            *current++ = tagFloatValue;
            current = CodedOutputStream::WriteVarint32ToArray(valueSize, current);
            std::char_traits<char>::copy((char*)current, (const char*)values, valueSize);
            current += valueSize;
        }
        Blank.SetCurrent(current);
    }

}
