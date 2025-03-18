#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/system/filemap.h>
#include <util/generic/algorithm.h>
#include <util/string/split.h>
#include <util/memory/blob.h>

#include <library/cpp/json/json_reader.h>

#include "atlas.h"
#include "codecs.h"
#include "dbg_view.h"

// Description: https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/basesearch/indexfiles/omnidex

namespace NOmni {
    class TOmniReader;

    /**
 * Omnidex tree iterator.
 *
 * An iterator might point to one of three node types:
 * - Data node.
 * - Array node.
 * - Map node.
 *
 */
    struct TOmniIterator {
    public:
        explicit TOmniIterator(size_t key = -1)
            : Key(key)
        {
        }

        /**
     * Returns an iterator to a subtree with the specified key
     * This method should be used if map keys are known in advance i.e. docids in range [0, max_doc_id]
     *
     * @param                           Identifier of a subtree to access
     */
        TOmniIterator GetByKey(size_t key) const;

        /**
     * Returns an iterator to a subtree at the specific position in the list
     * This method should be used for iteration and when the keys are not known in advance.
     *
     * @param                           Ordinal number of a subtree in the list
     */
        TOmniIterator GetByPos(size_t pos) const;

        /**
     * Returns true if a subtree with the specified key exists
     *
     * @param                           Identifier of a subtree
     */
        bool HasKey(size_t key) const;

        /**
     * Returns number of subtrees in the list
     */
        size_t GetSize() const {
            return Size;
        }

        /**
     * Returns current subtree key in parent's map
     */
        size_t GetKey() const {
            return Key;
        }

        /**
     * Get data if current node is a leaf. Undefined behavior otherwise
     *
     * @param                           Where to write the data
     */
        void GetData(TVector<char>* dst) const;

        /**
     * Dump subtree rooted at current iterator position in human-readable debug format
     *
     * @param dst                       Output stream to dump debug text
     * @param customViewers             User provided map NodeName -> CustomDebugViewer to dump uncommon data types
     */
        void DbgPrint(IOutputStream& dst, const TMap<TString, IDebugViewer*>* customViewers = nullptr) const;

        /**
     * Check if iterator is valid
     */
        bool Valid() const {
            return Offset != static_cast<size_t>(-1);
        }

        const TNodeInfo& NodeInfo() const;

    private:
        friend TOmniReader;
        TOmniIterator(const TOmniReader* owner, size_t dfsIdx, size_t size, size_t offset, size_t len, size_t key);

    private:
        const TOmniReader* Owner = nullptr;
        size_t DfsIdx = 0;
        size_t Size = 0;
        size_t Offset = size_t(-1);
        size_t Length = 0;
        size_t Key = size_t(-1);
    };

    /**
 * Main class for Omnidex reading
 */
    class TOmniReader {
        friend TOmniIterator;

    public:
        /**
     * Constructor
     *
     * @param                           Path to the index
     */
        TOmniReader(TString indexPath);

        /**
     * Returns an iterator to the root of the index tree
     */
        TOmniIterator Root() const {
            return UserRoot;
        }

        /**
     * Returns textual reperesentation of json scheme which was used for index generation
     */
        TString GetUserSchemeDump() const {
            return UserSchemeDump;
        }

    private:
        void SetupKernelIter();
        void SetupAtlases();
        void SetupDefaultDbgViewers();
        void SetupCodecs();
        const char* OfsToPtr(size_t offset) const {
            return Data.AsCharPtr() + offset;
        }
        TStringBuf ReadBlob(size_t offset, size_t len) const;
        void DbgPrint(IOutputStream& dst, const TOmniIterator& iter, size_t level, const TVector<IDebugViewer*>* customViewers) const;
        TOmniIterator GetChildIterByKey(const TOmniIterator& parent, size_t key) const;
        TOmniIterator GetChildIterByPos(const TOmniIterator& parent, size_t pos) const;
        void GetData(const TOmniIterator& iter, TVector<char>* dst) const;

    private:
        TVector<TAtomicSharedPtr<const IAtlas>> Atlases;
        TVector<TBlob> Tables;
        TVector<TAtomicSharedPtr<IDecompressor>> Decompressors;
        TVector<TAtomicSharedPtr<IDebugViewer>> Viewers;
        THolder<TOmniTree> OmniTree;
        THolder<TMemoryMap> IndexFile;
        TBlob Data;
        TString IndexPath;
        TOmniIterator KernelIter;
        TOmniIterator UserRoot;
        TString UserSchemeDump;
    };

}
