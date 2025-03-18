#include "low_mount.h"

#include <util/stream/format.h>
#include <util/string/builder.h>
#include <util/system/error.h>

#include <errno.h>
#include <sys/stat.h>

namespace NFuse {

namespace {

using TPrintConnInfo = struct fuse_init_in;

void PrintEmpty(IOutputStream&, TArgParser, const TPrintConnInfo&) {
}

void PrintSingleString(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    stream << arg.ReadStr();
}

void PrintInit(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    auto& in = arg.Read<struct fuse_init_in>();
    auto major = in.major;
    auto minor = in.minor;
    auto max_readahead = in.max_readahead;
    auto flags = Hex(in.flags);
    stream << LabeledOutput(major, minor, max_readahead, flags);
}

void PrintForget(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    auto& in = arg.Read<struct fuse_forget_in>();
    auto nlookup = in.nlookup;
    stream << LabeledOutput(nlookup);
}

void PrintBatchForget(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    auto& header = arg.Read<struct fuse_batch_forget_in>();
    auto count = header.count;
    stream << LabeledOutput(count);
    stream << ", inodes = ";
    for (size_t i = 0; i < count; i++) {
        auto& item = arg.Read<struct fuse_forget_one>();
        stream << item.nodeid;
        if (item.nlookup != 1) {
            stream << "(" << item.nlookup << ")";
        }
        stream << " ";
    }
}

void PrintAccess(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    auto& in = arg.Read<struct fuse_access_in>();
    auto mask = Hex(in.mask);
    stream << LabeledOutput(mask);
}

void PrintOpen(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    auto& in = arg.Read<struct fuse_open_in>();
    auto flags = Hex(in.flags);
    stream << LabeledOutput(flags);
}

void PrintRelease(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    auto& in = arg.Read<struct fuse_release_in>();
    auto fh = Hex(in.fh);
    auto flags = Hex(in.flags);
    auto release_flags = Hex(in.release_flags);
    auto lock_owner = in.lock_owner;
    stream << LabeledOutput(fh, flags, release_flags, lock_owner);
}

void PrintRead(IOutputStream& stream, TArgParser arg, const TPrintConnInfo& connInfo) {
    auto& in = arg.Read<struct fuse_read_in>();
    auto fh = Hex(in.fh);
    auto offset = in.offset;
    auto size = in.size;
    auto read_flags = Hex(in.read_flags);
    stream << LabeledOutput(fh, offset, size, read_flags);

    if (connInfo.minor >= 9) {
        auto lock_owner = in.lock_owner;
        auto flags = Hex(in.flags);
        stream << ", " << LabeledOutput(lock_owner, flags);
    }
}

void PrintSetAttr(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    const auto& setAttrIn = arg.Read<struct fuse_setattr_in>();
    auto valid = Hex(setAttrIn.valid);
    auto fh = Hex(setAttrIn.fh);
    auto size = setAttrIn.size;
    auto lock_owner = setAttrIn.lock_owner;
    auto atime = setAttrIn.atime;
    auto mtime = setAttrIn.mtime;
    auto atimensec = setAttrIn.atimensec;
    auto mtimensec = setAttrIn.mtimensec;
    auto mode = Hex(setAttrIn.mode);
    auto uid = setAttrIn.uid;
    auto gid = setAttrIn.gid;
    stream << LabeledOutput(valid, fh, size, lock_owner, atime,
                            mtime, atimensec, mtimensec, mode, uid, gid);
}

void PrintWrite(IOutputStream& stream, TArgParser arg, const TPrintConnInfo& connInfo) {
    const auto& in = arg.Read<struct fuse_write_in>();
    auto fh = Hex(in.fh);
    auto offset = in.offset;
    auto size = in.size;
    auto write_flags = Hex(in.write_flags);
    stream << LabeledOutput(fh, offset, size, write_flags);

    if (connInfo.minor >= 9) {
        auto lock_owner = in.lock_owner;
        auto flags = Hex(in.flags);
        stream << ", " << LabeledOutput(lock_owner, flags);
    }
}

void PrintFlush(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    const auto& writeIn = arg.Read<struct fuse_flush_in>();
    auto fh = Hex(writeIn.fh);
    auto lock_owner = writeIn.lock_owner;
    stream << LabeledOutput(fh, lock_owner);
}

void PrintFSync(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    const auto& fsyncIn = arg.Read<struct fuse_fsync_in>();
    auto fh = Hex(fsyncIn.fh);
    auto fsync_flags = Hex(fsyncIn.fsync_flags);
    stream << LabeledOutput(fh, fsync_flags);
}

void PrintFAllocate(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    const auto& fAllocateIn = arg.Read<struct fuse_fallocate_in>();
    auto fh = Hex(fAllocateIn.fh);
    auto offset = fAllocateIn.offset;
    auto length = fAllocateIn.length;
    auto mode = Hex(fAllocateIn.mode);
    stream << LabeledOutput(fh, offset, length, mode);
}

void PrintMkNod(IOutputStream& stream, TArgParser arg, const TPrintConnInfo& connInfo) {
    const auto& in = arg.Read<struct fuse_mknod_in>();
    auto mode = Hex(in.mode);
    auto rdev = Hex(in.rdev);
    stream << LabeledOutput(mode, rdev);
    if (connInfo.minor >= 12) {
        auto umask = Hex(in.umask);
        stream << ", " << LabeledOutput(umask);
    }
}

void PrintMkDir(IOutputStream& stream, TArgParser arg, const TPrintConnInfo& connInfo) {
    const auto& in = arg.Read<struct fuse_mkdir_in>();
    auto mode = Hex(in.mode);
    auto name = arg.ReadStr();
    stream << LabeledOutput(mode, name);
    if (connInfo.minor >= 12) {
        auto umask = Hex(in.umask);
        stream << ", " << LabeledOutput(umask);
    }
}

void PrintCreate(IOutputStream& stream, TArgParser arg, const TPrintConnInfo& connInfo) {
    const auto& in = arg.Read<struct fuse_create_in>();
    auto flags = Hex(in.flags);
    auto mode = Hex(in.mode);
    stream << LabeledOutput(flags, mode);
    if (connInfo.minor >= 12) {
        auto umask = Hex(in.umask);
        stream << ", " << LabeledOutput(umask);
    }
}

void PrintSymlink(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    auto name = arg.ReadStr();
    auto link = arg.ReadStr();
    stream << LabeledOutput(name, link);
}

void PrintLink(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    const auto& linkIn = arg.Read<struct fuse_link_in>();
    auto oldnodeid = linkIn.oldnodeid;
    auto name = arg.ReadStr();
    stream << LabeledOutput(oldnodeid, name);
}

void PrintRename(IOutputStream& stream, TArgParser arg, const TPrintConnInfo& connInfo) {
#ifdef _darwin_
    if (connInfo.flags & FUSE_RENAME_SWAP) {
        const auto& renameIn = arg.Read<struct fuse_rename_in>();
        auto newDir = renameIn.newdir;
        auto name = TStringBuf(arg.ReadStr());
        auto newName = TStringBuf(arg.ReadStr());
        auto flags = renameIn.flags;
        stream << LabeledOutput(newDir, name, newName, flags);
    } else {
        auto newDir = arg.Read<ui64>();
        auto name = TStringBuf(arg.ReadStr());
        auto newName = TStringBuf(arg.ReadStr());
        stream << LabeledOutput(newDir, name, newName);
    }
#else
    Y_UNUSED(connInfo);
    const auto& renameIn = arg.Read<struct fuse_rename_in>();
    auto newDir = renameIn.newdir;
    auto name = arg.ReadStr();
    auto newName = arg.ReadStr();
    stream << LabeledOutput(newDir, name, newName);
#endif
}

void PrintBmap(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    const auto& bmapIn = arg.Read<struct fuse_bmap_in>();
    auto block = bmapIn.block;
    auto blocksize = bmapIn.blocksize;
    stream << LabeledOutput(block, blocksize);
}

void PrintListXattr(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    const auto& getXattrIn = arg.Read<struct fuse_getxattr_in>();
    auto size = getXattrIn.size;
    stream << LabeledOutput(size);
}

void PrintGetXattr(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    const auto& getXattrIn = arg.Read<struct fuse_getxattr_in>();
    auto size = getXattrIn.size;
    auto attrName = arg.ReadStr();
    stream << LabeledOutput(size, attrName);
}

void PrintSetXattr(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    const auto& getXattrIn = arg.Read<struct fuse_setxattr_in>();
    auto size = getXattrIn.size;
    auto flags = getXattrIn.flags;
    auto attrName = arg.ReadStr();
    stream << LabeledOutput(size, flags, attrName);
}

void PrintFuseAttr(IOutputStream& stream, const struct fuse_attr& attr) {
#define TIMECONV(v) TInstant::MicroSeconds(v * 1000000 + v##nsec / 1000)
    stream << " ino=" << attr.ino
           << " size=" << attr.size
           << " blocks=" << attr.blocks
           << " atime=" << TIMECONV(attr.atime)
           << " mtime=" << TIMECONV(attr.mtime)
           << " ctime=" << TIMECONV(attr.ctime)
#ifdef _darwin_
           << " crtime=" << TIMECONV(attr.crtime)
#endif
           << " fmt=" << IntToString<8>(attr.mode >> 12)
           << " access=" << IntToString<8>(attr.mode & ~S_IFMT)
           << " nlink=" << attr.nlink
           << " uid=" << attr.uid
           << " gid=" << attr.gid
           << " rdev=" << attr.rdev
#ifdef _darwin_
           << " flags=" << attr.flags
#endif
           << " blksize=" << attr.blksize;
#undef TIMECONV
}

void PrintEntryOut(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    const auto& entryOut = arg.Read<struct fuse_entry_out>();
#define DURATIONCONV(v) TDuration::MicroSeconds(v * 1000000 + v##_nsec / 1000)
    stream << "nodeid=" << entryOut.nodeid
           << " gen=" << entryOut.generation
           << " eval=" << DURATIONCONV(entryOut.entry_valid)
           << " aval=" << DURATIONCONV(entryOut.attr_valid);
    PrintFuseAttr(stream, entryOut.attr);
#undef DURATIONCONV
}

void PrintAttrOut(IOutputStream& stream, TArgParser arg, const TPrintConnInfo&) {
    const auto& attrOut = arg.Read<struct fuse_attr_out>();
    stream << "aval=" << TDuration::MicroSeconds(attrOut.attr_valid * 1000000 +
                                                 attrOut.attr_valid_nsec / 1000);
    PrintFuseAttr(stream, attrOut.attr);
}

using THandler = void (TLowMount::*)(
    TRequestContextRef request,
    TArgParser arg);

using TArgPrinter = void(*)(IOutputStream& stream, TArgParser arg, const TPrintConnInfo& connInfo);

struct THandlerEntry {
    TStringBuf Name;
    THandler Handler = nullptr;
    TArgPrinter ArgPrinter = nullptr;
};

} // namespace

constexpr auto InitHandlers() {
    std::array<THandlerEntry, MAX_FUSE_HANDLERS> handlers;
    handlers[FUSE_INIT] = {"INIT",
                           &TLowMount::FuseInit,
                           PrintInit};

    handlers[FUSE_LOOKUP] = {"LOOKUP",
                             &TLowMount::FuseLookup,
                             PrintSingleString};
    handlers[FUSE_FORGET] = {"FORGET",
                             &TLowMount::FuseForget,
                             PrintForget};
    handlers[FUSE_BATCH_FORGET] = {"BATCH_FORGET",
                                   &TLowMount::FuseBatchForget,
                                   PrintBatchForget};

    handlers[FUSE_GETATTR] = {"GETATTR",
                              &TLowMount::FuseGetAttr,
                              PrintEmpty};
    handlers[FUSE_READLINK] = {"READLINK",
                               &TLowMount::FuseReadLink,
                               PrintEmpty};
    handlers[FUSE_ACCESS] = {"ACCESS",
                             &TLowMount::FuseAccess,
                             PrintAccess};
    handlers[FUSE_OPEN] = {"OPEN",
                           &TLowMount::FuseOpen,
                           PrintOpen};
    handlers[FUSE_RELEASE] = {"RELEASE",
                              &TLowMount::FuseRelease,
                              PrintRelease};
    handlers[FUSE_READ] = {"READ",
                           &TLowMount::FuseRead,
                           PrintRead};

    handlers[FUSE_OPENDIR] = {"OPENDIR",
                              &TLowMount::FuseOpenDir,
                              PrintOpen};
    handlers[FUSE_READDIR] = {"READDIR",
                              &TLowMount::FuseReadDir,
                              PrintRead};
#if FUSE_KERNEL_VERSION >= 7 && FUSE_KERNEL_MINOR_VERSION >= 21
    handlers[FUSE_READDIRPLUS] = {"READDIRPLUS",
                                  &TLowMount::FuseReadDirPlus,
                                  PrintRead};
#endif
    handlers[FUSE_RELEASEDIR] = {"RELEASEDIR",
                                 &TLowMount::FuseReleaseDir,
                                 PrintRelease};

    handlers[FUSE_STATFS] = {"STATFS",
                             &TLowMount::FuseStatFs,
                             PrintEmpty};

    handlers[FUSE_SETATTR] = {"SETATTR",
                              &TLowMount::FuseSetAttr,
                              PrintSetAttr};
    handlers[FUSE_WRITE] = {"WRITE",
                            &TLowMount::FuseWrite,
                            PrintWrite};
    handlers[FUSE_FLUSH] = {"FLUSH",
                            &TLowMount::FuseFlush,
                            PrintFlush};
    handlers[FUSE_FSYNC] = {"FSYNC",
                            &TLowMount::FuseFSync,
                            PrintFSync};
    handlers[FUSE_FALLOCATE] = {"FALLOCATE",
                                &TLowMount::FuseFAllocate,
                                PrintFAllocate};

    handlers[FUSE_MKNOD] = {"MKNOD",
                            &TLowMount::FuseMkNod,
                            PrintMkNod};
    handlers[FUSE_MKDIR] = {"MKDIR",
                            &TLowMount::FuseMkDir,
                            PrintMkDir};
    handlers[FUSE_CREATE] = {"CREATE",
                             &TLowMount::FuseCreate,
                             PrintCreate};
    handlers[FUSE_SYMLINK] = {"SYMLINK",
                              &TLowMount::FuseSymlink,
                              PrintSymlink};
    handlers[FUSE_LINK] = {"LINK",
                           &TLowMount::FuseLink,
                           PrintLink};
    handlers[FUSE_UNLINK] = {"UNLINK",
                             &TLowMount::FuseUnlink,
                             PrintSingleString};
    handlers[FUSE_RMDIR] = {"RMDIR",
                            &TLowMount::FuseRmDir,
                            PrintSingleString};
    handlers[FUSE_RENAME] = {"RENAME",
                             &TLowMount::FuseRename,
                             PrintRename};
    handlers[FUSE_FSYNCDIR] = {"FSYNCDIR",
                               &TLowMount::FuseFSyncDir,
                               PrintFSync};

    handlers[FUSE_LISTXATTR] = {"LISTXATTR",
                                &TLowMount::FuseListXattr,
                                PrintListXattr};
    handlers[FUSE_GETXATTR] = {"GETXATTR",
                               &TLowMount::FuseGetXattr,
                               PrintGetXattr};
    handlers[FUSE_SETXATTR] = {"SETXATTR",
                               &TLowMount::FuseSetXattr,
                               PrintSetXattr};
    handlers[FUSE_REMOVEXATTR] = {"REMOVEXATTR",
                                  &TLowMount::FuseRemoveXattr,
                                  PrintSingleString};

    handlers[FUSE_GETLK] = {"GETLK"};
    handlers[FUSE_SETLK] = {"SETLK"};
    handlers[FUSE_SETLKW] = {"SETLKW"};

    handlers[FUSE_INTERRUPT] = {"INTERRUPT"};
    handlers[FUSE_BMAP] = {"BMAP",
                           &TLowMount::FuseBmap,
                           PrintBmap};
    handlers[FUSE_DESTROY] = {"DESTROY"};
    handlers[FUSE_IOCTL] = {"IOCTL"};
    handlers[FUSE_POLL] = {"POLL"};
    handlers[FUSE_NOTIFY_REPLY] = {"NOTIFY_REPLY"};
    //TODO: some more linux ops
#ifdef _darwin_
    handlers[FUSE_SETVOLNAME] = {"SETVOLNAME"};
    handlers[FUSE_GETXTIMES] = {"GETXTIMES"};
    handlers[FUSE_EXCHANGE] = {"EXCHANGE"};
    //TODO: new osx ops
#endif
    return handlers;
}

namespace {

constexpr auto HANDLERS = InitHandlers();
constexpr THandlerEntry CUSE_INIT_HANDLER = {"CUSE_INIT"};

constexpr const THandlerEntry* LookupHandler(ui32 fuseOpcode) {
    if (fuseOpcode == CUSE_INIT) {
        return &CUSE_INIT_HANDLER;
    }
    if (fuseOpcode >= HANDLERS.size()) {
        return nullptr;
    }
    auto& handler = HANDLERS[fuseOpcode];
    if (handler.Name) {
        return & handler;
    } else {
        return nullptr;
    }
}

} // namespace

TString PrintFuseOp(const struct fuse_in_header& header, TArgParser args, const TPrintConnInfo& connInfo) {
    TStringBuilder msg;
    auto handler = LookupHandler(header.opcode);

    if (handler) {
        msg << handler->Name << " ";
    } else {
        msg << "UNKNOWN OPCODE " << header.opcode << " ";
    }

    auto& unique = header.unique;
    auto& nodeid = header.nodeid;
    auto& len = header.len;
    auto& uid = header.uid;
    auto& gid = header.gid;
    auto& pid = header.pid;
    msg << LabeledOutput(unique, nodeid, len, uid, gid, pid);
    if (args.Raw()) {
        if (handler) {
            if (handler->ArgPrinter) {
                msg << ' ';
                (*handler->ArgPrinter)(msg.Out, args, connInfo);
            }
        } else {
            msg << " " << HexText(args.Raw());
        }
    }

    return std::move(msg);
}

TLowMount::TLowMount(const TOptions& options)
    : Log_(options.Log)
    , Channel_(options.Channel)
    , Dispatcher_(options.Dipatcher)
    , TraceFuseMsg_(options.Debug)
    , RequestTimeout_(options.RequestTimeout)
    , Deadlines_(options.DeadlineScheduler)
    , MountPath_(options.MountPath)
    , MyPid_(options.AllowOurself ? 0 : GetPID())
    , Stop_(0)
    , InvalidationThread_(IThreadPool::TParams()
                              .SetBlocking(false)
                              .SetCatching(false)
                              .SetThreadName("FuseInvalidation"))
{
    memset(&ConnInfo_, 0, sizeof(ConnInfo_));
    for (size_t i = 0; i < MAX_FUSE_HANDLERS; ++i) {
        SkipLoggingUnknownOpcodes_[i] = false;
    }
    if (Deadlines_ == nullptr) {
        OwnDeadlines_ = MakeHolder<NThreading::TDeadlineScheduler>(TDuration::MilliSeconds(100));
        Deadlines_ = OwnDeadlines_.Get();
    }
    if (!options.NoInvalidation) {
        InvalidationThread_.Start(1);
    }
    InitCounters(*options.Counters);
    InitFuse();
}

void TLowMount::InitCounters(NMonitoring::TDynamicCounters& counters) {
    auto collector = []() {
        return NMonitoring::ExplicitHistogram({
            16, 32, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000, 64000, 256000, 1024000, 4096000, 16384000
        });
    };
    for (size_t i = 0; i < HANDLERS.size(); i++) {
        TString name = TString::Join("FuseOp_", HANDLERS[i].Name);
        if (name) {
            Histograms_[i] = counters.GetHistogram(name, collector());
        }
    }

    ActiveRequests_ = counters.GetCounter("FuseActiveRequests");
    Timeouts_ = counters.GetCounter("FuseTimeouts");
    ResponsesSuccess_ = counters.GetCounter("FuseResponsesSuccess", true);
    ResponsesError_ = counters.GetCounter("FuseResponsesError", true);
    UnhandledOps_ = counters.GetCounter("FuseUnhandledOps", true);
    QueuedInvalidations_ = counters.GetCounter("FuseQueuedInvalidations");
    InvalOk_ = counters.GetCounter("FuseInvalOk", true);
    InvalNoent_ = counters.GetCounter("FuseInvalNoent", true);
    InvalTimeout_ = counters.GetCounter("FuseInvalTimeout", true);
    InvalOther_ = counters.GetCounter("FuseInvalOther", true);
};

void TLowMount::Shutdown(const TString& reason) {
    if (!AtomicCas(&Stop_, 1, 0)) {
        Log_ << TLOG_INFO << "Mount " << GetMountPath()
             << " already shutting down" << Endl;
        return;
    }

    Log_ << TLOG_INFO << "Mount " << GetMountPath() << " is shutting down: "
         << reason << Endl;

    TVector<NThreading::IDeadlineActionRef> activeRequests;
    if (OwnDeadlines_) {
        activeRequests = OwnDeadlines_->Stop();
    } else {
        activeRequests = Deadlines_->Stop(reinterpret_cast<intptr_t>(this));
    }
    for (auto& req : activeRequests) {
        // XXX: what error should we send here?
        static_cast<TRequestContext*>(req.Get())->SendError(ENODEV);
    }
}

void TLowMount::InitFuse() {
    TBuffer tempBuf;
    TArgParser initParser;

    while (true) {
        if (AtomicGet(Stop_)) {
            ythrow yexception() << "Mount stopped while waiting for INIT message";
        }

        auto status = Channel_->Receive(tempBuf);
        switch (status) {
        case EMessageStatus::Success:
            break;
        case EMessageStatus::Again:
            continue;
        case EMessageStatus::Stop:
            ythrow yexception() << "Unmounted while waiting for INIT message";
        default:
            ythrow yexception() << "Unexpected receive status while waiting for INIT message";
        }

        TStringBuf data(tempBuf.Data(), tempBuf.Size());
        if (data.size() < sizeof(struct fuse_in_header) + sizeof(struct fuse_init_in)) {
            TStringBuilder msg;
            msg << "short read on fuse device: " << HexText(data);
            Log_ << TLOG_CRIT << msg << Endl;
            Shutdown(msg);
            ythrow yexception() << msg;
        }

        initParser = data;
        break;
    }

    auto& header = initParser.Read<struct fuse_in_header>();
    if (header.opcode != FUSE_INIT) {
        TStringBuilder msg;
        msg << "expected FUSE_INIT, but got " << PrintFuseOp(header, initParser, InConnInfo_);
        Log_ << TLOG_CRIT << msg << Endl;

        SendError(header, EPROTO);

        Shutdown(msg);
        ythrow yexception() << msg;
    }

    Log_ << TLOG_INFO << "Received FUSE INIT message: "
         << PrintFuseOp(header, initParser, InConnInfo_) << Endl;
    InConnInfo_ = initParser.Read<struct fuse_init_in>();

    if (InConnInfo_.major != FUSE_KERNEL_VERSION) {
        TStringBuilder msg;
        msg << "FUSE version " << InConnInfo_.major << " is not supported";
        Log_ << TLOG_CRIT << msg << Endl;

        SendError(header, EPROTO);

        Shutdown(msg);
        ythrow yexception() << msg;
    }

    if (InConnInfo_.minor < 5) {
        TStringBuilder msg;
        msg << "FUSE version " << InConnInfo_.major << "." << InConnInfo_.minor
            << " < 7.5 is not supported";
        Log_ << TLOG_CRIT << msg << Endl;

        SendError(header, EPROTO);

        Shutdown(msg);
        ythrow yexception() << msg;
    }

    memset(&ConnInfo_, 0, sizeof(ConnInfo_));
    ConnInfo_.major = InConnInfo_.major;
    ConnInfo_.minor = std::min<ui32>(InConnInfo_.minor, FUSE_KERNEL_MINOR_VERSION);
    ConnInfo_.max_write = Channel_->GetMaxWrite();
    ConnInfo_.max_readahead = InConnInfo_.max_readahead; // whatever kernel can do
    ConnInfo_.congestion_threshold = 0;
    ConnInfo_.max_background = 0;

    // Give dispatcher opportunity to select connection flags & modify it's parameters
    Dispatcher_->Init(InConnInfo_, this, ConnInfo_);
    // sanitize connection flags
    ConnInfo_.flags &= InConnInfo_.flags;

    {
        auto wantFlags = Hex(ConnInfo_.flags);
        auto major = FUSE_KERNEL_VERSION;
        auto minor = ConnInfo_.minor;
        Log_ << TLOG_INFO << "Replying to INIT with "
             << LabeledOutput(major,
                              minor,
                              ConnInfo_.max_write,
                              ConnInfo_.max_readahead,
                              ConnInfo_.congestion_threshold,
                              ConnInfo_.max_background,
                              wantFlags)
             << Endl;
    }

#ifdef _darwin_
    static_assert(FUSE_KERNEL_MINOR_VERSION == 19,
                  "check whether fuse_init_out has changed in new headers");
    SendReply(header, ConnInfo_);
#else
    static_assert(FUSE_KERNEL_MINOR_VERSION == 31,
                  "check whether fuse_init_out has changed in new headers");
    if (InConnInfo_.minor >= 23) {
        SendReply(header, ConnInfo_);
    } else {
        // Send only FUSE_COMPAT_22_INIT_OUT_SIZE bytes
        SendReply(header, TStringBuf{reinterpret_cast<const char*>(&ConnInfo_),
                                     FUSE_COMPAT_22_INIT_OUT_SIZE});
    }
#endif

    Initialized_.Signal();
}

bool TLowMount::WaitInitD(TInstant deadline) {
    return Initialized_.WaitD(deadline);
}

void TLowMount::SetIOThreadCount(int count) {
    PidRings = std::make_unique<TPidRing[]>(count);
    PidRingsSize = count;
}

TVector<pid_t> TLowMount::GetLastPids() const {
    TVector<pid_t> res;
    res.reserve(PidRingsSize * TPidRing::SIZE);
    for (int i = 0; i < PidRingsSize; i++) {
        const auto buf = PidRings[i].Read();
        res.insert(res.end(), buf.begin(), buf.end());
    }
    return res;
}

void TLowMount::Process(TStringBuf buf, int threadIdx) {
    Y_ASSERT(buf.Size() >= sizeof(struct fuse_in_header));

    TArgParser args{buf};
    auto& header = args.Read<struct fuse_in_header>();
    auto handler = LookupHandler(header.opcode);

    if (TraceFuseMsg_ || !handler) {
        Log_ << TLOG_DEBUG << PrintFuseOp(header, args, InConnInfo_) << Endl;
    }

    Y_ASSERT(threadIdx < PidRingsSize);
    PidRings[threadIdx].Put(header.pid);

    if (Y_UNLIKELY(MyPid_ != 0 &&
                   MyPid_ == static_cast<TProcessId>(header.pid) &&
                   header.opcode != FUSE_FORGET &&
                   header.opcode != FUSE_BATCH_FORGET &&
                   header.opcode != FUSE_INTERRUPT))
    {
        Log_ << TLOG_CRIT << "Received FUSE request from ourself: "
             << PrintFuseOp(header, args, InConnInfo_) << Endl;
        SendError(header, EIO);
        return;
    }

    if (handler && handler->Handler) {
        auto ctx = MakeRequest(header);
        // This is just a safebelt, request handlers should honor deadlines
        // given to them in request context
        Deadlines_->Schedule(ctx);
        ActiveRequests_->Inc();

        try {
            (this->*handler->Handler)(ctx, args.Raw());
        } catch (...) {
            auto bt = TBackTrace::FromCurrentException().PrintToString();
            Log_ << TLOG_ERR << "Exception in fuse op "
                 << PrintFuseOp(header, TArgParser(), InConnInfo_)
                 << CurrentExceptionMessage() << "\n" << bt << Endl;
            int err = Dispatcher_->HandleException(TTypedRequestContextBase(ctx),
                                                   std::current_exception());
            ctx->SendError(err);
        }
    } else {
        UnhandledOps_->Inc();

        bool writeLog = true;
        if (header.opcode < MAX_FUSE_HANDLERS) {
            if (SkipLoggingUnknownOpcodes_[header.opcode]) {
                writeLog = false;
            } else {
                SkipLoggingUnknownOpcodes_[header.opcode] = true;
            }
        }
        if (writeLog) {
            Log_ << TLOG_INFO << "Unhandled FUSE op "
                << LabeledOutput(header.opcode, header.unique) << Endl;
        }
        SendError(header, ENOSYS);
    }
}

void TLowMount::Timeout(TRequestContextRef ctx) {
    Timeouts_->Inc();
    const TTypedRequestContextBase typedCtx(ctx);

    try {
        int err = Dispatcher_->HandleTimeout(typedCtx);
        ctx->SendError(err);

        Log_ << TLOG_ERR << "FUSE request timed out "
             << PrintFuseOp(ctx->Header(), TArgParser(), InConnInfo_) << " "
             << LabeledOutput(ctx->GetStart(), ctx->GetDeadline()) << " "
             << Dispatcher_->GetDebugInfo(typedCtx)
             << Endl;
    } catch (const yexception& ex) {
        Log_ << TLOG_ERR << "Exception while sending timeout error: "
             << PrintFuseOp(ctx->Header(), TArgParser(), InConnInfo_) << " "
             << LabeledOutput(ctx->GetStart(), ctx->GetDeadline()) << " "
             << Dispatcher_->GetDebugInfo(typedCtx)
             << " : " << ex.AsStrBuf() << Endl;
    }
}

void TLowMount::FinishRequest(TRequestContextRef ctx, ui32 opcode) {
    Deadlines_->Remove(ctx);
    ActiveRequests_->Dec();
    if (opcode < Histograms_.size()) {
        auto hist = Histograms_[opcode];
        if (hist) {
            hist->Collect((TInstant::Now() - ctx->GetStart()).MicroSeconds());
        }
    }
}

EMessageStatus TLowMount::SendError(const struct fuse_in_header& req, int err) {
    struct fuse_out_header resp;
    memset(&resp, 0, sizeof(resp));
    resp.len = sizeof(resp);
    resp.error = -err;
    resp.unique = req.unique;

    if (TraceFuseMsg_) {
        auto unique = req.unique;
        if (err != 0) {
            auto errStr = LastSystemErrorText(err);
            Log_ << TLOG_DEBUG << "SendError " << LabeledOutput(unique, err, errStr) << Endl;
        } else {
            Log_ << TLOG_DEBUG << "SendSuccess " << LabeledOutput(unique) << Endl;
        }
    }

    if (err == 0) {
        ResponsesSuccess_->Inc();
    } else {
        ResponsesError_->Inc();
    }

    std::array<struct iovec, 1> iov;
    iov[0].iov_base = &resp;
    iov[0].iov_len = sizeof(resp);
    return Channel_->Send(iov);
}

EMessageStatus TLowMount::SendReply(const struct fuse_in_header& req, TStringBuf payload) {
    struct fuse_out_header header;
    memset(&header, 0, sizeof(header));
    header.len = sizeof(header) + payload.size();
    header.error = 0;
    header.unique = req.unique;

    std::array<iovec, 2> iov;
    iov[0].iov_base = &header;
    iov[0].iov_len = sizeof(header);
    iov[1].iov_base = const_cast<char*>(payload.data());
    iov[1].iov_len = payload.size();

    //TODO: try to decode message based on header opcode
    if (TraceFuseMsg_) {
        auto unique = req.unique;
        auto len = header.len;
        auto nodeid = req.nodeid;
        const bool entryOpcode = (
               req.opcode == FUSE_LOOKUP
            || req.opcode == FUSE_LINK
            || req.opcode == FUSE_MKNOD
            || req.opcode == FUSE_MKDIR
            || req.opcode == FUSE_SYMLINK
        );
        if (entryOpcode && payload.size() == sizeof(struct fuse_entry_out)) {
            TStringStream msg;
            PrintEntryOut(msg, TArgParser(payload), InConnInfo_);
            Log_ << TLOG_DEBUG << "EntryReply " << LabeledOutput(unique, nodeid)
                 << " " << msg.Str() << Endl;
        } else if (req.opcode == FUSE_GETATTR && payload.size() == sizeof(struct fuse_attr_out)) {
            TStringStream msg;
            PrintAttrOut(msg, TArgParser(payload), InConnInfo_);
            Log_ << TLOG_DEBUG << "AttrReply " << LabeledOutput(unique, nodeid)
                 << " " << msg.Str() << Endl;
        } else if (payload.size() > 128) {
            Log_ << TLOG_DEBUG << "SendReply " << LabeledOutput(unique, len)
                 << " " << HexText(payload.Head(64)) << " (truncated) "
                 << HexText(payload.Last(64)) << Endl;
        } else {
            Log_ << TLOG_DEBUG << "SendReply " << LabeledOutput(unique, len)
                 << " " << HexText(payload) << Endl;
        }
    }

    ResponsesSuccess_->Inc();
    return Channel_->Send(iov);
}

EMessageStatus TLowMount::SendNotify(int notifyCode, TArrayRef<iovec> iov) {
    struct fuse_out_header header;
    memset(&header, 0, sizeof(header));
    header.len = sizeof(header);
    for (size_t i = 1; i < iov.size(); ++i) {
        header.len += iov[i].iov_len;
    }
    header.error = notifyCode;
    header.unique = 0;

    iov[0].iov_base = &header;
    iov[0].iov_len = sizeof(header);

    EMessageStatus res = Channel_->Send(iov);
    switch (res) {
        case EMessageStatus::Success:
            InvalOk_->Inc();
            break;
        case EMessageStatus::NoEntry:
            InvalNoent_->Inc();
            break;
        case EMessageStatus::Timeout:
            InvalTimeout_->Inc();
            break;
        default:
            InvalOther_->Inc();
    }

    return res;
}

void TLowMount::FuseInit(
    TRequestContextRef ctx,
    TArgParser)
{
    ctx->SendError(EPROTO);
    ythrow yexception() << "Unexpected FUSE_INIT after mount was already initialized";
}

void TLowMount::FuseLookup(
    TRequestContextRef ctx,
    TArgParser arg)
{
    return Dispatcher_->Lookup(ctx, ctx->INode(), arg.ReadStr());
}

void TLowMount::FuseForget(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& forgetIn = arg.Read<struct fuse_forget_in>();
    Dispatcher_->Forget(ctx->INode(), forgetIn.nlookup);
    ctx->SendNone();
}

void TLowMount::FuseBatchForget(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& forgetIn = arg.Read<struct fuse_batch_forget_in>();
    for (size_t i = 0; i < forgetIn.count; i++) {
        const auto& item = arg.Read<struct fuse_forget_one>();
        Dispatcher_->Forget(item.nodeid, item.nlookup);
    }
    ctx->SendNone();
}

void TLowMount::FuseGetAttr(
    TRequestContextRef ctx,
    TArgParser)
{
    return Dispatcher_->GetAttr(ctx, ctx->INode());
}

void TLowMount::FuseReadLink(
    TRequestContextRef ctx,
    TArgParser)
{
    return Dispatcher_->ReadLink(ctx, ctx->INode());
}

void TLowMount::FuseAccess(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& accessIn = arg.Read<struct fuse_access_in>();
    return Dispatcher_->Access(ctx, ctx->INode(), accessIn.mask);
}

void TLowMount::FuseOpen(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& openIn = arg.Read<struct fuse_open_in>();
    return Dispatcher_->Open(ctx, ctx->INode(), openIn.flags);
}

void TLowMount::FuseRelease(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& releaseIn = arg.Read<struct fuse_release_in>();
    return Dispatcher_->Release(ctx, ctx->INode(), releaseIn.fh);
}

void TLowMount::FuseRead(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& readIn = arg.Read<struct fuse_read_in>();
    return Dispatcher_->Read(ctx,
                             ctx->INode(),
                             readIn.fh,
                             readIn.size,
                             readIn.offset);
}

void TLowMount::FuseOpenDir(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& openIn = arg.Read<struct fuse_open_in>();
    return Dispatcher_->OpenDir(ctx, ctx->INode(), openIn.flags);
}

void TLowMount::FuseReleaseDir(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& releaseIn = arg.Read<struct fuse_release_in>();
    return Dispatcher_->ReleaseDir(ctx, ctx->INode(), releaseIn.fh);
}

void TLowMount::FuseReadDir(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& readIn = arg.Read<struct fuse_read_in>();
    return Dispatcher_->ReadDir(ctx,
                                ctx->INode(),
                                readIn.fh,
                                readIn.offset,
                                TFuseDirList(readIn.size));
}

void TLowMount::FuseReadDirPlus(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& readIn = arg.Read<struct fuse_read_in>();
    return Dispatcher_->ReadDirPlus(ctx,
                                    ctx->INode(),
                                    readIn.fh,
                                    readIn.offset,
                                    TFuseDirListPlus(readIn.size));
}

void TLowMount::FuseStatFs(
    TRequestContextRef ctx,
    TArgParser)
{
    return Dispatcher_->StatFs(ctx);
}

void TLowMount::FuseSetAttr(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& setAttrIn = arg.Read<struct fuse_setattr_in>();
    Dispatcher_->SetAttr(ctx, ctx->INode(), setAttrIn);
}

void TLowMount::FuseWrite(
    TRequestContextRef ctx,
    TArgParser arg)
{
    auto dataOffset = ConnInfo_.minor < 9
                          ? FUSE_COMPAT_WRITE_IN_SIZE
                          : sizeof(struct fuse_write_in);
    auto dataPtr = arg.Raw().Data() + dataOffset;
    const auto& writeIn = arg.Read<struct fuse_write_in>();

    auto data = TStringBuf(dataPtr, writeIn.size);

    Dispatcher_->Write(ctx,
                       ctx->INode(),
                       writeIn.fh,
                       data,
                       writeIn.offset);
}

void TLowMount::FuseFlush(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& flushIn = arg.Read<struct fuse_flush_in>();
    Dispatcher_->Flush(ctx, ctx->INode(), flushIn.fh);
}

void TLowMount::FuseFSync(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& fsyncIn = arg.Read<struct fuse_fsync_in>();
    // No symbolic constant for this
    // Reference code here: https://a.yandex-team.ru/arc_vcs/contrib/libs/fuse/lib/fuse_lowlevel.c?rev=dbf8ab8393231eb3725ae2cf9dabe51ed8219901#L1361
    bool dataSync = fsyncIn.fsync_flags & 1;
    Dispatcher_->FSync(ctx, ctx->INode(), fsyncIn.fh, dataSync);
}

void TLowMount::FuseFAllocate(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& fAllocateIn = arg.Read<struct fuse_fallocate_in>();
    Dispatcher_->FAllocate(ctx, ctx->INode(),
                           fAllocateIn.offset,
                           fAllocateIn.length,
                           fAllocateIn.mode);
}

void TLowMount::FuseMkNod(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto argPtr = arg.Raw().Data();
    const auto& mknodIn = arg.Read<struct fuse_mknod_in>();

    auto mode = mknodIn.mode;
    size_t nameOffset;
    if (ConnInfo_.minor < 12) {
        nameOffset = FUSE_COMPAT_MKNOD_IN_SIZE;
    } else {
        nameOffset = sizeof(struct fuse_mknod_in);
        if (ConnInfo_.flags & FUSE_DONT_MASK) {
            mode &= ~mknodIn.umask;
        }
    }

    auto namePtr = argPtr + nameOffset;
    auto name = TStringBuf(namePtr);

    Dispatcher_->MkNod(ctx, ctx->INode(), name, mode, mknodIn.rdev);
}

void TLowMount::FuseMkDir(
    TRequestContextRef ctx,
    TArgParser arg
) {
    const auto& mkDirIn = arg.Read<struct fuse_mkdir_in>();
    auto name = arg.ReadStr();

    auto mode = mkDirIn.mode;
    if (ConnInfo_.minor >= 12 && ConnInfo_.flags & FUSE_DONT_MASK) {
        mode &= ~mkDirIn.umask;
    }

    Dispatcher_->MkDir(ctx, ctx->INode(), name, mode);
}

void TLowMount::FuseCreate(
    TRequestContextRef ctx,
    TArgParser arg
) {
    const auto& createIn = arg.Read<struct fuse_create_in>();
    auto name = TStringBuf(arg.Raw().Data());

    auto mode = createIn.mode;
    if (ConnInfo_.minor >= 12 && ConnInfo_.flags & FUSE_DONT_MASK) {
        mode &= ~createIn.umask;
    }

    Dispatcher_->Create(ctx, ctx->INode(), name, mode, createIn.flags);
}

void TLowMount::FuseSymlink(
    TRequestContextRef ctx,
    TArgParser arg
) {
    auto name = arg.ReadStr();
    auto link = arg.ReadStr();

    Dispatcher_->Symlink(ctx, ctx->INode(), name, link);
}

void TLowMount::FuseLink(
    TRequestContextRef ctx,
    TArgParser arg
) {
    const auto& linkIn = arg.Read<struct fuse_link_in>();
    auto name = arg.ReadStr();

    Dispatcher_->Link(ctx, ctx->INode(), linkIn.oldnodeid, name);
}

void TLowMount::FuseUnlink(
    TRequestContextRef ctx,
    TArgParser arg
) {
    auto name = TStringBuf(arg.ReadStr());

    Dispatcher_->Unlink(ctx, ctx->INode(), name);
}

void TLowMount::FuseRmDir(
    TRequestContextRef ctx,
    TArgParser arg
) {
    auto name = TStringBuf(arg.ReadStr());

    Dispatcher_->RmDir(ctx, ctx->INode(), name);
}

void TLowMount::FuseRename(
    TRequestContextRef ctx,
    TArgParser arg
) {
    TInodeNum newDir;
    ui32 flags = 0;
#ifdef _darwin_
    if (InConnInfo_.flags & FUSE_RENAME_SWAP) {
        const auto& renameIn = arg.Read<struct fuse_rename_in>();
        newDir = renameIn.newdir;
        flags = renameIn.flags;
    } else {
        newDir = arg.Read<ui64>();
    }
#else
    const auto& renameIn = arg.Read<struct fuse_rename_in>();
    newDir = renameIn.newdir;
#endif

    auto name = TStringBuf(arg.ReadStr());
    auto newName = TStringBuf(arg.ReadStr());

    Dispatcher_->Rename(ctx, ctx->INode(), name, newDir, newName, flags);
}

void TLowMount::FuseFSyncDir(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& fsyncIn = arg.Read<struct fuse_fsync_in>();
    // No symbolic constant for this
    // Reference code here: https://a.yandex-team.ru/arc_vcs/contrib/libs/fuse/lib/fuse_lowlevel.c?rev=dbf8ab8393231eb3725ae2cf9dabe51ed8219901#L1421
    bool dataSync = fsyncIn.fsync_flags & 1;
    Dispatcher_->FSyncDir(ctx, ctx->INode(), fsyncIn.fh, dataSync);
}

void TLowMount::FuseListXattr(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& getXattrIn = arg.Read<struct fuse_getxattr_in>();
    Dispatcher_->ListXattr(ctx, ctx->INode(), TFuseXattrList(getXattrIn.size));
}

void TLowMount::FuseGetXattr(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& getXattrIn = arg.Read<struct fuse_getxattr_in>();
    auto attrName = arg.ReadStr();
    Dispatcher_->GetXattr(ctx, ctx->INode(), attrName, TFuseXattrBuf(getXattrIn.size));
}

void TLowMount::FuseSetXattr(
    TRequestContextRef ctx,
    TArgParser arg)
{
    const auto& setXattrIn = arg.Read<struct fuse_setxattr_in>();
    auto attrName = arg.ReadStr();
    auto value = TStringBuf(arg.Raw().Data(), setXattrIn.size);
    Dispatcher_->SetXattr(ctx, ctx->INode(), setXattrIn.flags, attrName, value);
}

void TLowMount::FuseRemoveXattr(
    TRequestContextRef ctx,
    TArgParser arg)
{
    auto attrName = arg.ReadStr();
    Dispatcher_->RemoveXattr(ctx, ctx->INode(), attrName);
}

void TLowMount::FuseBmap(
    TRequestContextRef ctx,
    TArgParser arg)
{
    Y_UNUSED(arg);
    Dispatcher_->Bmap(ctx);
}

EMessageStatus TLowMount::InvalidateInodeSync(TInodeNum ino, i64 off, i64 len) {
    if (ConnInfo_.minor < 12) {
        return EMessageStatus::Success;
    }

    if (TraceFuseMsg_) {
        Log_ << TLOG_DEBUG << "Notify InvalidateInode "
             << LabeledOutput(ino, off, len) << Endl;
    }

    struct fuse_notify_inval_inode_out out;
    memset(&out, 0, sizeof(out));
    out.ino = ino;
    out.off = off;
    out.len = len;

    std::array<iovec, 2> iov;
    iov[1].iov_base = &out;
    iov[1].iov_len = sizeof(out);

    return SendNotify(FUSE_NOTIFY_INVAL_INODE, iov);
}

void TLowMount::InvalidateInode(TInodeNum ino, i64 off, i64 len) {
    if (ConnInfo_.minor < 12) {
        return;
    }

    QueuedInvalidations_->Inc();
    InvalidationThread_.SafeAddFunc([this, ino, off, len] {
        try {
            InvalidateInodeSync(ino, off, len);
        } catch (const TSystemError& ex) {
            Log_ << TLOG_ERR << "Error during InvalidateInode "
                 << LabeledOutput(ino, off, len) << " " << ex.AsStrBuf() << Endl;
        }
        QueuedInvalidations_->Dec();
    });
}

EMessageStatus TLowMount::InvalidateEntrySync(TInodeNum parent, const TString& name) {
    if (ConnInfo_.minor < 12) {
        return EMessageStatus::Success;
    }

    if (TraceFuseMsg_) {
        Log_ << TLOG_DEBUG << "Notify InvalidateEntry "
             << LabeledOutput(parent, name) << Endl;
    }

    struct fuse_notify_inval_entry_out out;
    memset(&out, 0, sizeof(out));
    out.parent = parent;
    out.namelen = name.size();

    std::array<iovec, 3> iov;
    iov[1].iov_base = &out;
    iov[1].iov_len = sizeof(out);
    iov[2].iov_base = const_cast<char*>(name.c_str());
    iov[2].iov_len = name.size() + 1;

    return SendNotify(FUSE_NOTIFY_INVAL_ENTRY, iov);
}

void TLowMount::InvalidateEntry(TInodeNum parent, const TString& name) {
    if (ConnInfo_.minor < 12) {
        return;
    }

    QueuedInvalidations_->Inc();
    InvalidationThread_.SafeAddFunc([this, parent, name] {
        if (TraceFuseMsg_) {
            Log_ << TLOG_DEBUG << "Notify InvalidateEntry "
                 << LabeledOutput(parent, name) << Endl;
        }

        try {
            InvalidateEntrySync(parent, name);
        } catch (const TSystemError& ex) {
            Log_ << TLOG_ERR << "Error during InvalidateEntry "
                 << LabeledOutput(parent, name) << " " << ex.AsStrBuf() << Endl;
        }
        QueuedInvalidations_->Dec();
    });
}

EMessageStatus TLowMount::NotifyEntryDeleteSync(TInodeNum parent, TInodeNum child, const TString& name) {
    if (ConnInfo_.minor < 18) {
        return InvalidateEntrySync(parent, name);
    }

    if (TraceFuseMsg_) {
        Log_ << TLOG_DEBUG << "NotifyEntryDelete "
             << LabeledOutput(parent, child, name) << Endl;
    }

    struct fuse_notify_delete_out out;
    memset(&out, 0, sizeof(out));
    out.parent = parent;
    out.child = child;
    out.namelen = name.size();

    std::array<iovec, 3> iov;
    iov[1].iov_base = &out;
    iov[1].iov_len = sizeof(out);
    iov[2].iov_base = const_cast<char*>(name.c_str());
    iov[2].iov_len = name.size() + 1;

    return SendNotify(FUSE_NOTIFY_DELETE, iov);
}

void TLowMount::NotifyEntryDelete(TInodeNum parent, TInodeNum child, const TString& name) {
    QueuedInvalidations_->Inc();
    InvalidationThread_.SafeAddFunc([this, parent, child, name] {
        try {
            NotifyEntryDeleteSync(parent, child, name);
        } catch (const TSystemError& ex) {
            Log_ << TLOG_ERR << "Error during NotifyEntryDelete "
                 << LabeledOutput(parent, child, name) << " " << ex.AsStrBuf() << Endl;
        }
        QueuedInvalidations_->Dec();
    });
}

NThreading::TFuture<void> TLowMount::FlushInvalidations() {
    NThreading::TPromise<void> promise = NThreading::NewPromise();
    InvalidationThread_.SafeAddFunc([promise] () mutable {
        promise.SetValue();
    });
    return promise.GetFuture();
}

} // namespace NFuse
