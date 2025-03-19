#include "archflush.h"
#include "archconf.h"
#include <util/stream/buffer.h>
#include <kernel/tarc/iface/farcface.h>
#include <kernel/indexer/faceproc/docattrs.h>
#include <kernel/indexer/face/docinfo.h>

void TFullArchiveFlusher::FlushDoc(const TDocInfoEx* docInfo, const TFullDocAttrs& docAttrs) {
    if (!!FullArchiveWriter) {
        TBufferOutput buf;
        docAttrs.PackTo(buf, TFullDocAttrs::AttrArcFull|TFullDocAttrs::AttrErf);
        size_t blocks_num = 1;
        TBlockForFullArchiveWriter blocks[3];
        blocks[0].BlockText = docInfo->DocText;
        blocks[0].BlockTextSize = docInfo->DocSize;
        blocks[0].BlockType = FABT_ORIGINAL;
        if (docInfo->ConvSize) {
            blocks[blocks_num].BlockText = docInfo->ConvText;
            blocks[blocks_num].BlockTextSize = docInfo->ConvSize;
            blocks[blocks_num].BlockType = FABT_HTMLCONV;
            blocks_num++;
        }
        if (docInfo->AnchorData) {
            blocks[blocks_num].BlockText = docInfo->AnchorData;
            blocks[blocks_num].BlockTextSize = docInfo->AnchorSize;
            blocks[blocks_num].BlockType = FABT_ANCHORTEXT;
            blocks_num++;
        }
        FullArchiveWriter->WriteDocToFullArchive(docInfo->DocId, *docInfo->DocHeader, blocks, blocks_num,
                                                 buf.Buffer().Data(), buf.Buffer().Size());
    }
}

TFullArchiveFlusher::TFullArchiveFlusher(const TArchiveProcessorConfig* cfg) {
    FullArchiveHolder.Reset(FullArchive = new TFixedBufferFileOutput((cfg->Archive + "tag").data()));
    Init(true);
}

TFullArchiveFlusher::TFullArchiveFlusher(IOutputStream* fullArchive, bool writeHeader) {
    FullArchive = fullArchive;
    Init(writeHeader);
}

void TFullArchiveFlusher::Init(bool writeHeader) {
    if (writeHeader)
        WriteTextArchiveHeader(*FullArchive);
    FullArchiveWriter.Reset(new TFullArchiveWriter(*FullArchive));
}

TFullArchiveFlusher::~TFullArchiveFlusher() {
}

void TFullArchiveFlusher::Term() {
    FullArchiveWriter.Destroy();
    FullArchiveHolder.Destroy();
}
