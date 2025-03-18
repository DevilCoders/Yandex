#include "repository.h"

#include "auth.h"
#include "commit.h"
#include "tree.h"

namespace NLibgit2 {
    TRepository::TRepository(const std::filesystem::path& path) {
        git_repository* repo;
        GitThrowIfError(git_repository_open(&repo, path.string().c_str()));
        Repo_.Reset(repo);
    }

    TRef TRepository::Head() const {
        git_reference* ref;
        GitThrowIfError(git_repository_head(&ref, Repo_.Get()));
        return TRef(ref);
    }

    TOid TRepository::HeadOid() const {
        return Head().Resolve().Target();
    }

    TOdbObject TRepository::FindFile(TStringBuf rev, const TFsPath& path) {
        TObjectDB db(*this);
        TCommit commit(*this, TOid(rev));
        return TOdbObject(db, TTreeEntry(commit.Tree(), path).Id());
    }

    void TRepository::ForEachFile(TStringBuf revision, const TFsPath &path, TFileCallback cb) {
        TCommit commit(*this, TOid(revision));
        TTree commitTree = commit.Tree();
        if (path == "") {
            TraverseTree(std::move(commitTree), path, cb);
        } else {
            TTreeEntry entry(commitTree, path);
            TTree tree(*this, entry.Id());
            TraverseTree(std::move(tree), path, cb);
        }
    }

    TDiff TRepository::DiffWithHead() {
        TCommit headCommit = TCommit(*this, HeadOid());

        git_diff* diff;

        git_diff_options opts;
        // FIXME: use GIT_DIFF_OPTIONS_VERSION upon update
        git_diff_init_options(&opts, GIT_DIFF_OPTIONS_VERSION);

        // FIXME: make diff options configurable from the interface
        opts.flags = (
                GIT_DIFF_NORMAL |
                GIT_DIFF_INCLUDE_IGNORED |
                GIT_DIFF_INCLUDE_UNTRACKED |
                GIT_DIFF_RECURSE_UNTRACKED_DIRS
        );

        GitThrowIfError(
                git_diff_tree_to_workdir(
                        &diff,
                        Repo_.Get(),
                        headCommit.Tree().Get(),
                        &opts
                )
        );

        return TDiff(diff);
    }

    void TRepository::TraverseTree(TTree&& tree, const TFsPath& path, TFileCallback cb) {
        for (TTreeEntryRef entry: tree) {
            TFsPath nextPath = path / entry.Name();
            switch (entry.FileMode()) {
                case TGitFileMode::GIT_FILEMODE_TREE: {
                    TTree nextTree(*this, entry.Id());
                    TraverseTree(std::move(nextTree), nextPath, cb);
                    break;
                }
                default: {
                    break;
                }
            }
            cb(entry, nextPath);
        }
    }

    TOid TRepository::CreateCommit(const TString& ref, const TTree& tree,
                                   const TSignature& author, const TSignature& committer,
                                   const TString& message_encoding, const TString& message, const TVector<TCommit>& parents) {
        TVector<const git_commit *> parent_commits;
        for (const TCommit& parent: parents) {
            parent_commits.emplace_back(parent.Get());
        }
        git_oid id;
        GitThrowIfError(git_commit_create(&id, Repo_.Get(), ref.c_str(), author.Get(),
                                          committer.Get(), message_encoding.c_str(), message.c_str(), tree.Get(), parent_commits.size(), parent_commits.data()));
        return id;
    }

    TOid TRepository::CommitToHead(const TTree& tree,
                                   const TSignature& author, const TSignature& committer,
                                   const TString& message_encoding, const TString& message) {
        TVector<TCommit> parents;
        try {
            parents.emplace_back(*this, HeadOid());
        } catch (TGitException& e) {
            // HEAD oid not found, it means HEAD commit is missing
            // and this is initial commit, doing nothing.
        }
        return CreateCommit("HEAD", tree, author, committer, message_encoding, message, parents);
    }

