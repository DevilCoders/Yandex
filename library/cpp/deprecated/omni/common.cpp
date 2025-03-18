#include "common.h"

namespace NOmni {
    const char* SystemAtlasName = "checked_continuous";

    ECodecType CodecNameToType(const TString& codecName) {
        static const char* names[] = {"None", "Comptable", "ComptableHQ", "Zlib"};
        static_assert(Y_ARRAY_SIZE(names) == CT_MAX, "names - enums disparity");
        for (size_t i = 0; i < CT_MAX; ++i) {
            if (codecName == names[i])
                return static_cast<ECodecType>(i);
        }
        ythrow yexception() << "unknown codec name: " << codecName;
    }

    EAtlasType AtlasNameToType(const TString& atlasName) {
        static const char* names[] = {"simple_continuous", "checked_continuous", "optimal_continuous", "len_continuous", "len_map", "fixed_size_continuous"};
        static_assert(Y_ARRAY_SIZE(names) == AT_MAX, "names - enums disparity");
        for (size_t i = 0; i < AT_MAX; ++i) {
            if (atlasName == names[i])
                return static_cast<EAtlasType>(i);
        }
        ythrow yexception() << "unknown atlas name: " << atlasName;
    }

    ENodeType GetNodeType(const TJsonValue& js) {
        if (js.Has("Children")) {
            Y_VERIFY(!js.Has("Child0"), "a node can't be both uniform and composite");
            return NT_UNIFORM_MAP;
        }
        if (js.Has("Child0")) {
            return NT_COMPOSITE_MAP;
        }
        return NT_BLOB;
    }

    TJsonValue ReadScheme(const TString& path) {
        TFileInput src(path);
        TJsonValue jstree;
        Y_VERIFY(ReadJsonTree(&src, &jstree));
        return jstree;
    }

    TJsonValue ParseScheme(const TString& scheme) {
        TStringInput sin(scheme);
        TJsonValue jstree;
        Y_VERIFY(ReadJsonTree(&sin, &jstree));
        return jstree;
    }

    TJsonValue MakeIndexScheme(const TJsonValue& userScheme) {
        TJsonValue userSchemeDump(NJson::JSON_MAP);
        userSchemeDump.InsertValue("Name", "UserSchemeStr");
        userSchemeDump.InsertValue("Codec", "None");
        userSchemeDump.InsertValue("Attributes", "string,HR");

        TJsonValue codecTableSection(NJson::JSON_MAP);
        codecTableSection.InsertValue("Name", "CodecTable");
        codecTableSection.InsertValue("Codec", "None");

        TJsonValue codecsSection(NJson::JSON_MAP);
        codecsSection.InsertValue("Name", "CodecsTables");
        codecsSection.InsertValue("Atlas", SystemAtlasName);
        codecsSection.InsertValue("Children", codecTableSection);

        TJsonValue res(NJson::JSON_MAP);
        res.InsertValue("Name", "FullScheme");
        res.InsertValue("Atlas", SystemAtlasName);
        res.InsertValue("Child0", userSchemeDump);
        res.InsertValue("Child1", codecsSection);
        res.InsertValue("Child2", userScheme);

        return res;
    }

    TOmniTree::TOmniTree(const TString& userScheme)
        : UserSchemeDump(userScheme)
        , NumNodes(0)
    {
        UserJs = ParseScheme(userScheme);
        SystemJs = MakeIndexScheme(UserJs);
        TSet<TString> uniqNames;
        TraverseJsonTree(SystemJs, uniqNames);
        for (size_t i = 0; i < Infos.size(); ++i) {
            Y_VERIFY(Infos[i].DfsIndex == i);
        }
        Y_VERIFY(NumNodes == Infos.size());
        Y_VERIFY(NumNodes == StaticTablePaths.size());
        Y_VERIFY(NumNodes == AdjList.size());
    }

    const TNodeInfo& TOmniTree::GetInfo(size_t dfsIdx) const {
        return Infos[dfsIdx];
    }

    TString TOmniTree::GetUserSchemeDump() const {
        return UserSchemeDump;
    }

