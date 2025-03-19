#pragma once

#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/variant.h>
#include <util/memory/blob.h>
#include <util/system/file.h>
#include <util/system/mutex.h>
#include <util/system/types.h>

#include <atomic>
#include <variant>


namespace NFileMapper {

using TBytes = ui64;
using TInode = ui64;

struct TFileMapperStats {
    const TBytes MappedLimit = 0;
    const TBytes MappedSize = 0;
    const TBytes AdditionalSize = 0; // MappedLimit < MappedSize + AdditionalSize
};

struct TFileMapperLimitReachedError : public yexception, public TFileMapperStats {
    using yexception::yexception;
};

enum class EMapMode {
    Unlocked,
    Locked,
};

class TFileMapper : private TNonCopyable {
public:
    struct TOptions {
        TBytes MappedLimit = Max<TBytes>();
        TMaybe<EMapMode> OverriddenMapMode = Nothing();
    };

public:
    TFileMapper(TOptions options);
    ~TFileMapper();

    // For monitoring purposes only
    TBytes MappedSize() const;
    TBytes MappedLimit() const;

    TMaybe<TBlob> TryMapFile(const TString& path, EMapMode mode);
    TMaybe<TBlob> TryMapFile(const TFsPath& path, EMapMode mode);
    TMaybe<TBlob> TryMapFile(const TFile& file, EMapMode mode);

    TBlob MapFileOrThrow(const TString& path, EMapMode mode);
    TBlob MapFileOrThrow(const TFsPath& path, EMapMode mode);
    TBlob MapFileOrThrow(const TFile& file, EMapMode mode);

    // Drop unused files now.
    // By default, unused files are released on each MapFile() call.
    void ReleaseUnusedFiles();

private:
    void ReleaseFile(TInode inode);
    void ReleaseUnusedFilesImpl() /* REQUIRES(LockerMutex_) */;

    std::variant<TBlob, TFileMapperStats> TryMapFileImpl(const TFile& file, EMapMode mode);

    class TMappedBlobBase;

private:
    const TBytes MappedLimit_;
    std::atomic<TBytes> Mapped_ = 0;

    TMaybe<EMapMode> OverriddenMapMode_;

    TMutex LockerMutex_;
    THashMap<TInode, TIntrusivePtr<TMappedBlobBase>> FilesByInode_;

    TMutex ReleaseQueueMutex_;
    TVector<TInode> ReleaseQueue_;
};

} // namespace NFileMapper
