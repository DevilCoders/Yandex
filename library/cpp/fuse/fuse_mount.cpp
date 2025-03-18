#include "fuse_mount.h"
#include "fuse_get_version.h"

#include <util/generic/maybe.h>
#include <util/generic/queue.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/string/printf.h>
#include <util/system/execpath.h>
#include <util/system/fs.h>
#include <util/system/tempfile.h>

#include <library/cpp/fuse/socket_pair/socket_pair.h>

#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef _darwin_
    #include <CoreFoundation/CoreFoundation.h>
    #include <DiskArbitration/DiskArbitration.h>
#endif

namespace NFuse {
    namespace {
        const char FUSERMOUNT_PROG[] = "fusermount";
        const char FUSE_COMMFD_ENV[] = "_FUSE_COMMFD";

        struct TMacConstants {
            TFsPath MountProg;
            TStringBuf ExecutablePathEnv;
            TStringBuf CallByLibEnv;
            TStringBuf LibName;
        };

        const TMacConstants MacFuseConstants{
            .MountProg = "/Library/Filesystems/macfuse.fs/Contents/Resources/mount_macfuse",
            .ExecutablePathEnv = "_FUSE_DAEMON_PATH",
            .CallByLibEnv = "_FUSE_CALL_BY_LIB",
            .LibName = "macfuse",
        };

        const TMacConstants OsxFuseConstants{
            .MountProg = "/Library/Filesystems/osxfuse.fs/Contents/Resources/mount_osxfuse",
            .ExecutablePathEnv = "MOUNT_OSXFUSE_DAEMON_PATH",
            .CallByLibEnv = "MOUNT_OSXFUSE_CALL_BY_LIB",
            .LibName = "osxfuse",
        };

        class TPosixSpawnFileActions {
        public:
            TPosixSpawnFileActions() {
                Y_ENSURE_EX(posix_spawn_file_actions_init(&Actions_) == 0,
                            TSystemError() << "fuse: posix_spawn_file_actions_init() failed");
            }

            ~TPosixSpawnFileActions() {
                if (posix_spawn_file_actions_destroy(&Actions_)) {
                    perror("fuse: posix_spawn_file_actions_destroy() failed");
                }
            }

            void SetQuiet() {
                const auto addOpenDevNull = [&](int fd) {
                    return posix_spawn_file_actions_addopen(Get(),
                                                            fd,
                                                            "/dev/null",
                                                            O_RDONLY,
                                                            0);
                };

                Y_ENSURE_EX(addOpenDevNull(STDOUT_FILENO) == 0 &&
                                addOpenDevNull(STDERR_FILENO) == 0,
                            TSystemError() << "fuse: posix_spawn_file_actions_addopen() failed");
            }

            void RedirectOutput(TFsPath outputFile) {
                const auto addOutput = [&](int fdIn) {
                    return posix_spawn_file_actions_addopen(Get(),
                                                            fdIn,
                                                            outputFile.c_str(),
                                                            O_CREAT | O_WRONLY | O_APPEND,
                                                            0);
                };

                Y_ENSURE_EX(addOutput(STDOUT_FILENO) == 0 &&
                                addOutput(STDERR_FILENO) == 0,
                            TSystemError() << "fuse: posix_spawn_file_actions_addopen() failed");
            }

            void AddClose(int fd) {
                Y_ENSURE_EX(posix_spawn_file_actions_addclose(Get(), fd) == 0,
                            TSystemError() << "fuse: posix_spawn_file_actions_addclose() failed");
            }

            posix_spawn_file_actions_t* Get() {
                return &Actions_;
            }

        private:
            posix_spawn_file_actions_t Actions_;
        };

        class TPosixSpawnAttr {
        public:
            TPosixSpawnAttr() {
                Y_ENSURE_EX(posix_spawnattr_init(&Attr_) == 0,
                            TSystemError() << "fuse: posix_spawn_file_actions_init() failed");
            }

            ~TPosixSpawnAttr() {
                if (posix_spawnattr_destroy(&Attr_)) {
                    perror("fuse: posix_spawn_file_actions_destroy() failed");
                }
            }

            posix_spawnattr_t* Get() {
                return &Attr_;
            }

        private:
            posix_spawnattr_t Attr_;
        };

