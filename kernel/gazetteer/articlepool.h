#pragma once

// Immutable pool of articles in memory.
// Allows searching article by its name or identifier (contained in references).
// Could be saved/loaded to/from binary stream.
// Implements distinct strategies of memory/performance trade-off.


#include "tokenize.h" // Article marker
#include "gztarticle.h"

#include "common/binaryguard.h"
#include "common/protohelpers.h"
#include "common/serialize.h"
#include "common/secondhand.h"
#include "common/constptr.h"
#include "common/bytesize.h"
#include "common/tools.h"
#include "protoparser/sourcetree.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <library/cpp/containers/comptrie/comptrie.h>

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>
#include <util/generic/map.h>
#include <util/generic/hash.h>
#include <util/memory/blob.h>
#include <util/memory/pool.h>
#include <util/charset/wide.h>

#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/rwlock.h>
#include <util/system/mutex.h>


// forward declarations from base.proto
class TSearchKey;


namespace NGzt
{


// Defined in this file.
class TArticlePoolBuilder;
class TArticlePool;
class TArticleStorage;
class IKeyCollector;


//forward declarations
class TProtoPoolBuilder;
class TProtoPool;
class TArticlePtr;
class TGztSourceTree;
class TOptions;

// forward declarations from syntax.proto
class TGztFileDescriptorProto;
class TArticleDescriptorProto;
class TArticleFieldDescriptorProto;
class TFieldValueDescriptorProto;


// forward declarations from binary.proto
namespace NBinaryFormat {
    class TGztInfo;
    class TArticlePool;
}

const char ARTICLE_FOLDER_SEPARATOR = '/';

// abstract interface for article keys collection (implemented in TGazetteerBuilder)
class IKeyCollector
{
public:
    virtual void Collect(const NProtoBuf::Message& article, ui32 offset, const TOptions& options) = 0;
};


// String hash-table with faster insertion and less memory consumption (compared to THashMap)
template <typename TStringType, typename TValue>
class TStringDictionary
{
public:
    typedef typename TStringType::char_type TChar;
    typedef TBasicStringBuf<TChar> TStrBuf;
    typedef THashMap<TStrBuf, TValue, ::THash<TStrBuf>, ::TEqualTo<TStrBuf> > THash;
    typedef typename THash::const_iterator const_iterator;

    TStringDictionary()
        : Keys(1024)
    {
    }

    inline const_iterator End() const
    {
        return Hash.end();
    }

    inline const_iterator Find(const TStrBuf& key) const
    {
        return Hash.find(key);
    }

    TStrBuf InsertUnique(const TStrBuf& key, const TValue& value)
    {
        // insertion without uniqueness checking
        TStrBuf pool_key(Keys.Append(key.data(), key.size()), key.size());
        Hash[pool_key] = value;
        return pool_key;
    }

    TStrBuf Insert(const TStrBuf& key, const TValue& value)
    {
        typename THash::iterator existing = Hash.find(key);
        if (existing != Hash.end())
        {
            existing->second = value;
            return existing->first;
        }
        else
            return InsertUnique(key, value);
    }

    size_t ByteSize() const {
        return ::ByteSize(Keys) + ::ByteSize(Hash);
    }

private:
    TMemoryPool Keys;
    THash Hash;
};

class TArticlePoolBuilder : TNonCopyable
{
public:
    typedef NProtoBuf::Message TMessage;
    typedef NProtoBuf::Descriptor TDescriptor;

    TArticlePoolBuilder(TGztSourceTree* sourse_tree, TProtoPoolBuilder* proto_builder,
                        IKeyCollector* key_indexer);
    virtual ~TArticlePoolBuilder();


    // Parses and builds specified .gzt file with articles.
    // @filename must be a virtual name relative to SourceTree.
    // Returns false on error.
    bool FromVirtualFile(const TString& filename);

    // Same, but takes a direct filename on disk (full path or relatively to curdir)
    bool FromDiskFile(const TString& filename);

    // Parses and builds specified @text representing textual gazetteer dictionary content
    bool FromText(const TString& text);

    // Parses and builds specified @stream representing textual gazetteer dictionary content
    bool FromStream(IInputStream* input);

