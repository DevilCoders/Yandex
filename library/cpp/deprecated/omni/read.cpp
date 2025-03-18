#include "read.h"
#include <util/string/cast.h>

namespace NOmni {
    TOmniIterator::TOmniIterator(const TOmniReader* owner, size_t dfsIdx, size_t size, size_t offset, size_t len, size_t key)
        : Owner(owner)
        , DfsIdx(dfsIdx)
        , Size(size)
        , Offset(offset)
        , Length(len)
        , Key(key)
    {
    }

    TOmniIterator TOmniIterator::GetByKey(size_t key) const {
        return Owner->GetChildIterByKey(*this, key);
    }

    bool TOmniIterator::HasKey(size_t key) const {
        return this->GetByKey(key).Valid();
    }

    TOmniIterator TOmniIterator::GetByPos(size_t pos) const {
        return Owner->GetChildIterByPos(*this, pos);
    }

    void TOmniIterator::GetData(TVector<char>* dst) const {
        Owner->GetData(*this, dst);
    }

    void TOmniIterator::DbgPrint(IOutputStream& dst, const TMap<TString, IDebugViewer*>* customViewers) const {
        THolder<TVector<IDebugViewer*>> customDfsViewers;
        if (customViewers) {
            customDfsViewers.Reset(new TVector<IDebugViewer*>(Owner->OmniTree->GetNumNodes(), nullptr));
            TVector<TString> nodeNames = Owner->OmniTree->GetNodesNames();
            for (size_t i = 0; i < nodeNames.size(); ++i) {
                auto it = customViewers->find(nodeNames[i]);
                if (it != customViewers->end()) {
                    customDfsViewers->at(i) = it->second;
                }
            }
        }
        Owner->DbgPrint(dst, *this, 0, customDfsViewers.Get());
        dst.Flush();
    }

    TOmniReader::TOmniReader(TString indexPath)
        : IndexPath(indexPath)
        , KernelIter(nullptr, -1, -1, -1, -1, -1)
        , UserRoot(nullptr, -1, -1, -1, -1, -1)
    {
        SetupKernelIter();
        SetupAtlases();
        SetupCodecs();
        SetupDefaultDbgViewers();
        UserRoot = KernelIter.GetByKey(2);
    }

    void TOmniReader::SetupKernelIter() {
        IndexFile.Reset(new TMemoryMap(IndexPath));
        Data = TBlob::FromMemoryMap(*IndexFile, 0, IndexFile->Length());
        Y_VERIFY(memcmp(Data.Data(), "jsstruct", 8) == 0);
        ui64 version = *reinterpret_cast<const ui64*>(Data.AsCharPtr() + 8);
        Y_VERIFY(version == 1);
        TFilePointer mainFilePtr = *reinterpret_cast<const TFilePointer*>(Data.AsCharPtr() + 16);
        size_t kernelAtlasSize = 3;
        Y_VERIFY(mainFilePtr.Key == 0);
        THolder<const IAtlas> bootstrapAtlas(NewAtlas(AtlasNameToType(SystemAtlasName)));
        Y_VERIFY(bootstrapAtlas->GetSize(OfsToPtr(mainFilePtr.Offset), mainFilePtr.Length) == kernelAtlasSize);
        TFilePointer userSchemeFilePtr = bootstrapAtlas->GetKeyFilePtr(OfsToPtr(mainFilePtr.Offset), mainFilePtr.Length, 0);
        UserSchemeDump = ToString(ReadBlob(userSchemeFilePtr.Offset, userSchemeFilePtr.Length));
        OmniTree.Reset(new TOmniTree(UserSchemeDump));
        KernelIter = TOmniIterator(this, 0, kernelAtlasSize, mainFilePtr.Offset, mainFilePtr.Length, 0);
    }

    void TOmniReader::SetupAtlases() {
        TVector<EAtlasType> atlasTypes = OmniTree->GetAtlasesTypes();
        Atlases.resize(atlasTypes.size());
        for (size_t i = 0; i < atlasTypes.size(); ++i) {
            Atlases[i].Reset(NewAtlas(atlasTypes[i]));
        }
    }

    void TOmniReader::SetupDefaultDbgViewers() {
        TVector<TString> viewerNames = OmniTree->GetDbgViewersNames();
        Viewers.resize(viewerNames.size());
        for (size_t i = 0; i < viewerNames.size(); ++i) {
            Viewers[i].Reset(CreateDbgViewerByName(viewerNames[i]));
        }
    }