    size_t TOmniTree::GetChildDfsIdx(size_t dfsIdx, size_t childKey) {
        const TNodeInfo& parentInfo = Infos[dfsIdx];
        Y_ASSERT(parentInfo.NodeType != NT_BLOB);
        if (parentInfo.NodeType == NT_UNIFORM_MAP) {
            Y_ASSERT(AdjList[dfsIdx].size() == 1);
            return AdjList[dfsIdx][0];
        }
        Y_ASSERT(childKey < AdjList[dfsIdx].size());
        return AdjList[dfsIdx][childKey];
    }

    bool TOmniTree::HasStaticTables() const {
        for (size_t i = 0; i < StaticTablePaths.size(); ++i) {
            if (StaticTablePaths[i].size())
                return 1;
        }
        return 0;
    }

    TVector<TString> TOmniTree::GetStaticTablePaths() const {
        return StaticTablePaths;
    }

    size_t TOmniTree::GetNumNodes() const {
        return NumNodes;
    }

    TVector<ECodecType> TOmniTree::GetCodecsTypes() const {
        TVector<ECodecType> res(GetNumNodes(), CT_MAX);
        for (size_t i = 0; i < GetNumNodes(); ++i) {
            res[i] = GetInfo(i).CodecType;
        }
        return res;
    }

    TVector<EAtlasType> TOmniTree::GetAtlasesTypes() const {
        TVector<EAtlasType> res(GetNumNodes(), AT_MAX);
        for (size_t i = 0; i < GetNumNodes(); ++i) {
            res[i] = GetInfo(i).AtlasType;
        }
        return res;
    }

    TVector<TString> TOmniTree::GetDbgViewersNames() const {
        TVector<TString> res(GetNumNodes());
        for (size_t i = 0; i < GetNumNodes(); ++i) {
            res[i] = GetInfo(i).ViewerName;
        }
        return res;
    }

    TVector<TString> TOmniTree::GetNodesNames() const {
        TVector<TString> res(GetNumNodes());
        for (size_t i = 0; i < GetNumNodes(); ++i) {
            res[i] = GetInfo(i).Name;
        }
        return res;
    }

    TJsonValue TOmniTree::PatchUserScheme(const TVector<TString>& tablePaths) const {
        Y_VERIFY(tablePaths.size() == GetNumNodes());
        TJsonValue res = SystemJs;
        size_t dfsIdx = 0;
        Patch(tablePaths, dfsIdx, res);
        Y_VERIFY(dfsIdx == GetNumNodes());
        TString userSchemePath = "Child2";
        return res[userSchemePath];
    }

    void TOmniTree::Patch(const TVector<TString>& paths, size_t& dfsIdx, TJsonValue& js) const {
        if (paths[dfsIdx].size()) {
            js["TablePath"] = paths[dfsIdx];
        }
        ++dfsIdx;
        ENodeType nt = GetNodeType(js);
        if (nt == NT_UNIFORM_MAP) {
            Patch(paths, dfsIdx, js["Children"]);
        }
        if (nt == NT_COMPOSITE_MAP) {
            size_t childNo = 0;
            while (1) {
                TString childKey = "Child" + ToString(childNo);
                if (!js.Has(childKey))
                    break;
                Patch(paths, dfsIdx, js[childKey]);
                ++childNo;
            }
        }
    }

    size_t TOmniTree::CurIndex() {
        Y_VERIFY(NumNodes == Infos.size());
        size_t res = NumNodes;
        ++NumNodes;
        return res;
    }

    size_t TOmniTree::GetNextIndex() {
        return NumNodes;
    }

