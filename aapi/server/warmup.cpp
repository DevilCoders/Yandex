#include "warmup.h"

#include <aapi/lib/common/git_hash.h>
#include <aapi/lib/node/svn_commit.fbs.h>

#include <library/cpp/blockcodecs/stream.h>

#include <util/string/split.h>

using namespace NThreading;

namespace NAapi {

static const char DELIM = '/';

TWarmupProcessor::TWarmupProcessor(const TVector<TString>& paths, TVcsServer* server)
    : Server_(server)
    , Paths_(paths)
    , Finished_(false)
{
    std::reverse(Paths_.begin(), Paths_.end());
}

TWarmupProcessor::~TWarmupProcessor()
{ }

void TWarmupProcessor::Walk(ui64 revision) {
    auto future = GetTreeHash(revision);
    future.Wait();
    if (!future.GetValue()) {
        return;
    }

    {
        auto g(Guard(Lock_));
        while (Paths_) {
            const auto hash = GetPathHash(Paths_.back(), future.GetValue());
            if (hash) {
                Finished_ = false;
                Server_->AsyncWalk(*hash, this);
                while (!Finished_) {
                    Cond_.WaitI(Lock_);
                }
            }
            Paths_.pop_back();
        }
    }
}

bool TWarmupProcessor::Push(const TDirectories& dirs) {
    TVector<TString> hashes;
    for (const TDirectory& dir: dirs.GetDirectories()) {
        TEntry entry;
        TEntriesIter iter(dir.GetBlob().data(), TEntriesIter::EIterMode::EIM_FILES);
        while (iter.Next(entry)) {
            for (const TString& blob : entry.Blobs) {
                hashes.push_back(blob);

                if (hashes.size() >= 256) {
                    Server_->AsyncPrefetchObjects(hashes);
                    hashes.clear();
                }
            }
        }
    }
    if (hashes) {
        Server_->AsyncPrefetchObjects(hashes);
    }
    return true;
}

void TWarmupProcessor::Finish(const grpc::Status&) {
    auto g(Guard(Lock_));
    Finished_ = true;
    Cond_.Signal();
}

TFuture<TString> TWarmupProcessor::GetTreeHash(ui64 revision) {
    TString revisionHash = GitLikeHash(ToString(revision));
    auto promise = NewPromise<TString>();

    Server_->ScheduleGetObject(revisionHash, [promise] (const TString&, const TString& data, const grpc::Status&) mutable
        {
            if (data.empty()) {
                promise.SetValue(TString());
                return;
            }

            TString fbsDump;
            {
                TMemoryInput input(data.data(), data.size() - 1);
                NBlockCodecs::TDecodedInput decodedInput(&input);
                TStringOutput output(fbsDump);
                TransferData(&decodedInput, &output);
            }

            auto fbsCi = NNode::GetSvnCommit(fbsDump.data());
            promise.SetValue(
                TString(reinterpret_cast<const char*>(fbsCi->tree()->data()), fbsCi->tree()->size()));
        }
    );

    return promise.GetFuture();
}

TFuture<TMaybe<TString>> TWarmupProcessor::GetObject(const TString& hash) {
    auto promise = NewPromise<TMaybe<TString>>();
    Server_->ScheduleGetObject(hash, [promise] (const TString&, const TString& data, const grpc::Status& status) mutable
        {
            if (status.error_code() == grpc::StatusCode::OK) {
                promise.SetValue(data);
            } else {
                promise.SetValue(Nothing());
            }
        });
    return promise.GetFuture();
}

TMaybe<TString> TWarmupProcessor::GetPathHash(const TString& path, const TString& rootHash) {
    TString curdir;
    TVector<TStringBuf> parts;
    Split(path, &DELIM, parts);

    if (parts.empty()) {
        return rootHash;
    }

    auto fut1 = GetObject(rootHash);
    fut1.Wait();
    if (!fut1.GetValue()) {
        return Nothing();
    } else {
        curdir = *fut1.GetValue();
    }

    for (size_t i = 0; i + 1 < parts.size(); ++i) {
        const TStringBuf& part = parts[i];
        TEntriesIter iter(curdir.data(), TEntriesIter::EIterMode::EIM_DIRS);
        TEntry entry;
        bool found = false;
        while (iter.Next(entry)) {
            if (entry.Name == part) {
                found = true;
                auto fut2 = GetObject(entry.Hash);
                fut2.Wait();
                if (!fut2.GetValue()) {
                    return Nothing();
                } else {
                    curdir = *fut2.GetValue();
                }
                break;
            }
        }

        if (!found) {
            return Nothing();
        }
    }

    TEntriesIter iter(curdir.data());
    TEntry entry;
    while (iter.Next(entry)) {
        if (entry.Name == parts.back()) {
            return entry.Hash;
        }
    }

    return Nothing();
}

} // namespace NAapi