    void TOmniReader::SetupCodecs() {
        size_t n = OmniTree->GetNumNodes();
        Decompressors.resize(n);
        for (size_t i = 0; i < n; ++i) {
            Decompressors[i].Reset(CreateDecompressorByType(CT_NONE, TBlob()));
        }
        TOmniIterator tablesIter = KernelIter.GetByKey(1);
        Y_VERIFY(tablesIter.GetSize() == n);
        TVector<char> buf;
        for (size_t i = 0; i < n; ++i) {
            tablesIter.GetByKey(i).GetData(&buf);
            Tables.push_back(TBlob::Copy(buf.data(), buf.size()));
        }
        TVector<ECodecType> codecTypes = OmniTree->GetCodecsTypes();
        for (size_t i = 0; i < n; ++i) {
            Decompressors[i].Reset(CreateDecompressorByType(codecTypes[i], Tables[i]));
        }
    }

    TStringBuf TOmniReader::ReadBlob(size_t offset, size_t len) const {
        return TStringBuf(OfsToPtr(offset), len);
    }

    void TOmniReader::DbgPrint(IOutputStream& dst, const TOmniIterator& iter, size_t level, const TVector<IDebugViewer*>* customViewers) const {
        TString indent(level * 4, ' ');
        const TNodeInfo& info = OmniTree->GetInfo(iter.DfsIdx);
        if (info.NodeType == NT_BLOB) {
            TVector<char> buf;
            iter.GetData(&buf);
            TVector<TString> dbgLines;
            const IDebugViewer* dbgViewer = Viewers[info.DfsIndex].Get();
            if (customViewers && customViewers->at(info.DfsIndex))
                dbgViewer = customViewers->at(info.DfsIndex);
            dbgViewer->GetDbgLines(TStringBuf(buf.data(), buf.size()), &dbgLines);
            for (size_t i = 0; i < dbgLines.size(); ++i) {
                dst << indent << dbgLines[i] << '\n';
            }
            return;
        }
        for (size_t i = 0; i < iter.GetSize(); ++i) {
            TOmniIterator child = iter.GetByPos(i);
            dst << indent << child.GetKey() << ": {\n";
            DbgPrint(dst, child, level + 1, customViewers);
            dst << indent << "}\n";
        }
    }

    TOmniIterator TOmniReader::GetChildIterByKey(const TOmniIterator& parent, size_t key) const {
        TFilePointer keyFilePtr = Atlases[parent.DfsIdx]->GetKeyFilePtr(OfsToPtr(parent.Offset), parent.Length, key);
        if (!keyFilePtr.Valid())
            return TOmniIterator(key);
        size_t childDfs = OmniTree->GetChildDfsIdx(parent.DfsIdx, key);
        size_t grandChildrenCnt = 0;
        const TNodeInfo& info = OmniTree->GetInfo(childDfs);
        if (info.NodeType != NT_BLOB) {
            grandChildrenCnt = Atlases[info.DfsIndex]->GetSize(OfsToPtr(keyFilePtr.Offset), keyFilePtr.Length);
        }
        return TOmniIterator(this, childDfs, grandChildrenCnt, keyFilePtr.Offset, keyFilePtr.Length, key);
    }

    TOmniIterator TOmniReader::GetChildIterByPos(const TOmniIterator& parent, size_t pos) const {
        TFilePointer posFilePtr = Atlases[parent.DfsIdx]->GetPosFilePtr(OfsToPtr(parent.Offset), parent.Length, pos);
        size_t childDfs = OmniTree->GetChildDfsIdx(parent.DfsIdx, pos);
        size_t grandChildrenCnt = 0;
        const TNodeInfo& info = OmniTree->GetInfo(childDfs);
        Y_ASSERT(info.NodeType != NT_COMPOSITE_MAP || posFilePtr.Key == pos);
        if (info.NodeType != NT_BLOB) {
            grandChildrenCnt = Atlases[info.DfsIndex]->GetSize(OfsToPtr(posFilePtr.Offset), posFilePtr.Length);
        }
        return TOmniIterator(this, childDfs, grandChildrenCnt, posFilePtr.Offset, posFilePtr.Length, posFilePtr.Key);
    }

    void TOmniReader::GetData(const TOmniIterator& iter, TVector<char>* dst) const {
        const TNodeInfo& info = OmniTree->GetInfo(iter.DfsIdx);
        Y_ASSERT(info.NodeType == NT_BLOB);
        TStringBuf encoded = ReadBlob(iter.Offset, iter.Length);
        Decompressors[iter.DfsIdx]->Decompress(encoded, dst);
    }

    const TNodeInfo& TOmniIterator::NodeInfo() const {
        return Owner->OmniTree->GetInfo(DfsIdx);
    }

}