        /* return value:
         * >= 0  => fd
         * -1    => error
         */
        int receive_fd(int fd) {
            struct msghdr msg;
            struct iovec iov;
            char buf[1];
            int rv;
            size_t ccmsg[CMSG_SPACE(sizeof(int)) / sizeof(size_t)];
            struct cmsghdr* cmsg;

            iov.iov_base = buf;
            iov.iov_len = 1;

            memset(&msg, 0, sizeof(msg));
            msg.msg_name = 0;
            msg.msg_namelen = 0;
            msg.msg_iov = &iov;
            msg.msg_iovlen = 1;
            /* old BSD implementations should use msg_accrights instead of
             * msg_control; the interface is different. */
            msg.msg_control = ccmsg;
            msg.msg_controllen = sizeof(ccmsg);

            while (((rv = recvmsg(fd, &msg, 0)) == -1) && errno == EINTR)
                ;
            Y_ENSURE_EX(rv != -1,
                        TSystemError() << "recvmsg() failed");
            Y_ENSURE(rv,
                     "fusermount did not send file descriptor");

            cmsg = CMSG_FIRSTHDR(&msg);
            Y_ENSURE(cmsg->cmsg_type == SCM_RIGHTS,
                     "got control message of unknown type " << cmsg->cmsg_type);
            return *(int*)CMSG_DATA(cmsg);
        }

        TString GetMountFuseLogs(TLog log, TMaybe<TFsPath> tempFile, const TFsPath& mountpoint) {
            if (tempFile && NFs::Exists(*tempFile)) {
                log << TLOG_INFO << "Trying to read fusermount logs in " << *tempFile << Endl;
                TFileInput fusermountOutput(*tempFile);
                auto logs = fusermountOutput.ReadAll();
                if (logs.EndsWith("only allowed if 'user_allow_other' is set in /etc/fuse.conf\n")) {
                    TStringBuilder sb;
                    // Just 'sudo echo user_allow_other >> /etc/fuse.conf'
                    // won't work because redirection is done by the shell before sudo is even started
                    sb << "To fix this error add user_allow_other to /etc/fuse.conf:\n"
                        << "    $ echo \"user_allow_other\" | sudo tee -a /etc/fuse.conf\n"
                        << "and then try again\n";
                    logs += sb;
                } else if (logs.EndsWith(" is itself on a macFUSE volume\n")) {
                    TStringBuilder sb;
                    sb << "This error means, that mount point is already mounted.\n"
                        << "You can force unmount it via\n"
                        << "    $ umount -f " << mountpoint.c_str() << "\n"
                        << "and then try again\n";
                    logs += sb;
                }
                return logs;
            }
            return {};
        };

        int FuseMountFusermount(const TFsPath& mountpoint, bool autoUnmount,
                                       const char* opts, int quiet, TLog log) {
            auto fds = MakeSocketPair();
            TPosixSpawnFileActions fileActions;
            TMaybe<TFsPath> tmpf;
            if (quiet) {
                fileActions.SetQuiet();
            } else {
                tmpf = MakeTempName();
                fileActions.RedirectOutput(*tmpf);
            }

            fileActions.AddClose(fds.second);

            TVector<const char*> argv;
            argv.push_back(FUSERMOUNT_PROG);
            if (opts) {
                argv.push_back("-o");
                argv.push_back(opts);
            }
            argv.push_back("--");
            argv.push_back(mountpoint.c_str());
            argv.push_back(nullptr);

            auto fusermountEnvFdVariable =
                Sprintf("%s=%i", FUSE_COMMFD_ENV, static_cast<int>(fds.first));
            TVector<const char*> env;
            env.push_back(fusermountEnvFdVariable.c_str());
            env.push_back(nullptr);

            TPosixSpawnAttr spawnAttr;

            pid_t childPid = -1;

            log << TLOG_INFO << "fusermount cmd: " << JoinRange(" ", argv.begin(), argv.end() - 1) << Endl;
            Y_ENSURE_EX(posix_spawnp(&childPid,
                                     FUSERMOUNT_PROG,
                                     fileActions.Get(),
                                     spawnAttr.Get(),
                                     const_cast<char* const*>(argv.data()),
                                     const_cast<char* const*>(env.data())) == 0,
                        TSystemError() << "fuse: posix_spawnp() failed");
            Y_ENSURE(childPid >= 0);

            fds.first.Close();

            int fuseFd;
            try {
                fuseFd = receive_fd(fds.second);
            } catch (const TSystemError& ex) {
                const auto logs = GetMountFuseLogs(log, tmpf, mountpoint);
                if (!logs.Empty()) {
                    throw TSystemError() << ex.what() << "\n" << logs;
                }
                throw;
            } catch (const yexception& ex) {
                const auto logs = GetMountFuseLogs(log, tmpf, mountpoint);
                if (!logs.Empty()) {
                    throw yexception() << ex.what() << "\n" << logs;
                }
                throw;
            }
            if (fuseFd < 0) {
                log << TLOG_ERR << "fuse: receive_fd() failed with code " << fuseFd << Endl;
            }

            if (!autoUnmount) {
                /* without auto_unmount option fusermount will not exit until
                       this socket is closed */
                fds.second.Close();
                /* bury zombie */
                if (waitpid(childPid, NULL, 0) != childPid) {
                    perror("fuse: waitpid() failed");
                }
            } else {
                fds.second.Release();
            }

            return fuseFd;
        }

