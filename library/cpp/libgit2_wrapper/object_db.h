#pragma once

#include "exception.h"
#include "holder.h"
#include "object_id.h"

#include <contrib/libs/libgit2/include/git2.h>

// Classes and functions for working with Git object database
namespace NLibgit2 {

    class TRepository;

    class TObjectDB {
    public:
        explicit TObjectDB(TRepository& repo);

        git_odb* Get() {
            return DB_.Get();
        }

    private:
        NPrivate::THolder<git_odb, git_odb_free> DB_;
    };

    class TObjectWStream {
    public:
        explicit TObjectWStream(TObjectDB& odb, git_off_t size, git_object_t type) {
            git_odb_stream* stream;
            GitThrowIfError(git_odb_open_wstream(&stream, odb.Get(), size, type));
            Stream_.Reset(stream);
        }

        void Write(const void* buffer, size_t len) {
            GitThrowIfError(git_odb_stream_write(Stream_.Get(), static_cast<const char*>(buffer), len));
        }

        git_oid Finalize() {
            git_oid res;
            GitThrowIfError(git_odb_stream_finalize_write(&res, Stream_.Get()));
            return res;
        }

    private:
        NPrivate::THolder<git_odb_stream, git_odb_stream_free> Stream_;
    };

    class TOdbObject {
    public:
        explicit TOdbObject(TObjectDB& db, TOid id) {
            git_odb_object* object;
            git_odb_read(&object, db.Get(), id.Get());
            OdbObject_.Reset(object);
        }

        TString Data() {
            return { reinterpret_cast<const char *>(git_odb_object_data(OdbObject_.Get())), Size()} ;
        }

        ui64 Size() {
            return git_odb_object_size(OdbObject_.Get());
        }

        git_odb_object* Get() {
            return OdbObject_.Get();
        }

    private:
        NPrivate::THolder<git_odb_object, git_odb_object_free> OdbObject_;
    };

} // namespace NLibgit2