    // Builds already parsed gazetteer represented in @file.
    bool FromProtobuf(const TGztFileDescriptorProto& parsedFile);


    // synonym for FromVirtualFile, deprecated
    // TODO: remove
    bool LoadVirtualFile(const TString& filename) {
        return FromVirtualFile(filename);
    }
/*
    // For manual compilation
    bool AddArticle(const TSearchKey& key, const TMessage* article_data, const TUtf16String& article_title = TUtf16String());
*/

    // @names should be UTF8
    bool FindOffsetByName(const TStringBuf& name, ui32& offset) const;

    // A whole article entries pool size in bytes.
    ui32 GetTotalSize() const;


    inline const TDiskSourceTree& GetSourceTree() const {
        return *SourceTree;
    }

    // Add normalized @virtual_file to the list of dependencies of currently compiled gazetteer
    // Return true if a new dependency was added, false otherwise (when there is such dependency already)
    bool AddImportedFile(const TString& virtual_file);

    inline void AddCustomArticle(ui32 offset, const TString& keytext) {
        CustomArticles.insert(offset);
        CustomKeys.insert(keytext);
    }

    typedef TCompactTrie<wchar16, ui32> TTitleMap;
    static void BuildTitleMap(const TBlob& articleData, TBuffer& result);


    void SaveHeader(NBinaryFormat::TGztInfo& header) const;

    // Serialize pool of article entries
    // Corresponding de-serialization method is in TArticlePool constructor.
    void Save(IOutputStream* output) const;
    void Save(NBinaryFormat::TArticlePool& proto, TBlobCollection& blobs) const;

    // Parse a single article with type specified with @decriptor from @gztFile
    // and store parsed content into @message.
    static bool ParseSingleArticle(const TString gztFile, const TDescriptor& descriptor, TMessage& message);

    TString DebugByteSize() const {
        TStringStream str;
        str << "ArticlePoolBuilder:\n" << DEBUG_BYTESIZE(Articles) << DEBUG_BYTESIZE(ArticleData.Buffer());
        return str.Str();
    }

private:
    // -----------------------------------------------------------------
    // Error logging helpers

    // Invokes Errors->AddError() with the line and column number
    // of the supplied object descriptor (for now only top-level articles are supported).
    void AddError(const NProtoBuf::Message* descriptor, const TString& error);

    // Invokes Errors->AddError() with the line and column number taken from CurrentTopLevelArticle.
    void AddError(const TString& error);

    // -----------------------------------------------------------------
    // article storage helpers

    ui32 StoreArticle(const TStringBuf& name, const TMessage* article);

    // Serialize article into given @stream
    void SaveArticle(IOutputStream* output, const TStringBuf& name, const TMessage* article) const;

    // -----------------------------------------------------------------

    // Parses and imports specified file which could be either .proto with message type definitions
    // or another .dict file with articles. Returns false on error.
    bool LoadDependency(const TString& filename);
    bool LoadProto(const TString& filename);

    bool LoadToGztFileDescriptor(const TString& filename, TGztFileDescriptorProto& file);

    bool BuildFromStream(NProtoBuf::io::ZeroCopyInputStream* input);

    // Builds possibly partially parsed articles file - without reference resolution. Returns false on error.
    bool BuildPartialFile(const TGztFileDescriptorProto& file);

    // Builds protobuf Message object from article-proto and places it in Data and global namespace Global.
    // Returns false if error occurred.
    bool BuildArticle(const TArticleDescriptorProto& article_proto);

    // Fills fields of @article with values from @fields_proto according to @article's descriptor.
    // Resolves position/keyword fields.
    bool FillArticle(TMessage* article, const NProtoBuf::RepeatedPtrField<TArticleFieldDescriptorProto>& fields_proto);

    // Removes search keys from @article (to decrease article data size). Only 'repeated' keys cleared.
    void ClearArticleKeys(TMessage* article) const;

    bool VerifyUniqueName(const TStringBuf& name);
    bool CollectKeys(const TMessage& article, ui32 offset, const TStringBuf& article_name);

    // -----------------------------------------------------------------
    // fields assigning

