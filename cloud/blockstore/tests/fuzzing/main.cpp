#include "starter.h"

#include <cloud/blockstore/libs/common/guarded_sglist.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/device_handler.h>
#include <cloud/blockstore/libs/service/request_helpers.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/yexception.h>
#include <util/system/types.h>
#include <util/system/yassert.h>

namespace {

////////////////////////////////////////////////////////////////////////////////

bool ReadData(
    void* dst_data,
    size_t dst_size,
    const ui8* src_data,
    size_t src_size,
    size_t& offset)
{
    if ((src_size - offset) < dst_size) {
        return false;
    }
    memcpy(dst_data, src_data + offset, dst_size);
    offset += dst_size;
    return true;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

extern "C"
int LLVMFuzzerTestOneInput(const ui8* data, size_t size)
{
    using namespace NCloud::NBlockStore;

    auto* starter = NFuzzing::TStarter::GetStarter();
    if (starter) {
        TLog& Log = starter->GetLogger();
        size_t offset = 0;
        while (offset < size) {
            for (auto& device: starter->GetDevices()) {
                ui64 from;
                ui64 length;
                if (!ReadData(&from, sizeof(from), data, size, offset) ||
                    !ReadData(&length, sizeof(length), data, size, offset))
                {
                    return 0;
                }

                ui16 sgListSize;
                if (!ReadData(
                        &sgListSize,
                        sizeof(sgListSize),
                        data,
                        size,
                        offset)) {
                    return 0;
                }

                // sgListCount 16376 and 65536 on every list size
                // max memory needed is 1GB on one process
                sgListSize &= 0x3FFF;

                TSgList sgList;
                TVector<TVector<ui8>> sgListData(sgListSize);
                for (auto& sgData: sgListData)
                {
                    ui16 sgListSize;
                    if (!ReadData(
                            &sgListSize,
                            sizeof(sgListSize),
                            data,
                            size,
                            offset)) {
                        return 0;
                    }
                    if (sgListSize != 0) {
                        sgData.resize(sgListSize);
                        sgList.emplace_back(
                            reinterpret_cast<const char *>(sgData.data()),
                            sgData.size());
                    }
                }

                uint8_t requestType;
                if (!ReadData(
                        &requestType,
                        sizeof(requestType),
                        data,
                        size,
                        offset)) {
                    return 0;
                }
                Log << TLOG_DEBUG << "Fuzzer parameters: "
                     <<((requestType & 0x1) ? "Read" : "Write")
                     << " from " << from
                     << " length " << length
                     << " sgList " << sgList.size();
                for(const auto& sg: sgList) {
                    Log << TLOG_DEBUG << " sg " << sg.Size();
                }

                if (requestType & 0x1) {
                    TString checkpointId;
                    auto result = device->Read(
                        MakeIntrusive<TCallContext>(CreateRequestId()),
                        from,
                        length,
                        TGuardedSgList(sgList),
                        checkpointId);

                    result.GetValueSync();
                } else {
                    auto result = device->Write(
                        MakeIntrusive<TCallContext>(CreateRequestId()),
                        from,
                        length,
                        TGuardedSgList(sgList));

                    result.GetValueSync();
                }
            }
        }
    }
    return 0;
}

