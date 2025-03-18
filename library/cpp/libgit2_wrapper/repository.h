#pragma once

#include "callbacks.h"
#include "diff.h"
#include "exception.h"
#include "index.h"
#include "object_db.h"
#include "object_id.h"
#include "reference.h"
#include "signature.h"

#include <contrib/libs/libgit2/include/git2.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <filesystem>

namespace NLibgit2 {
    class TTree;
    class TCommit;

    class TRepository {
    public:
        TRepository() = default;
        TRepository(TRepository&& other) noexcept {
            Swap(other);
        }
        TRepository& operator=(TRepository&& other) noexcept {
            Swap(other);
            return *this;
        }
    public:
        explicit TRepository(const std::filesystem::path& path);

        explicit TRepository(const TString& path) {
            git_repository* repo;
            GitThrowIfError(git_repository_open(&repo, path.c_str()));
            Repo_.Reset(repo);
        }

        static TRepository Init(const std::filesystem::path& path, bool bare = true) {
            TRepository newRepo;
            git_repository* repo;
            GitThrowIfError(git_repository_init(&repo, path.string().c_str(), bare));
            newRepo.Repo_.Reset(repo);
            return newRepo;
        }

        static TRepository Init(const TString& path, bool bare = true) {
            TRepository newRepo;
            git_repository* repo;
            GitThrowIfError(git_repository_init(&repo, path.c_str(), bare));
            newRepo.Repo_.Reset(repo);
            return newRepo;
        }

        static TRepository Clone(const TString& url, const TString& localPath,
                                 TCredentialCallback credCb, TCheckCertCallback checkCertificateCallback);
        static TRepository Clone(const std::string& url, const std::filesystem::path& localPath, git_clone_options* opts);

        bool BranchIsHead(const std::string& branch);
        void CreateTrackingBranchAndSwitchTo(const std::string& remote, const std::string& branch);

        // WARN: includes untracked and ignored files
        TDiff DiffWithHead();

        TRef Head() const;

        TOid HeadOid() const;

        TOid CreateCommit(const TString& ref, const TTree& tree,
                          const TSignature& author, const TSignature& committer,
                          const TString& message_encoding, const TString& message,
                          const TVector<TCommit> &parents);

        TOid CommitToHead(const TTree& tree,
                          const TSignature& author, const TSignature& committer,
                          const TString& message_encoding, const TString& message);

        [[nodiscard]] git_repository* Get() {
            return Repo_.Get();
        }

        TOdbObject FindFile(TStringBuf rev, const TFsPath& path);

        void ForEachFile(TStringBuf revision, const TFsPath& path, TFileCallback cb);

        void Swap(TRepository& other) {
            std::swap(Repo_, other.Repo_);
        }

        TIndex Index() {
            git_index* index;
            GitThrowIfError(git_repository_index(&index, Repo_.Get()));
            return TIndex(index);
        }

        TTree Tree(TOid oid);

    private:
        NPrivate::THolder<git_repository, git_repository_free> Repo_;
        void TraverseTree(TTree&& tree, const TFsPath& path, TFileCallback cb);
    };
} // namespace NLibgit2