        TString MakeArgv(const TRawChannelOptions& options) {
            TVector<TStringBuf> args;
            if (!options.NoAutoUnmount) {
#ifndef _darwin_
                // auto_unmount was introduced in 2.9.0
                if (GetFuseVersion() > "2.9") {
                    args.push_back("auto_unmount");
                }
#endif
            }
            if (options.AllowRoot) {
                // This is not an error. libfuse does the same https://a.yandex-team.ru/arc_vcs/contrib/libs/fuse/lib/mount.c?rev=r8216098#L209-213
                // Only-root restriction is enfoced in userspace https://a.yandex-team.ru/arc_vcs/contrib/libs/fuse/lib/fuse_lowlevel.c?rev=r8216293#L2394-2400
                args.push_back("allow_other");
            }
            if (options.AllowOther) {
                args.push_back("allow_other");
            }
            if (options.ReadOnly) {
                args.push_back("ro");
            } else {
                args.push_back("rw");
            }

            args.push_back("default_permissions");
            args.push_back("nosuid");
            args.push_back("nodev");
            args.push_back("nonempty");

            for (const auto& extra : options.Extra) {
                args.push_back(extra);
            }

            return JoinSeq(",", args);
        }

        int ArcFuseMountLinux(const TFsPath& mountpoint, const TRawChannelOptions& options, TLog log) {
            log << TLOG_INFO << "Mounting using fusermount" << Endl;

            Y_ENSURE(!options.AllowOther || !options.AllowRoot,
                     "fuse: 'allow_other' and 'allow_root' options are mutually exclusive");

            auto argv = MakeArgv(options);

            TMaybe<int> res;
            if (options.Subtype) {
                auto argsStrWithSubtype = Join(',', argv, "subtype=" + options.Subtype);

                try {
                    res = FuseMountFusermount(mountpoint,
                                              !options.NoAutoUnmount,
                                              argsStrWithSubtype.c_str(),
                                              1,
                                              log);
                } catch (yexception& e) {
                    log << TLOG_ERR << "fuse: exception was thrown: " << e.what() << Endl;
                    res = -1;
                }
                if (res == -1) {
                    log << TLOG_WARNING << "fusermount: fail. will retry without subtype" << Endl;
                    res = FuseMountFusermount(mountpoint,
                                              !options.NoAutoUnmount,
                                              argv.c_str(),
                                              0,
                                              log);
                }
            } else {
                res = FuseMountFusermount(mountpoint,
                                          !options.NoAutoUnmount,
                                          argv.c_str(),
                                          1,
                                          log);
            }

            if (*res >= 0) {
                log << TLOG_INFO << "fusermount: success. fd=" << *res << Endl;
            } else {
                log << TLOG_ERR << "fusermount: fail" << Endl;
            }

            return *res;
        }

