#include "parse.h"

#include <extsearch/images/robot/library/thumbid/md5.h>
#include <extsearch/images/kernel/imgformat/formatfilter.h>
#include <quality/functionality/content_plugins/lib/kiwi/kiwi.h>
#include <yweb/robot/logel/logelutil.h>

#include <util/system/yassert.h>

namespace NSnippets {
    static const size_t MAX_IMAGE_RESPONSE_SIZE = 16 << 20;

    void ParseThumb(TZoraThumb& res, const TLogel<TUnknownRec>* logel) {
        const TUnkLogel* const unkLogel = logel;
        Y_VERIFY(!!unkLogel, "unkLogel is nullptr.");
        const std::pair<const char*, size_t> crcData = NLogUtil::StreamField(unkLogel, PsImgCrc, "");
        res.ImageCRC.assign(crcData.first, crcData.second);
        const std::pair<const char*, size_t> data = NLogUtil::StreamField(unkLogel, PsThumbnail, "");
        res.Thumbnail.assign(data.first, data.second);
        const std::pair<const char*, size_t> info = NLogUtil::StreamField(unkLogel, PsImgAttrs, "");
        res.ImageInfoBlob.assign(info.first, info.second);
        res.MimeType = static_cast<MimeTypes>(NLogUtil::MimeType(logel));
    }

    void ParseThumbFromOrigDoc(TZoraThumb& res, const TLogel<TUnknownRec>* logel) {
        TLogel<TUnknownRec>::TConstIterator it = logel->Find(PsOrigDoc);
        if (it == logel->End()) {
            return;
        }
        MimeTypes mimeType = static_cast<MimeTypes>(NLogUtil::MimeType(logel));
        if (!NImages::IsImageFormat(mimeType)) {
            return;
        }
        TString content;
        TStringOutput out(content);
        if (!NContentPlugins::DocContentsFromHttpResponse(it->Data(), it->Size, out, false)) {
            return;
        }
        if (content.size() > MAX_IMAGE_RESPONSE_SIZE) {
            return;
        }
        res.Thumbnail = content;
        res.ImageCRC = CalcMD5Sum(content);
        res.MimeType = mimeType;
    }
}
