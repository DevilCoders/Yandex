#pragma once

#include <kernel/search_types/search_types.h>
#include <kernel/keyinv/invkeypos/keycode.h>
#include <library/cpp/wordpos/wordpos.h>
#include <kernel/keyinv/hitlist/hitformat.h>

namespace NIndexerCore {
    namespace NIndexerDetail {

        //! represents implementation of IYndexStorage
        //! @note this implementation does not open index file and does not close it and so this class has no Close() method
        //!       if output index requires FAT use WriteFastAccessTable<TIndexReader>(IndexFile, IndexWriter.HasSubIndex());
        template <typename TOutputIndex, typename TIndexWriter>
        class TIndexStorageImpl : private TNonCopyable {
            TOutputIndex* IndexFile;
            TIndexWriter* IndexWriter;
            char PrevKeyText[MAXKEY_BUF];
            SUPERLONG PrevPos;
            const bool AlwaysRemoveDuplicateHits; // used in StorePositions() only
        public:
            //! @param indexFile                    index file must be open
            //! @param alwaysRemoveDuplicateHits    works for StorePositions() only
            TIndexStorageImpl(TOutputIndex& indexFile, TIndexWriter& indexWriter, bool alwaysRemoveDuplicateHits = false)
                : IndexFile(&indexFile)
                , IndexWriter(&indexWriter)
                , PrevPos(START_POINT)
                , AlwaysRemoveDuplicateHits(alwaysRemoveDuplicateHits)
            {
                Y_ASSERT(indexFile.IsOpen());
                *PrevKeyText = 0;
            }

            //! implementation of virtual void IYndexStorage::StorePositions()
            void StorePositions(const char* keyText, const SUPERLONG* positions, size_t posCount);

            void StoreKey(const char* key) {
                if (strcmp(PrevKeyText, key) != 0) {
                    Flush();
                    Y_ASSERT(strlen(key) < MAXKEY_BUF);
                    strcpy(PrevKeyText, key);
                }
            }
            void StoreHit(SUPERLONG hit) {
                IndexWriter->WriteHit(hit);
                PrevPos = hit; // actually PrevPos is needed for StorePositions only
                               // StoreNextHit must not be used together with StorePositions
            }
            void StoreRawHit(SUPERLONG hit) {
                IndexWriter->WriteRawHit(hit);
            }
            void StoreHits(const void* data, ui32 length, i64 count) {
                IndexWriter->WriteHits(data, length, count);
            }
            void FlushKey(const char* key) {
                IndexWriter->WriteKey(key);
                *PrevKeyText = 0; // sorting of keys is checked in StorePositions only
                                  // FlushNextKey must not be used together with StorePositions
            }
            void Flush() {
                FlushKey(PrevKeyText);
            }
        };

        inline void VerifyPosition(const char* keyText, SUPERLONG pos, SUPERLONG prevPos) {
            if (pos < prevPos) {
                Clog << "StorePositions: entries to store are not sorted\n";
                Clog << keyText;
                Clog << " [" << TWordPosition::Doc(prevPos)
                     << "."  << TWordPosition::Break(prevPos)
                     << "."  << TWordPosition::Word(prevPos)
                     << "."  << (ui32)TWordPosition::GetRelevLevel(prevPos);
                Clog << "] >= [";
                Clog << TWordPosition::Doc(pos)
                     << "."  << TWordPosition::Break(pos)
                     << "."  << TWordPosition::Word(pos)
                     << "."  << (ui32)TWordPosition::GetRelevLevel(pos);
                Clog << "]\n";
                assert(0);
            }
        }

        template <typename TOutputIndex, typename TIndexWriter>
        void TIndexStorageImpl<TOutputIndex, TIndexWriter>::StorePositions(const char* keyText, const SUPERLONG* positions, size_t posCount) {
            Y_ASSERT(positions && posCount);

#ifndef NDEBUG
            if (strcmp(keyText, PrevKeyText) < 0) {
                Clog << "StorePositions: entries to store are not sorted\n";
                Clog << "\"" << keyText << "\" < \"" << PrevKeyText << "\"\n";
                return;
            }
#endif

            const SUPERLONG* const end = positions + posCount;
            const SUPERLONG* pos = positions;
            const bool removeDuplicates = (AlwaysRemoveDuplicateHits ||
                (IndexFile->GetVersion() == YNDEX_VERSION_RAW64_HITS ? false : NeedRemoveHitDuplicates(keyText)));

            if (strncmp(keyText, PrevKeyText, sizeof(PrevKeyText)) != 0) {
                IndexWriter->WriteKey(PrevKeyText);
                strlcpy(PrevKeyText, keyText, sizeof(PrevKeyText));

                // write the first position
                if (pos != end)
                    IndexWriter->WriteHit(*pos++);
            } else {
                // check the first position
                if (removeDuplicates && *pos != PrevPos)
                    IndexWriter->WriteHit(*pos++);
            }

            if (removeDuplicates) {
                for (; pos != end; ++pos) {
                    const SUPERLONG prevPos = pos[-1];
                    if (*pos != prevPos) {
#ifndef NDEBUG
                        VerifyPosition(keyText, *pos, prevPos);
#endif
                        IndexWriter->WriteHit(*pos);
                    }
                }
            } else {
                for (; pos != end; ++pos) {
#ifndef NDEBUG
                    VerifyPosition(keyText, *pos, pos[-1]);
#endif
                    IndexWriter->WriteHit(*pos);
                }
            }

            if (posCount)
                PrevPos = pos[-1];
        }

    } // NIndexerDetail
} // NIndexerCore

