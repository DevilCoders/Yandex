#pragma once

#include <util/generic/string.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/system/filemap.h>
#include <util/generic/algorithm.h>
#include <util/string/split.h>
#include <util/memory/blob.h>

#include <library/cpp/json/json_reader.h>

namespace NOmni {
    extern const char* SystemAtlasName;

    using NJson::TJsonValue;

    /**
 * Type of a node in an index scheme
 */
    enum ENodeType {
        NT_BLOB = 0,          /**< Simple data node. */
        NT_COMPOSITE_MAP = 1, /**< A map node that can have children of different types. */
        NT_UNIFORM_MAP = 2,   /**< A map node that can have children of only one type. */
    };

    enum ECodecType {
        CT_NONE = 0,
        CT_COMPTABLE = 1,
        CT_COMPTABLE_HQ = 2,
        CT_ZLIB = 3,
        CT_MAX,
    };

    ECodecType CodecNameToType(const TString& codecName);

    enum EAtlasType {
        AT_SIMPLE = 0,
        AT_CHECKED = 1,
        AT_ARR_FLAT = 2,
        AT_ARR_NESTED = 3,
        AT_MAP_NESTED = 4,
        AT_ARR_FIXED_SIZE = 5,
        AT_MAX,
    };

    EAtlasType AtlasNameToType(const TString& atlasName);

    struct TNodeInfo {
        explicit TNodeInfo(TString name, ENodeType nodeType, size_t dfsIndex, ECodecType codecType, EAtlasType atlasType, TString viewerName)
            : Name(name)
            , NodeType(nodeType)
            , DfsIndex(dfsIndex)
            , CodecType(codecType)
            , AtlasType(atlasType)
            , ViewerName(viewerName)
        {
        }

        /*
     * Uniq name of the node. Can be used to set a debug viewer
     */
        TString Name;
        ENodeType NodeType;

        /*
     * Node id in the scheme if enumerating all the nodes in depth-first order. Also referred to as 'dfs node id'.
     */
        size_t DfsIndex;

        /**
     * Identifier of a compression algorithm. CT_NONE if the node is a map node
     */
        ECodecType CodecType;

        /**
     * Identifier of an atlas file pointer table storage algorithm. AT_MAX if the node is a data node
     */
        EAtlasType AtlasType;

        /**
     * Name of the default viewer to use when creating a debug dump of this node.
     *
     * @see IDebugViewer
     */
        TString ViewerName;
    };

    /*
 * Omni index scheme.
 */
    class TOmniTree {
    public:
        /*
     * @param userScheme                Json string containing the scheme.
     */
        TOmniTree(const TString& userScheme);
        const TNodeInfo& GetInfo(size_t dfsIdx) const;
        TString GetUserSchemeDump() const;
        size_t GetChildDfsIdx(size_t dfsIdx, size_t childKey);
        size_t GetNumNodes() const;
        bool HasStaticTables() const;
        TVector<TString> GetStaticTablePaths() const;
        TVector<ECodecType> GetCodecsTypes() const;
        TVector<EAtlasType> GetAtlasesTypes() const;
        TVector<TString> GetDbgViewersNames() const;
        TVector<TString> GetNodesNames() const;
        TJsonValue PatchUserScheme(const TVector<TString>& tablePaths) const;

    private:
        void Patch(const TVector<TString>& paths, size_t& dfsIdx, TJsonValue& js) const;
        size_t CurIndex();
        size_t GetNextIndex();
        void TraverseJsonTree(const TJsonValue& js, TSet<TString>& uniqNames);

    private:
        TJsonValue UserJs;
        TJsonValue SystemJs;
        TString UserSchemeDump;
        size_t NumNodes;
        TVector<TVector<size_t>> AdjList;
        TVector<TNodeInfo> Infos;
        TVector<TString> StaticTablePaths;
    };

    struct TFilePointer {
        TFilePointer(ui64 key, ui64 offset, ui64 len);
        bool operator<(const TFilePointer& rhs) const {
            return this->Key < rhs.Key;
        }
        ui64 End() const {
            return Offset + Length;
        }
        bool Valid() const {
            return Offset != static_cast<ui64>(-1);
        }
        void Save(IOutputStream* out) const;

    public:
        ui64 Key;
        ui64 Offset;
        ui64 Length;
    };

    namespace NPrivate {
        inline void EncodeVarint(ui64 value, ui8* output, ui8& encodedLen) {
            const ui64 c127 = 127;
            const ui64 c128 = 128;
            ui8 outputSize = 0;
            while (value > c127) {
                output[outputSize] = ((ui8)(value & c127)) | c128; //Set the next byte flag
                value >>= 7;
                outputSize++;
            }
            output[outputSize++] = ((ui8)value) & c127;
            encodedLen += outputSize;
        }

        inline ui64 DecodeVarint(const ui8* input, ui8& decodedLen) {
            const ui64 c127 = 127;
            const ui64 c128 = 128;
            ui64 ret = 0;
            ui8 i = 0;
            while (1) {
                Y_ASSERT(i < sizeof(ui64) + 2);
                ret |= (input[i] & c127) << (7 * i);
                if (!(input[i] & c128))
                    break;
                ++i;
            }
            decodedLen += i + 1;
            return ret;
        }

    }

    //TODO move impl to cpp
    struct TFilePointersIter {
        TFilePointersIter(ui8* data, size_t len, size_t numPtrs, ui64 endOffset)
            : Start(data)
            , Data(data)
            , End(Data + len)
            , NumPtrs(numPtrs)
            , Cur(0)
            , PrevKey(0)
            , PrevOffset(0)
            , EndOffset(endOffset)
        {
        }
        void Restart() {
            //TODO get rid of this method. copy iterator instead. and remove "Start" field
            Data = Start;
            Cur = 0;
            PrevKey = 0;
            PrevOffset = 0;
        }
        TFilePointer GetAndAdvance() {
            Y_ASSERT(!AtEnd());
            TFilePointer res(0, 0, 0);
            ui8 len = 0;
            res.Key = NPrivate::DecodeVarint(Data, len) + PrevKey;
            res.Offset = NPrivate::DecodeVarint(Data + len, len) + PrevOffset;
            res.Length = NPrivate::DecodeVarint(Data + len, len);
            Data += len;
            ++Cur;
            PrevKey = res.Key;
            PrevOffset = res.Offset;
            return res;
        }
        bool AtEnd() {
            Y_VERIFY((Data == End) == (Cur == NumPtrs));
            return Data == End;
        }
        size_t Size() {
            return NumPtrs;
        }
        size_t Pos() {
            return Cur;
        }
        ui64 GetEndOffset() {
            return EndOffset;
        }

    private:
        ui8* Start;
        ui8* Data;
        ui8* End;
        size_t NumPtrs;
        size_t Cur;
        ui64 PrevKey;    //for delta-encoding
        ui64 PrevOffset; //for delta-encoding
        ui64 EndOffset;
    };

    struct TNode {
        TNode(size_t dfsIndex, TOmniTree* tree);
        TNode StartChild(size_t key);
        void FinishChild(ui64 offset, ui64 len);
        const TNodeInfo& GetInfo();
        const TNodeInfo& GetChildInfo(size_t key);
        void AppendFilePointer(const TFilePointer& fptr);

    public:
        size_t DfsIndex;
        TOmniTree* Tree;
        TVector<char> CompactFilePtrs;
        bool HasStartedSubmap;
        size_t StartedSubmapKey;
        size_t NumPtrs;
        ui64 PrevKey;    //for delta-encoding
        ui64 PrevOffset; //for delta-encoding
        ui64 EndOffset;
    };

}
