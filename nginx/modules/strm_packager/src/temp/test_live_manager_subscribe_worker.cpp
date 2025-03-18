#include "test_live_manager_subscribe_worker.h"

#include <nginx/modules/strm_packager/src/base/config.h>

namespace NStrm::NPackager::NTemp {
    // static
    void TTestLiveManagerSubscribeWorker::CheckConfig(const TLocationConfig& config) {
        Y_ENSURE(config.LiveManager);
    }

    TTestLiveManagerSubscribeWorker::TTestLiveManagerSubscribeWorker(TRequestContext& context, const TLocationConfig& config)
        : TRequestWorker(context, config, /* kaltura mode = */ false, "packager_test_live_manager_subscribe_worker:")
    {
    }

    void TTestLiveManagerSubscribeWorker::Work() {
        const TString uuid = *GetArg<TString>("uuid", true);
        const ui64 chunkIndex = *GetArg<ui64>("index", true);

        SendData({HangDataInPool("\n chunkIndex: " + ToString(chunkIndex) + "\n"), true});

        TLiveManager::TStreamData* streamdata = Config.LiveManager->Find(uuid);

        Y_ENSURE(streamdata);

        Config.LiveManager->Subscribe(
            *this,
            *streamdata,
            chunkIndex,
            [this, uuid, chunkIndex](NLiveDataProto::TStream const& info) {
                TMaybe<ui64> ChunkDuration;
                TMaybe<ui64> FirstChunkIndex;
                TMaybe<ui64> LastChunkInS3Index;
                TMaybe<ui64> LastChunkIndex;

                TMaybe<i64> diff;

                const ui64 curTime = Now().MilliSeconds();
                TMaybe<ui64> curTimeIndex;

                ChunkDuration = info.GetChunkDuration();
                FirstChunkIndex = info.GetFirstChunkIndex();
                LastChunkInS3Index = info.GetLastChunkInS3Index();
                LastChunkIndex = info.GetLastChunkIndex();

                diff = chunkIndex - *LastChunkIndex;

                curTimeIndex = curTime / info.GetChunkDuration();

                TString resp;
                TStringOutput rso(resp);
                rso
                    << "\n uuid:                               " << uuid
                    << "\n chunkIndex:                         " << chunkIndex
                    << "\n &info:                              " << (void*)&info
                    << "\n info.ChunkDuration:                 " << ChunkDuration
                    << "\n info.FirstChunkIndex:               " << FirstChunkIndex
                    << "\n info.LastChunkInS3Index:            " << LastChunkInS3Index
                    << "\n info.LastChunkIndex:                " << LastChunkIndex
                    << "\n diff:                               " << diff
                    << "\n curTimeIndex:                       " << curTimeIndex
                    << "\n curTime:                            " << curTime
                    << "\n\n";

                SendData({HangDataInPool(resp), false});
                Finish();
            });
    }

}