    TTree TRepository::Tree(TOid oid) {
        git_tree* tree;
        GitThrowIfError(git_tree_lookup(&tree, Repo_.Get(), oid.Get()));
        return TTree(tree);
    }

    TRepository TRepository::Clone(const TString& url, const TString& localPath,
                                   TCredentialCallback credCb, TCheckCertCallback checkCertificateCallback) {
        TRepository newRepo;
        git_repository* repo;
        git_clone_options opts;
        GitThrowIfError(git_clone_init_options(&opts, GIT_CLONE_OPTIONS_VERSION));
        opts.fetch_opts.callbacks.credentials = [](git_cred **out, const char* url,
                                                   const char* username_from_url, unsigned int allowed_types, void* payload) {
            auto credCb = reinterpret_cast<TCredentialCallback*>(payload);
            TCredentials cred = (*credCb)(TString(url), TString(username_from_url), allowed_types);
            *out = cred.Get();
            return 0;
        };
        opts.fetch_opts.callbacks.certificate_check = checkCertificateCallback;
        opts.fetch_opts.callbacks.payload = &credCb;
        GitThrowIfError(git_clone(&repo, url.c_str(), localPath.c_str(), &opts));
        newRepo.Repo_.Reset(repo);
        return newRepo;
    }

    TRepository TRepository::Clone(const std::string& url, const std::filesystem::path& localPath, git_clone_options* opts) {
        TRepository newRepo;
        git_repository* repo;
        GitThrowIfError(git_clone(&repo, url.c_str(), localPath.string().c_str(), opts));
        newRepo.Repo_.Reset(repo);
        return newRepo;
    }

    bool TRepository::BranchIsHead(const std::string& branch) {
        git_reference *raw_branch_ref = nullptr;
        if (git_branch_lookup(&raw_branch_ref, Repo_.Get(), branch.c_str(), GIT_BRANCH_LOCAL)) {
            return false;
        }
        NPrivate::THolder<git_reference, git_reference_free> branch_ref(raw_branch_ref);

        if (git_branch_is_head(branch_ref.Get())) {
            return true;
        } else {
            return false;
        }
    }

    void TRepository::CreateTrackingBranchAndSwitchTo(const std::string& remote, const std::string& branch) {
        git_reference *raw_remote_ref = nullptr;
        std::string refname = "refs/remotes/" + remote + "/" + branch;

        // Get reference on remote branch
        GitThrowIfError(git_reference_lookup(&raw_remote_ref, Repo_.Get(), refname.c_str()));
        NPrivate::THolder<git_reference, git_reference_free> remote_ref(raw_remote_ref);

        // Get head commit from remote branch
        git_annotated_commit *raw_checkout_target = nullptr;
        GitThrowIfError(git_annotated_commit_from_ref(&raw_checkout_target, Repo_.Get(), remote_ref.Get()));
        NPrivate::THolder<git_annotated_commit, git_annotated_commit_free> checkout_target(raw_checkout_target);

        // Create local branch from remote branch
        git_reference *raw_branch_ref = nullptr;
        GitThrowIfError(git_branch_create_from_annotated(&raw_branch_ref, Repo_.Get(), branch.c_str(), checkout_target.Get(), 0));
        NPrivate::THolder<git_reference, git_reference_free> branch_ref(raw_branch_ref);

        // Set HEAD on new local branch
        const char *target_head = git_reference_name(branch_ref.Get());
        GitThrowIfError(git_repository_set_head(Repo_.Get(), target_head));

        // Checkout on current HEAD
        git_checkout_options opts;
        git_checkout_init_options(&opts, GIT_CHECKOUT_OPTIONS_VERSION);
        opts.checkout_strategy = GIT_CHECKOUT_SAFE;
        GitThrowIfError(git_checkout_head(Repo_.Get(), &opts));
    }
}