    bool AssignPositionalValue(TMessage* article, size_t field_index, const TFieldValueDescriptorProto& value);
    bool AssignKeywordValue(TMessage* article, const TString& field_name, const TFieldValueDescriptorProto& value);
    bool AssignFieldValue(TMessage* article, const NProtoBuf::FieldDescriptor* field, const TFieldValueDescriptorProto& value);

    // Returns true if @article's all required fields are initialized, otherwise adds error and returns false.
    bool CheckRequiredFields(TMessage* article);

    // -----------------------------------------------------------------
    // reference resolution
    struct TUnresolvedRefs;

    // Setter for TRef::id (allow manipulate with both generated and runtime-built TRef descriptors)
    void SetRefId(TMessage* reference, ui32 value) const;

    // Fills @reference (which should be a sub-message of type "TRef") with index of referenced identifier in @value.
    // The referred object should be already built.
    bool ResolveReference(TMessage* reference, const TFieldValueDescriptorProto& value);


private:
    // ==================================================================
    // data-members

    //has its own ErrorCollector and SourceLocationTable!
    TErrorCollector Errors;
    bool HadErrors;

    NProtoBuf::compiler::SourceLocationTable SourceLocationTable;
    TGztSourceTree* SourceTree;

    // parser of imported .proto files, builds DescriptorPool with each user-defined article type.
    TProtoPoolBuilder* ProtoBuilder;

    // Lazily-built set of prototype objects used for current article parsing
    // These prototypes are dropped with TArticlePoolBuilder itself.
    typedef THashMap<const TDescriptor*, TMessage*> TPrototypes;
    TPrototypes Prototypes;

    // global namespace of articles: article name (utf-8 encoded) -> article info
    //typedef THashMap<TString, ui32, ::THash<TStringBuf>, ::TEqualTo<TStringBuf> > TArticleHash;
    typedef TStringDictionary<TString, ui32> TArticleHash;
    TArticleHash Articles;


    // articles protobuf-serialized data
    TBufferOutput ArticleData;

    // Pointer to top-level article-proto which is currently being constructed.
    // Used for by ErrorCollector to display error location. Valid only during BuildArticle() call.
    const TArticleDescriptorProto* CurrentTopLevelArticle;


    // All imported files (including root file where compilation started)
    // with their corresponding checksums (file name -> file md5) and builtin flag
    TMap<TString, TBinaryGuard::TDependency> LoadedFiles;

    // Here all custom keys are collected (with type=CUSTOM)
    TSet<TString> CustomKeys;
    TSet<ui32> CustomArticles;  // offsets of articles having custom keys


    // All unresolved "ref" objects groupped by name of referred object.
    // Used as temporarily storage for unresolved refs, at the end of file building all references should be resolved.
    THolder<TUnresolvedRefs> UnresolvedRefs;
    const NProtoBuf::FieldDescriptor* RuntimeTRefId;

    THolder<TOptions> CurrentOptions;

    IKeyCollector* Keys;


#ifdef GZT_DEBUG
    NTools::TPerformanceCounter ArticleCounter;
#endif
};




class TArticlePool: public TMutableRefCounted<TArticlePool>, TNonCopyable
{
public:
    typedef NProtoBuf::Message TMessage;
    typedef NProtoBuf::Descriptor TDescriptor;
    typedef NProtoBuf::io::CodedInputStream TCodedInputStream;

    TArticlePool(TProtoPool& descriptors);

    ~TArticlePool();

    void Reset(const TBlob& data);
    void Load(TMemoryInput* input);
    void Load(const NBinaryFormat::TArticlePool& proto, const TBlobCollection& blobs);


    static inline size_t MaxValidOffset() {
        return Max<i32>();       // i32 (not ui32) is a limit - to have some reserve here
    }

    static inline bool IsValidOffset(size_t offset) {
        return offset < MaxValidOffset();
    }

    inline const TProtoPool& ProtoPool() const {
        return *Descriptors;
    }

    //typedef TRecyclePool<TMessage, TMessage>::TItemPtr TMessagePtr;
    typedef TAutoPtr<TMessage> TMessagePtr;

