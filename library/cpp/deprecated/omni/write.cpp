#include "write.h"

namespace NOmni {
    TCppEmbeddedScheme::TCppEmbeddedScheme(TString scheme) {
        Scheme = scheme;
    }

    TBlob TCppEmbeddedScheme::LoadStaticCodecTable(TString pathName) const {
        Y_UNUSED(pathName);
        //TODO implement a mechanism for embedding static tables into cpp source code
        ythrow yexception() << "static tables embedding isn't implemented yet";
    }

    TJsonFileScheme::TJsonFileScheme(TString schemePath)
        : SchemePath(schemePath)
    {
        Scheme = TUnbufferedFileInput(SchemePath).ReadAll();
    }

    TBlob TJsonFileScheme::LoadStaticCodecTable(TString pathName) const {
        TString dirName = GetDirName(SchemePath);
        TString absPath = dirName + "/" + pathName;
        TFileInput fin(absPath);
        return TBlob::FromStream(fin).DeepCopy();
    }

    const ui64 TFileArchiver::CurrentVersion;
    const char* TFileArchiver::Marker = "jsstruct";

    IStackIndexer::IStackIndexer(const ISchemeProvider& provider) {
        TString userSchemeDump = provider.GetJsonScheme();
        OmniTree.Reset(new TOmniTree(userSchemeDump));
    }

    void IStackIndexer::WriteData(size_t key, TStringBuf data) {
        TNode& cur = LevelStack.back();
        const TNodeInfo& info = cur.GetChildInfo(key);
        size_t offset = this->GetCurOffset();
        this->ArchiveData(data, info);
        size_t blobLen = this->GetCurOffset() - offset;
        cur.AppendFilePointer(TFilePointer(key, offset, blobLen));
    }

    void IStackIndexer::StartMap(size_t key) {
        TNode& cur = LevelStack.back();
        TNode child = cur.StartChild(key);
        LevelStack.push_back(child);
    }

    void IStackIndexer::Finish() {
        Y_VERIFY(LevelStack.size() == 2);
        EndMap(); //user scheme
        ui64 mainOffset, mainAtlasLen;
        DoEndMap(&mainOffset, &mainAtlasLen); //system scheme
        Y_VERIFY(LevelStack.empty());
        DoFinish(mainOffset, mainAtlasLen);
    }

    void IStackIndexer::EndMap() {
        ui64 offset, blobLen;
        DoEndMap(&offset, &blobLen);
    }

    IStackIndexer::~IStackIndexer() {
        Y_VERIFY(LevelStack.empty());
    }

    TVector<TBlob> IStackIndexer::LoadStaticTables(const ISchemeProvider& provider, const TVector<TString>& staticTablesPaths) {
        TVector<TBlob> res(staticTablesPaths.size());
        for (size_t i = 0; i < staticTablesPaths.size(); ++i) {
            if (staticTablesPaths[i].size()) {
                res[i] = provider.LoadStaticCodecTable(staticTablesPaths[i]);
                Y_VERIFY(res[i].Size());
            }
        }
        return res;
    }

    void IStackIndexer::DoEndMap(ui64* offset, ui64* blobLen) {
        TNode& cur = LevelStack.back();
        *offset = this->GetCurOffset();
        TFilePointersIter iter((ui8*)cur.CompactFilePtrs.data(), cur.CompactFilePtrs.size(), cur.NumPtrs, cur.EndOffset);
        this->ArchiveFilePtrs(iter, cur.GetInfo());
        *blobLen = this->GetCurOffset() - *offset;
        LevelStack.pop_back();
        if (LevelStack.size()) {
            LevelStack.back().FinishChild(*offset, *blobLen);
        }
    }

    TStatCollector::TStatCollector(const ISchemeProvider& provider)
        : IStackIndexer(provider)
    {
        InitDataCollectors();
        SetUpUserRoot();
    }

    const TVector<TBlob>& TStatCollector::GetComputedCodecTables() const {
        Y_VERIFY(ComputedTables.size());
        return ComputedTables;
    }

    void TStatCollector::WriteArchiverModel(TString dstDirPath) {
        Y_VERIFY(LevelStack.empty());
        if (NFs::Exists(dstDirPath))
            RemoveDirWithContents(dstDirPath);
        MakeDirIfNotExist(dstDirPath.data());
        Y_ENSURE(!OmniTree->HasStaticTables(), "Mixing old and new static tables is not supported [yet].");
        TVector<TBlob> newTables = GetComputedCodecTables();
        TVector<TString> tableNames = WriteTables(newTables, dstDirPath);
        TJsonValue pathchedScheme = OmniTree->PatchUserScheme(tableNames);

        TStringStream tmp;
        tmp << pathchedScheme;
        TString pretty = NJson::PrettifyJson(tmp.Str());
        TFixedBufferFileOutput fout(dstDirPath + "/scheme.js");
        fout << pretty << Endl;
        fout.Finish();
    }

    TVector<TString> TStatCollector::WriteTables(const TVector<TBlob>& tables, TString dstDirPath) {
        TVector<TString> names(tables.size());
        for (size_t i = 0; i < tables.size(); ++i) {
            if (tables[i].Empty())
                continue;
            names[i] = "table." + ToString(i);
            TString absPath = dstDirPath + "/" + names[i];
            TFixedBufferFileOutput fout(absPath);
            fout.Write(tables[i].Data(), tables[i].Size());
            fout.Finish();
        }
        return names;
    }