        void ArcFuseUnmountLinux(const TFsPath& mountpoint, int fd, TLog log) {
            // this if is copy-pasted from libfuse
            if (fd != -1) {
                struct pollfd pfd;

                pfd.fd = fd;
                pfd.events = 0;
                int res = poll(&pfd, 1, 0);

                /* Need to close file descriptor, otherwise synchronous umount
                   would recurse into filesystem, and deadlock.

                   Caller expects fuse_kern_unmount to close the fd, so close it
                   anyway. */
                int close_res = close(fd);
                log << TLOG_INFO << "close(fd=" << fd << ") returned " << close_res << Endl;

                /* If file poll returns POLLERR on the device file descriptor,
                   then the filesystem is already unmounted */
                if (res == 1 && (pfd.revents & POLLERR)) {
                    log << TLOG_WARNING << "assume that already unmounted" << Endl;
                    return;
                }
            }

            TPosixSpawnFileActions fileActions;
            TPosixSpawnAttr attr;
            TVector<const char*> argv{
                FUSERMOUNT_PROG,
                "-u", "-q", "-z",
                "--",
                mountpoint.c_str(),
                nullptr};
            TVector<const char*> env{nullptr};

            pid_t childPid = -1;
            log << TLOG_INFO << "calling fusermount to unmount" << Endl;
            Y_ENSURE_EX(posix_spawnp(&childPid,
                                     FUSERMOUNT_PROG,
                                     fileActions.Get(),
                                     attr.Get(),
                                     const_cast<char* const*>(argv.data()),
                                     const_cast<char* const*>(env.data())) == 0,
                        TSystemError() << "fuse unmount: posix_spawnp() failed");
            Y_ENSURE(childPid >= 0);

            int status = 0;
            Y_ENSURE_EX(waitpid(childPid, &status, 0) == childPid,
                        TSystemError() << "fuse: waitpid() failed");
            const auto exitCode = WEXITSTATUS(status);
            Y_ENSURE(exitCode == 0,
                     "fuse: fusermount -u returned non-zero exit code: " << exitCode);

            log << TLOG_INFO << "successfully unmounted by fusermount" << Endl;
        }

        int ArcFuseMountMacImpl(const TFsPath& mountpoint,
                                const TRawChannelOptions& options,
                                TLog log,
                                const TMacConstants& constants) {
            if (!constants.MountProg.Exists()) {
                log << TLOG_WARNING << constants.LibName << ": program " << constants.MountProg
                    << " is not found. Mount attempt failed." << Endl;
                return -1;
            }

            log << TLOG_INFO << constants.LibName << ": will use "
                << constants.MountProg << " for mounting" << Endl;

            const auto mountPointRealPath = mountpoint.RealPath();
            const auto volumeName = mountPointRealPath.Basename();

            auto fds = MakeSocketPair();
            TPosixSpawnFileActions fileActions;
            TPosixSpawnAttr attr;

            TVector<TString> optionsList = {
                "volname=" + volumeName,
                "default_permissions",
            };
            if (options.AutoCache) {
                optionsList.push_back("auto_cache");
            }
            if (options.Subtype) {
                optionsList.push_back("subtype=" + options.Subtype);
                optionsList.push_back("fstypename=" + options.Subtype);
                optionsList.push_back("fsname=" + options.Subtype);
            }
            if (options.AllowRoot) {
                optionsList.push_back("allow_root");
            }
            if (options.AllowOther) {
                optionsList.push_back("allow_other");
            }

            for (const auto& item : options.Extra) {
                optionsList.push_back(item);
            }

            TString optionsStr = JoinSeq(",", optionsList);
            TVector<const char*> argv{
                constants.MountProg.c_str(),
                "-o",
                optionsStr.c_str(),
                mountPointRealPath.c_str(),
                nullptr};

            auto macfuseEnvFdVariable =
                Sprintf("%s=%i", FUSE_COMMFD_ENV, static_cast<int>(fds.first));
            auto daemonPathVariable =
                Sprintf("%s=%s", constants.ExecutablePathEnv.data(), GetExecPath().c_str());
            auto callByLibVariable =
                Sprintf("%s=1", constants.CallByLibEnv.data());

            TVector<const char*> env{
                macfuseEnvFdVariable.c_str(),
                daemonPathVariable.c_str(),
                // mount_macfuse refuses to do anything unless this is set
                callByLibVariable.c_str(),
                nullptr};

            TMaybe<TFsPath> tmpf;
            tmpf = MakeTempName();
            fileActions.RedirectOutput(*tmpf);

            pid_t childPid;

            log << TLOG_INFO << constants.LibName << " cmd: " << JoinRange(" ", argv.begin(), argv.end() - 1) << Endl;
            log << TLOG_INFO << constants.LibName << " env: " << JoinRange(" ", env.begin(), env.end() - 1) << Endl;
            Y_ENSURE_EX(posix_spawn(&childPid,
                                    constants.MountProg.c_str(),
                                    fileActions.Get(),
                                    attr.Get(),
                                    const_cast<char* const*>(argv.data()),
                                    const_cast<char* const*>(env.data())) == 0,
                        TSystemError() << constants.LibName << ": posix_spawnp() failed");

            fds.first.Close();

            int fuseFd;
            try {
                fuseFd = receive_fd(fds.second);
            } catch (const TSystemError& ex) {
                const auto logs = GetMountFuseLogs(log, tmpf, mountpoint);
                if (!logs.Empty()) {
                    throw TSystemError() << ex.what() << "\n" << logs;
                }
                throw;
            } catch (const yexception& ex) {
                const auto logs = GetMountFuseLogs(log, tmpf, mountpoint);
                if (!logs.Empty()) {
                    throw yexception() << ex.what() << "\n" << logs;
                }
                throw;
            }

            if (fuseFd >= 0) {
                log << TLOG_INFO << constants.LibName << ": success. fd=" << fuseFd << Endl;
            } else {
                log << TLOG_ERR << constants.LibName << ": fail" << Endl;
            }

            return fuseFd;
        }

