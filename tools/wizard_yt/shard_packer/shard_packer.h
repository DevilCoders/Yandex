#pragma once
#include <util/generic/string.h>
#include <util/folder/path.h>
#include <util/generic/hash_set.h>
#include <util/generic/deque.h>
#include <util/generic/buffer.h>
#include <util/ysaveload.h>
#include <util/stream/mem.h>
#include <util/stream/buffer.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/system/file.h>

/**
 * Packs wizard rules' data directories into files for sending to YT
 */
class ShardPacker {
public:
    static TBuffer DirContents(const TFsPath& directory);
    static void ProcessDirContents(const TFsPath& directory, IOutputStream& consumer);
    static void RestoreDirectory(TMemoryInput* input, const TString& prefix);
private:
    static TVector<TString> GetContents(const TFsPath& rootPath);
};

