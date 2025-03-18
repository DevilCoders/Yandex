#include "shard_packer.h"

TVector<TString> ShardPacker::GetContents(const TFsPath& rootPath) {
    THashSet<TString> discovered;
    TDeque<TString> queue;
    discovered.insert(rootPath);
    queue.push_back(rootPath);
    while (!queue.empty()) {
        auto name = queue.front();
        queue.pop_front();
        auto fsPath = TFsPath(name);
        if (fsPath.IsDirectory()) {
            TVector<TString> contents;
            fsPath.ListNames(contents);
            for (const auto& child: contents) {
                auto path = name + TString("/") + child;
                if (!discovered.contains(path)) {
                    discovered.insert(path);
                    queue.push_back(path);
                }
            }
        }
    }
    TVector<TString> result;
    for (const auto& path: discovered) {
        result.push_back(path);
    }
    std::sort(result.begin(), result.end());
    return result;
}

TBuffer ShardPacker::DirContents(const TFsPath& directory) {
    auto contents = GetContents(directory);
    size_t size = 0;
    size_t count = 0;
    for (const auto& path: contents) {
        if (!TFsPath(path).IsFile())
            continue;
        TFsPath fsPath = TFsPath(path);
        size += TFileStat(path).Size;
        ++count;
    }

    TBuffer result(size);
    TBufferOutput output(result);
    ::SaveSize(&output, count);
    TBuffer buffer;
    for (size_t i = 0; i < contents.size(); ++i) {
        auto path = contents[i].substr(directory.GetPath().size() - directory.GetName().size());
        if (!TFsPath(contents[i]).IsFile())
            continue;
        ::SaveSize(&output, path.size());
        ::SavePodArray(&output, path.data(), path.size());

        auto fstat = TFileStat(contents[i]);
        buffer.Resize(fstat.Size);
        TFile(contents[i], RdOnly).Read(buffer.Data(), fstat.Size);

        ::Save<ui64>(&output, fstat.Size);
        ::SavePodArray(&output, buffer.Data(), buffer.Size());
    }

    return result;
}

void ShardPacker::ProcessDirContents(const TFsPath& directory, IOutputStream& consumer) {
    auto contents = GetContents(directory);
    size_t size = 0;
    size_t count = 0;
    for (const auto& path: contents) {
        if (!TFsPath(path).IsFile())
            continue;
        TFsPath fsPath = TFsPath(path);
        size += TFileStat(path).Size;
        ++count;
    }

    ::SaveSize(&consumer, count);
    for (size_t i = 0; i < contents.size(); ++i) {
        auto path = contents[i].substr(directory.GetPath().size() - directory.GetName().size());
        if (!TFsPath(contents[i]).IsFile())
            continue;
        ::SaveSize(&consumer, path.size());
        ::SavePodArray(&consumer, path.data(), path.size());

        auto fstat = TFileStat(contents[i]);

        ::Save<ui64>(&consumer, fstat.Size);
        TFileInput input(contents[i]);
        TransferData(&input, &consumer);
    }
    consumer.Finish();
}

void ShardPacker::RestoreDirectory(TMemoryInput* input, const TString& prefix) {
    size_t count = ::LoadSize(input);
    for (size_t i = 0; i < count; ++i) {
        size_t pathSize = ::LoadSize(input);
        TStringBuf path(input->Buf(), pathSize);
        input->Skip(pathSize);

        ui64 fileSize;
        ::Load<ui64>(input, fileSize);
        TStringBuf fileContents(input->Buf(), fileSize);
        input->Skip(fileSize);

        auto newPath = prefix + "/" + path;
        TFsPath(newPath).Parent().MkDirs();
        TFile(newPath, CreateAlways | WrOnly).Write(fileContents.data(), fileSize);
    }
}