    void TStatCollector::ArchiveData(TStringBuf data, const TNodeInfo& info) {
        Y_VERIFY(info.NodeType == NT_BLOB);
        if (!Collectors[info.DfsIndex].Get())
            return;
        Collectors[info.DfsIndex]->AddStat(data);
    }

    void TStatCollector::ArchiveFilePtrs(TFilePointersIter& /*iter*/, const TNodeInfo& /*info*/) {
    }

    size_t TStatCollector::GetCurOffset() {
        return 0;
    }

    void TStatCollector::DoFinish(ui64 /*mainOffset*/, ui64 /*mainAtlasLen*/) {
        size_t n = Collectors.size();
        ComputedTables.resize(n);
        for (size_t i = 0; i < n; ++i) {
            if (Collectors[i].Get())
                ComputedTables[i] = Collectors[i]->BuildTable();
        }
    }

    void TStatCollector::InitDataCollectors() {
        TVector<TString> staticTablesPaths = OmniTree->GetStaticTablePaths();
        TVector<ECodecType> codecTypes = OmniTree->GetCodecsTypes();
        Collectors.resize(codecTypes.size());
        for (size_t i = 0; i < codecTypes.size(); ++i) {
            if (staticTablesPaths[i].empty())
                Collectors[i].Reset(CreateCollectorByType(codecTypes[i]));
        }
    }

    void TStatCollector::SetUpUserRoot() {
        LevelStack.push_back(TNode(0, OmniTree.Get()));
        StartMap(2);
    }

    TFileArchiver::TFileArchiver(const ISchemeProvider& provider, const TString& dstPath, const TVector<TBlob>* dynamicCodecTables)
        : IStackIndexer(provider)
        , DstPath(dstPath)
        , FOut(dstPath)
        , CurOffset(0)
    {
        Tables.resize(OmniTree->GetNumNodes());
        if (dynamicCodecTables)
            SetTables(*dynamicCodecTables);
        SetTables(LoadStaticTables(provider, OmniTree->GetStaticTablePaths()));
        InitCodecs(OmniTree->GetCodecsTypes());
        InitAtlases(OmniTree->GetAtlasesTypes());
        WriteMagick();
        SetUpUserRoot();
    }

    void TFileArchiver::ArchiveData(TStringBuf data, const TNodeInfo& info) {
        ICompressor* compressor = Compressors[info.DfsIndex].Get();
        compressor->Compress(data, &MemBuf);
        CurOffset += MemBuf.size();
        FOut.Write(MemBuf.data(), MemBuf.size());
    }

    void TFileArchiver::ArchiveFilePtrs(TFilePointersIter& iter, const TNodeInfo& info) {
        Y_VERIFY(info.NodeType != NT_BLOB);
        TBlob serialized = Atlases[info.DfsIndex]->Archive(iter);
        ArchiveData(TStringBuf(serialized.AsCharPtr(), serialized.Size()), info);
    }

    size_t TFileArchiver::GetCurOffset() {
        return CurOffset;
    }

    void TFileArchiver::DoFinish(ui64 mainOffset, ui64 mainAtlasLen) {
        TFilePointer mainIterval(0, mainOffset, mainAtlasLen);
        FOut.Finish();
        TFile file(DstPath, RdWr);
        file.Seek(16, sSet);
        file.Write(&mainIterval, sizeof(mainIterval));
    }

    void TFileArchiver::SetUpUserRoot() {
        LevelStack.push_back(TNode(0, OmniTree.Get()));
        WriteData(0, OmniTree->GetUserSchemeDump());
        StartMap(1);
        for (size_t i = 0; i < Tables.size(); ++i) {
            WriteData(i, TStringBuf(Tables[i].AsCharPtr(), Tables[i].Size()));
        }
        EndMap();
        StartMap(2);
    }

    void TFileArchiver::SetTables(const TVector<TBlob>& tables) {
        Y_VERIFY(tables.size() == Tables.size());
        for (size_t i = 0; i < tables.size(); ++i) {
            if (tables[i].Size()) {
                Y_VERIFY(!Tables[i].Size());
                Tables[i] = tables[i];
            }
        }
    }

    void TFileArchiver::WriteMagick() {
        FOut << Marker;
        Save(&FOut, CurrentVersion);
        TFilePointer mainFilePtrPlaceHolder(-1, -1, -1);
        mainFilePtrPlaceHolder.Save(&FOut);
        CurOffset = strlen(Marker) + sizeof(CurrentVersion) + sizeof(mainFilePtrPlaceHolder);
        Y_VERIFY(CurOffset == 40);
    }

    void TFileArchiver::InitCodecs(const TVector<ECodecType>& codecTypes) {
        Y_VERIFY(Tables.size() && Tables.size() == codecTypes.size());
        Y_VERIFY(Compressors.empty());
        Compressors.resize(codecTypes.size());
        for (size_t i = 0; i < codecTypes.size(); ++i) {
            Compressors[i].Reset(CreateCompressorByType(codecTypes[i], Tables[i]));
        }
    }

    void TFileArchiver::InitAtlases(const TVector<EAtlasType>& atlasTypes) {
        size_t n = atlasTypes.size();
        Y_VERIFY(n == Compressors.size());
        Atlases.resize(n);
        for (size_t i = 0; i < n; ++i) {
            Atlases[i].Reset(NewAtlas(atlasTypes[i]));
        }
    }

}