    void TOmniTree::TraverseJsonTree(const TJsonValue& js, TSet<TString>& uniqNames) {
        ENodeType nt = GetNodeType(js);
        size_t dfsIdx = CurIndex();
        TString viewerName;
        Y_VERIFY(js.Has("Name"), "Every node must have a name");
        TString name = js["Name"].GetString();
        uniqNames.insert(name);
        Y_VERIFY(uniqNames.size() == dfsIdx + 1, "Node names must be unique");
        if (js.Has("DbgViewer")) {
            viewerName = js["DbgViewer"].GetString();
        }
        StaticTablePaths.push_back(TString());
        if (js.Has("TablePath")) {
            StaticTablePaths.back() = js["TablePath"].GetString();
        }
        if (nt == NT_BLOB) {
            Y_VERIFY(js.Has("Codec"), "Every data node must provide a 'Codec' field");
            TString codecName = js["Codec"].GetString();
            ECodecType ctype = CodecNameToType(codecName);
            Infos.push_back(TNodeInfo(name, nt, dfsIdx, ctype, AT_MAX, viewerName));
            AdjList.push_back(TVector<size_t>());
            return;
        }
        if (nt == NT_UNIFORM_MAP) {
            Y_VERIFY(js.Has("Atlas"), "Every map node must provide an 'Atlas' field");
            TString atlasName = js["Atlas"].GetString();
            EAtlasType atype = AtlasNameToType(atlasName);
            Infos.push_back(TNodeInfo(name, nt, dfsIdx, CT_NONE, atype, viewerName));
            AdjList.push_back(TVector<size_t>(1, GetNextIndex()));
            TraverseJsonTree(js["Children"], uniqNames);
            return;
        }
        if (nt == NT_COMPOSITE_MAP) {
            Y_VERIFY(js.Has("Atlas"), "Every map node must provide an 'Atlas' field");
            TString atlasName = js["Atlas"].GetString();
            EAtlasType atype = AtlasNameToType(atlasName);
            Infos.push_back(TNodeInfo(name, nt, dfsIdx, CT_NONE, atype, viewerName));
            AdjList.push_back(TVector<size_t>());
            size_t childNo = 0;
            while (1) {
                TString childKey = "Child" + ToString(childNo);
                if (!js.Has(childKey))
                    break;
                AdjList[dfsIdx].push_back(GetNextIndex());
                TraverseJsonTree(js[childKey], uniqNames);
                ++childNo;
            }
        }
    }

    TNode::TNode(size_t dfsIndex, TOmniTree* tree)
        : DfsIndex(dfsIndex)
        , Tree(tree)
        , HasStartedSubmap(0)
        , StartedSubmapKey(0)
        , NumPtrs(0)
        , PrevKey(0)
        , PrevOffset(0)
        , EndOffset(0)
    {
    }

    TNode TNode::StartChild(size_t key) {
        size_t childDfs = Tree->GetChildDfsIdx(DfsIndex, key);
        Y_VERIFY(HasStartedSubmap == false);
        HasStartedSubmap = true;
        StartedSubmapKey = key;
        return TNode(childDfs, Tree);
    }

    void TNode::FinishChild(ui64 offset, ui64 len) {
        Y_VERIFY(HasStartedSubmap == true);
        HasStartedSubmap = false;
        TFilePointer fptr(StartedSubmapKey, offset, len);
        AppendFilePointer(fptr);
    }

    const TNodeInfo& TNode::GetInfo() {
        return Tree->GetInfo(DfsIndex);
    }

    const TNodeInfo& TNode::GetChildInfo(size_t key) {
        size_t childDfs = Tree->GetChildDfsIdx(DfsIndex, key);
        return Tree->GetInfo(childDfs);
    }

    void TNode::AppendFilePointer(const TFilePointer& fptr) {
        Y_VERIFY(HasStartedSubmap == false);
        ui8 buf[32];
        ui8 len = 0;
        Y_VERIFY(fptr.Key >= PrevKey);
        Y_VERIFY(fptr.Offset >= PrevOffset);
        NPrivate::EncodeVarint(fptr.Key - PrevKey, buf, len); //delta-encoding
        NPrivate::EncodeVarint(fptr.Offset - PrevOffset, buf + len, len);
        NPrivate::EncodeVarint(fptr.Length, buf + len, len);
        PrevKey = fptr.Key;
        PrevOffset = fptr.Offset;
        Y_VERIFY(len < 32);
        if (CompactFilePtrs.capacity() < CompactFilePtrs.size() + len) {
            CompactFilePtrs.reserve((size_t)(CompactFilePtrs.size() * 1.5) + len + 1);
        }
        size_t before = CompactFilePtrs.size();
        CompactFilePtrs.resize(CompactFilePtrs.size() + len);
        memcpy(CompactFilePtrs.data() + before, buf, len);
        ++NumPtrs;
        Y_VERIFY(fptr.End() >= EndOffset);
        EndOffset = fptr.End();
    }

    TFilePointer::TFilePointer(ui64 key, ui64 offset, ui64 len)
        : Key(key)
        , Offset(offset)
        , Length(len)
    {
    }

    void TFilePointer::Save(IOutputStream* out) const {
        ::Save(out, this->Key);
        ::Save(out, this->Offset);
        ::Save(out, this->Length);
    }

}
