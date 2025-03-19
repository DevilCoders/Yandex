#pragma once

#include <kernel/dups/banned_info/protos/banned_info.pb.h>

#include <util/stream/input.h>
#include <util/generic/deque.h>
#include <util/generic/fwd.h>
#include <util/generic/string.h>

namespace NDups {
    namespace NBannedInfo {
        struct TTSVImportResult {
            THolder<TBannedInfoListContent> Content;
            TDeque<TString> Log;
            size_t TrivialGroupsNum = 0;
            size_t WarningsNum = 0;
        };

        struct TImportParameters {
            bool EarlyValidation = false;
            bool SkipTrivialGroups = true;
            bool TrackContentRepeats = false;
        };

        TTSVImportResult ImportFromTSVStream(IInputStream& stream, const TImportParameters params = {});

        template<typename T>
        void RemoveDuplicatesPreservingOrder(TVector<T>& data) {
            THashSet<T> seen;
            size_t dst = 0;
            for (size_t src = 0; src < data.size(); ++src) {
                if (seen.insert(data[src]).second) {
                    if (dst != src)
                        data[dst] = std::move(data[src]);
                    dst++;
                }
            }
            data.resize(dst);
        }

    }
}