    // Deserialize article at @offset (with or without title)
    TMessagePtr LoadArticleAtOffset(ui32 offset) const;
    TMessagePtr LoadArticleAtOffset(ui32 offset, TUtf16String& name) const;



    const TDescriptor* FindDescriptorByOffset(ui32 offset) const;
    TUtf16String FindArticleNameByOffset(ui32 offset) const;

    // Returns blob with serialized article corresponding to specified @offset
    // This is direct block of memory from gzt-binary, not a copy, so you should not modify it in any way.
    TBlob GetArticleBinaryAtOffset(ui32 offset) const;


    // Find and load article from the pool by its title.
    // If no such article found (or title index is not initialized), a false returned.
    bool FindArticleByName(const TWtringBuf& title, TArticlePtr& article) const;


    // If there is custom article with @name (has key with type CUSTOM) returns true and fills @article, false otherwise
    bool FindCustomArticleByName(const TUtf16String& name, TArticlePtr& article) const;
    const TSet<ui32>* FindCustomArticleOffsetsByType(const TDescriptor* type) const;

    // Returns true if @folder is a prefix of @article, separated with '/' from the rest part of article title.
    // For example, 'a/b' or 'a/b/' are both folders for article 'a/b/c'
    static inline bool IsArticleFolder(const TWtringBuf& folder, const TWtringBuf& article) {
        return folder.size() < article.size() && article.StartsWith(folder) &&
            ((folder.size() > 0 && ARTICLE_FOLDER_SEPARATOR == folder.back()) ||
                                   ARTICLE_FOLDER_SEPARATOR == article[folder.size()]);
    }


    typedef TArticlePoolBuilder::TTitleMap TTitleMap;

    // build title index into @result
    void BuildTitleMap(TTitleMap& result) const;
    void BuildInternalTitleMap();


    // Prints specified @article in human-readable format.
    TString DebugString(const TMessage* article, const TString title) const;

    // Prints all articles from pool to @output stream in human-readable format.
    void DebugString(IOutputStream& output) const;

    void SetGeoGraph(const void* graph) {
        GeoGraph = graph;
    }

    template<typename T>
    const T* GetGeoGraph() const {
        return static_cast<const T*>(GeoGraph);
    }

public:
    class TIterator {
    public:
        TIterator(const TArticlePool& p);
        ~TIterator();

        bool Ok() const {
            return Pool.IsOffset(Offset);
        }

        ui32 operator*() const {
            return Offset;
        }

        TArticlePtr GetArticle() const {
            return TArticlePtr(Offset, Pool);
        }

        void operator++();

    private:
        class TImpl;
        THolder<TImpl> Impl;

        const TArticlePool& Pool;
        ui32 Offset;
    };

private: //methods//

    void ResetFactory();

    bool IsOffset(ui32 offset) const {
        return offset < ArticleData.Length();
    }

private: //data//

    TBlob ArticleData;
    TIntrusivePtr<TProtoPool> Descriptors;


    class TMessageFactory;
    typedef TIntrusivePtr<TMessageFactory> TFactoryRef;
    TVector<TFactoryRef> Factory;    // for each descriptor from Descriptors


    // articles offsets with key type CUSTOM
    THashMap<TUtf16String, ui32> CustomArticles;
    THashMap<const TDescriptor*, TSet<ui32> > CustomDescriptors;


    TBlob TitleIndexData;
    THolder<TTitleMap> TitleIndex;

    const void* GeoGraph;
};



class TArticleFolderIterator
{
public:
    inline TArticleFolderIterator(const TStringBuf& name)
        : Begin(name.data()), Tail(AfterSeparator(name))
    {
    }

    inline bool Ok() const {
        return !Tail.empty();
    }

    inline void operator++ () {
        Tail = AfterSeparator(Tail);
    }

    inline TStringBuf operator* () const {
        return TStringBuf(Begin, Tail.data() - 1);
    }

private:
    inline TStringBuf AfterSeparator(const TStringBuf& str) const {
        TStringBuf l, r;
        str.Split(ARTICLE_FOLDER_SEPARATOR, l, r);
        return r;
    }

    const char* Begin;
    TStringBuf Tail;
};



} // namespace NGzt
