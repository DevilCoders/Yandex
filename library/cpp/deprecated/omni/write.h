#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/generic/algorithm.h>
#include <util/string/split.h>
#include <util/memory/blob.h>
#include <util/generic/ptr.h>
#include <util/folder/dirut.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_prettifier.h>

#include "atlas.h"
#include "codecs.h"

//Wiki description: https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/basesearch/indexfiles/omnidex

namespace NOmni {
    /*
 * Base class for managing json scheme and static codec tables
 */
    class ISchemeProvider {
    public:
        TString GetJsonScheme() const {
            return Scheme;
        }
        virtual TBlob LoadStaticCodecTable(TString pathName) const = 0;
        virtual ~ISchemeProvider() {
        }

    protected:
        TString Scheme;
    };

    /*
 * Class to store JSON scheme in a separate json file.
 * Path to this file must be provided during index compilation. Can contain relative paths to static tables.
 * It is more convenient to use this class during experiments with index structure codecs and atlasses
 */
    class TJsonFileScheme: public ISchemeProvider {
    public:
        explicit TJsonFileScheme(TString schemePath);
        TBlob LoadStaticCodecTable(TString pathName) const override;

    private:
        TString SchemePath;
    };

    /*
 * Class to store JSON scheme (and tables, not implemented yet) in cpp code.
 * Use scheme_to_cpp.py to convert json scheme to a cpp embedded scheme
 * For production deployment this is more convenient than using a separate scheme file/directory:
 * no need to provide a separate resource with scheme [and static tables].
 */
    class TCppEmbeddedScheme: public ISchemeProvider {
    public:
        explicit TCppEmbeddedScheme(TString scheme);
        TBlob LoadStaticCodecTable(TString pathName) const override;
    };

    /*
 * Base class for index file creation and/or generation of compression tables.
 */
    class IStackIndexer {
    public:
        /**
     * @param provider                  json scheme container. Either cpp embedded or a separate file
     */
        explicit IStackIndexer(const ISchemeProvider& provider);

        virtual ~IStackIndexer();

        /**
     * @param key                       Identifier of an object to write.
     * @param data                      Object data.
     */
        void WriteData(size_t key, TStringBuf data);

        /**
     * Starts a map object, akin to opening an xml tag.
     *
     * Example usage:
     * @code
     * indexer.StartMap(0);
     * indexer.WriteData(1, "first_object");
     * indexer.WriteData(2, "second_object");
     * indexer.EndMap();
     * @endcode
     *
     * @param                           Identifier of a map to write.
     */
        void StartMap(size_t key);

        /**
     * Closes a map object.
     */
        void EndMap();

        /**
     * Finishes writing the file and flushes the data. No modifications to the
     * data are allowed after calling this method.
     */
        void Finish();

    private:
        void DoEndMap(ui64* offset, ui64* blobLen);

    protected:
        TVector<TBlob> LoadStaticTables(const ISchemeProvider& provider, const TVector<TString>& staticTablesPaths);
        virtual void ArchiveData(TStringBuf data, const TNodeInfo& info) = 0;
        virtual void ArchiveFilePtrs(TFilePointersIter& iter, const TNodeInfo& info) = 0;
        virtual size_t GetCurOffset() = 0;
        virtual void DoFinish(ui64 mainOffset, ui64 mainAtlasLen) = 0;

    protected:
        THolder<TOmniTree> OmniTree;
        TVector<TNode> LevelStack;
    };

    /*
 * `TStatCollector` class is used for generating compression tables that will
 * later be used when creating actual onmidex index file.
 */
    class TStatCollector: public IStackIndexer {
    public:
        /**
     * @param provider                  json scheme container. Either cpp embedded or a separate file
     */
        explicit TStatCollector(const ISchemeProvider& provider);

        /**
     * @returns                         Compression tables, indexed by dfs node id.
     * @see TNodeInfo
     */
        const TVector<TBlob>& GetComputedCodecTables() const;

        /**
     * Creates a directory with generated compression tables and an index
     * scheme that refers to them.
     *
     * @param dstDirPath                Directory to generate files in.
     */
        void WriteArchiverModel(TString dstDirPath);

    protected:
        TVector<TString> WriteTables(const TVector<TBlob>& tables, TString dstDirPath);
        void ArchiveData(TStringBuf data, const TNodeInfo& info) override;
        void ArchiveFilePtrs(TFilePointersIter& iter, const TNodeInfo& info) override;
        size_t GetCurOffset() override;
        void DoFinish(ui64 mainOffset, ui64 mainAtlasLen) override;

    private:
        void InitDataCollectors();
        void SetUpUserRoot();

    private:
        TVector<TAtomicSharedPtr<IStatCollector>> Collectors;
        TVector<TBlob> ComputedTables;
    };

    /*
 * `TFileArchiver` class is used for creating omnidex index file.
 */
    class TFileArchiver: public IStackIndexer {
    public:
        static const ui64 CurrentVersion = 1;
        static const char* Marker;

        /**
     * @param provider                  json scheme container. Either cpp embedded or a separate file
     * @param dstPath                   Path to omnidex index file to create.
     * @param dynamicCodecTables        Compression tables created via `TStatCollector` that was run on the _same data_.
     * @see TStatCollector::GetComputedCodecTables
     */
        TFileArchiver(const ISchemeProvider& provider, const TString& dstPath, const TVector<TBlob>* dynamicCodecTables = nullptr);

    protected:
        void ArchiveData(TStringBuf data, const TNodeInfo& info) override;
        void ArchiveFilePtrs(TFilePointersIter& iter, const TNodeInfo& info) override;
        size_t GetCurOffset() override;
        void DoFinish(ui64 mainOffset, ui64 mainAtlasLen) override;

    private:
        void SetUpUserRoot();
        void SetTables(const TVector<TBlob>& tables);
        void WriteMagick();
        void InitCodecs(const TVector<ECodecType>& codecNames);
        void InitAtlases(const TVector<EAtlasType>& atlasNames);

    private:
        TString DstPath;
        TFixedBufferFileOutput FOut;
        size_t CurOffset;
        TVector<TAtomicSharedPtr<ICompressor>> Compressors;
        TVector<TAtomicSharedPtr<const IAtlas>> Atlases;
        TVector<TBlob> Tables;
        TVector<char> MemBuf;
    };

}
