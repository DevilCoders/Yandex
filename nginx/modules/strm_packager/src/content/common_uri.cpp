#include <nginx/modules/strm_packager/src/content/common_uri.h>

#include <util/string/split.h>

namespace NStrm::NPackager {
    TCommonChunkInfo ParseCommonChunkInfo(const TStringBuf chunk) {
        const TVector<TStringBuf> chunkParts = StringSplitter(chunk).SplitBySet("-.");
        Y_ENSURE_EX(chunkParts.size() >= 3, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad uri : chunkParts.size(): " << chunkParts.size() << " <= 2");

        TCommonChunkInfo chunkInfo;

        if (chunkParts.back() == "vtt") {
            Y_ENSURE_EX(chunkParts[0] == "seg" || chunkParts[0] == "fragment", THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "vtt without seg or fragment '" << chunk << "'");
            chunkInfo.IsInit = false;
            chunkInfo.Container = EContainer::WEBVTT;
        } else if (chunkParts[0] == "init") {
            Y_ENSURE_EX(chunkParts.back() == "mp4", THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "init without .mp4 '" << chunk << "'");
            chunkInfo.ChunkIndex = 0;
            chunkInfo.IsInit = true;
            chunkInfo.Container = EContainer::MP4;
        } else if (chunkParts[0] == "fragment") {
            Y_ENSURE_EX(chunkParts.back() == "m4s", THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "fragment without .mp4 '" << chunk << "'");
            chunkInfo.IsInit = false;
            chunkInfo.Container = EContainer::MP4;
        } else if (chunkParts[0] == "seg") {
            Y_ENSURE_EX(chunkParts.back() == "ts", THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "seg without .ts '" << chunk << "'");
            chunkInfo.IsInit = false;
            chunkInfo.Container = EContainer::TS;
        } else {
            ythrow THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad chunk name '" << chunk << "'";
        }

        size_t ci = 1;
        if (!chunkInfo.IsInit) {
            Y_ENSURE_EX(TryFromString<ui64>(chunkParts[1], chunkInfo.ChunkIndex) && chunkInfo.ChunkIndex > 0, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad chunk index '" << chunk << "'");
            ci = 2;
        }

        for (; ci + 1 < chunkParts.size(); ++ci) {
            const TStringBuf& p = chunkParts[ci];
            Y_ENSURE_EX(p.Size() > 1, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad chunk '" << chunk << "'");

            ui32 value = 0;

            Y_ENSURE_EX(TryFromString<ui32>(TStringBuf(p.Data() + 1, p.Size() - 1), value), THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad chunk '" << chunk << "'");

            if (p[0] == 'v') {
                chunkInfo.Video = value;
            } else if (p[0] == 'a') {
                chunkInfo.Audio = value;
            } else if (p[0] == 'f' || p[0] == 'x') {
                // nothing
            } else if (p[0] == 'p') {
                chunkInfo.PartIndex = value;
            } else if (p[0] == 's') {
                chunkInfo.Subtitle = value;
            } else {
                ythrow THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad chunk '" << chunk << "'  p[0]: " << p[0];
            }
        }

        Y_ENSURE_EX(chunkInfo.Video.GetOrElse(1) >= 1, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad video index");
        Y_ENSURE_EX(chunkInfo.Audio.GetOrElse(1) >= 1, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad audio index");
        Y_ENSURE_EX(chunkInfo.Subtitle.GetOrElse(1) == 1, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad subtitle index");

        return chunkInfo;
    }

}
