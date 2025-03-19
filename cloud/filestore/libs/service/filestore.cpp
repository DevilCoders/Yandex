#include "filestore.h"

#include <util/generic/vector.h>
#include <util/string/builder.h>

#include <fcntl.h>

namespace NCloud::NFileStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

using TRequest = NProto::TCreateHandleRequest;

static const TVector<std::pair<int, int>> SupportedFlags = {
    {O_CREAT,     TRequest::E_CREATE},
    {O_EXCL,      TRequest::E_EXCLUSIVE},
    {O_APPEND,    TRequest::E_APPEND},
    {O_TRUNC,     TRequest::E_TRUNCATE},
    {O_DIRECTORY, TRequest::E_DIRECTORY},
    {O_NOATIME,   TRequest::E_NOATIME},
    {O_NOFOLLOW,  TRequest::E_NOFOLLOW},
    {O_NONBLOCK,  TRequest::E_NONBLOCK},
    {O_PATH,      TRequest::E_PATH},
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

std::pair<int, int> SystemFlagsToHandle(int flags)
{
    const int mode = flags & O_ACCMODE;
    flags &= ~O_ACCMODE;

    int value = 0;
    for (const auto& [flag, proto]: SupportedFlags) {
        if (flag & flags) {
            value |= ProtoFlag(proto);
            flags &= ~flag;
        }
    }

    if (mode == O_RDWR) {
        value |= ProtoFlag(TRequest::E_READ) | ProtoFlag(TRequest::E_WRITE);
    } else if (mode == O_WRONLY) {
        value |= ProtoFlag(TRequest::E_WRITE);
    } else if (mode == O_RDONLY) {
        value |= ProtoFlag(TRequest::E_READ);
    }

    return {value, flags};
}

int HandleFlagsToSystem(int flags)
{
    int value = 0;
    for (const auto& [flag, proto]: SupportedFlags) {
        if (HasFlag(flags, proto)) {
            value |= flag;
        }
    }

    const bool read = HasFlag(flags, TRequest::E_READ);
    const bool write = HasFlag(flags, TRequest::E_WRITE);

    if (read && write) {
        value |= O_RDWR;
    } else if (write) {
        value |= O_WRONLY;
    } else if (read) {
        value |= O_RDONLY;
    }

    return value;
}

TString HandleFlagsToString(int flags)
{
    TStringBuilder ss;
    for (int flag = TRequest::EFlags_MIN; flag < TRequest::EFlags_ARRAYSIZE; ++flag) {
        if (HasFlag(flags, flag)) {
            ss << TRequest::EFlags_Name(static_cast<TRequest::EFlags>(flag)) << "|";
        }
    }

    if (ss.EndsWith("|")) {
        ss.pop_back();
    }

    return std::move(ss);
}

}   // namespace NCloud::NFileStore
