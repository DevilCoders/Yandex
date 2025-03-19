#pragma once

#include <util/generic/noncopyable.h>
#include <util/system/file.h>

namespace NIndexerCore {

    template <typename TStream> class TOutputIndexFileImpl;

    namespace NIndexerDetail {

        //! hides implementation of FAT writing (as well as presence of subindex and writing version info)
        //! @note works with TOutputIndexFileImpl<TFile> only
        class TFastAccessTableWriter : private TNonCopyable {
        public:
            const bool HasSubIndex;
        public:
            explicit TFastAccessTableWriter(bool hasSubIndex)
                : HasSubIndex(hasSubIndex)
            {
            }
            //! writes version, FAT and 12-bytes information block to inv-file
            //! @note this member function can be used for writing FAT of custom indices that
            //!       have key format: char[MAXKEY_BUF] (key text), i32 (count), i32 (length)
            void WriteFAT(TOutputIndexFileImpl<TFile>& indexFile) const;
        };

        class TNoFastAccessTableWriter {
        public:
            explicit TNoFastAccessTableWriter(bool) {
            }
            template <typename TStream>
            void WriteFAT(TOutputIndexFileImpl<TStream>&) const {
            }
        };

    } // NIndexerDetail

    //! writes FAT to the index file and specifies that index has sub index (actually if index file has sub index then it is already written)
    void WriteFastAccessTable(TOutputIndexFileImpl<TFile>& indexFile, bool hasSubIndex);

} // NIndexerCore
