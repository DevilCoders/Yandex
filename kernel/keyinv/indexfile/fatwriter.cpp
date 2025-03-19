#include "fatwriter.h"
#include "indexfile.h"
#include "indexreader.h"

namespace NIndexerCore {
    namespace NIndexerDetail {

        void TFastAccessTableWriter::WriteFAT(TOutputIndexFileImpl<TFile>& indexFile) const {
            Y_ASSERT(indexFile.GetFormat() == IYndexStorage::FINAL_FORMAT);
            indexFile.Flush();
            indexFile.KeyStream.Seek(0, SEEK_SET); // to the beginning of key-file
            TInputIndexFile inputIndexFile(indexFile.KeyStream, indexFile.GetFormat(), indexFile.GetVersion());
            Y_ASSERT(inputIndexFile.IsOpen());
            THolder<TInvKeyReader> keyReader(new TInvKeyReader(inputIndexFile, HasSubIndex));
            NIndexerCore::WriteFastAccessTable(*keyReader, indexFile.InvStream);
            // after writing FAT positions of key- and inv-streams are at the end of files
        }

    } // NIndexerDetail

    void WriteFastAccessTable(TOutputIndexFileImpl<TFile>& indexFile, bool hasSubIndex) {
        NIndexerDetail::TFastAccessTableWriter writer(hasSubIndex);
        writer.WriteFAT(indexFile);
    }

} // NIndexerCore