        int ArcFuseMountMac(const TFsPath& mountpoint, const TRawChannelOptions& options, TLog log) {
            if (options.ReadOnly) {
                log << TLOG_WARNING << "fuse : RO mount is not supported by lib, will ignore this option" << Endl;
            }

            for (TQueue<TMacConstants> constantsOptions({MacFuseConstants, OsxFuseConstants});
                 constantsOptions;
                 ) {
                try {
                    auto constants = constantsOptions.front();
                    constantsOptions.pop();
                    auto res = ArcFuseMountMacImpl(mountpoint, options, log, constants);
                    Y_ENSURE_EX(res >= 0,
                                TSystemError() << constants.LibName
                                               << ": mount fail");
                    return res;
                } catch (const TSystemError& e){
                    log << TLOG_WARNING << e.what() << Endl;
                    if (constantsOptions) {
                        continue;
                    }

                    log << TLOG_ERR << "neither macfuse nor osxfuse succeeded to mount" << Endl;
                    throw;
                }
            }
            return -1;
        }

#ifdef _darwin_
        void ArcFuseUnmountMac(const TFsPath& mountpoint, int fd, TLog log) {
            auto unmountFs = [&]() {
                DASessionRef daSession = DASessionCreate(nullptr);
                CFURLRef url = CFURLCreateFromFileSystemRepresentation(
                    nullptr,
                    (const UInt8*)mountpoint.c_str(),
                    mountpoint.GetPath().length(),
                    TRUE);
                DADiskRef disk = DADiskCreateFromVolumePath(nullptr, daSession, url);
                if (disk) {
                    log << TLOG_INFO << "fuse unmount: DADiskCreateFromVolumePath() "
                        << "returned disk object. Will unmount it." << Endl;

                    DADiskUnmount(disk, kDADiskUnmountOptionForce, nullptr, nullptr);
                    CFRelease(disk);
                } else {
                    log << TLOG_INFO << "fuse unmount: DADiskCreateFromVolumePath() "
                        << "returned nullptr. Device is already unmounted." << Endl;
                }
                CFRelease(url);
                CFRelease(daSession);
            };

            unmountFs();

            Y_ENSURE_EX(close(fd) == 0,
                        TSystemError() << "fuse unmount: close() failed");
        }
#endif
    } // namespace

    int ArcFuseMount(const TFsPath& mountpoint, const TRawChannelOptions& options, TLog log) {
#ifdef _darwin_
        Y_UNUSED(ArcFuseMountLinux);
        auto fd = ArcFuseMountMac(mountpoint, options, log);
#else
        Y_UNUSED(ArcFuseMountMac);
        auto fd = ArcFuseMountLinux(mountpoint, options, log);
#endif
        Y_ENSURE(fd >= 0, "ArcFuseMount() failed");
        return fd;
    }

    void ArcFuseUnmount(const TFsPath& mountpoint, int fd, TLog log) {
#ifdef _darwin_
        Y_UNUSED(ArcFuseUnmountLinux);
        ArcFuseUnmountMac(mountpoint, fd, log);
#else
        ArcFuseUnmountLinux(mountpoint, fd, log);
#endif
    }
} // namespace NFuse
