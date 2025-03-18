#include "ssh.h"

#include <library/cpp/openssl/init/init.h>

#include <util/string/builder.h>
#include <util/stream/file.h>
#include <util/system/file.h>

#if defined(_unix_)
#include <termios.h>
#include <unistd.h>
#elif defined(_win_)
#include <windows.h>
#endif

namespace {

TStringBuf HiddenInput() {
    #if defined(_unix_)
        termios oldt;
        tcgetattr(STDIN_FILENO, &oldt);
        termios newt = oldt;
        newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    #elif defined(_win_)
        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(hStdin, &mode);
        SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
    #endif

    TString input;
    Cin >> input;
    Cout << Endl;

    #if defined(_unix_)
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    #elif defined(_win_)
        SetConsoleMode(hStdin, mode);
    #endif

    return input;
}

} // anonymous namespace

class TSSHSession::TSSHChannel: public TNonCopyable {
public:
    TSSHChannel(TSSHSession* session)
        : Session_(session)
    {
        while (
            !(Channel_ = libssh2_channel_open_session(Session_->GetSession())) &&
            Session_->GetLastErrorCode() == LIBSSH2_ERROR_EAGAIN
        ) {
            Session_->WaitSocket();
        }

        if (Channel_ == nullptr) {
            ythrow yexception() << "Can't open channel for ssh session: " << Session_->GetLastError();
        }
    }

    explicit operator bool () {
        return Channel_ != nullptr;
    }

    operator LIBSSH2_CHANNEL* () const {
        return Channel_;
    }

    ~TSSHChannel() {
        CloseChannel();
    }

private:
    void CloseChannel() {
        if (Channel_) {
            libssh2_channel_send_eof(Channel_);
            while (libssh2_channel_close(Channel_) == LIBSSH2_ERROR_EAGAIN) {
                Session_->WaitSocket();
            }
            libssh2_channel_free(Channel_);
        }
    }

private:
    LIBSSH2_CHANNEL* Channel_;
    TSSHSession* Session_;
    TSocket Socket_;
};

TSSHLib::TSSHLib() {
    InitOpenSSL();
    int rc = libssh2_init(0);
    if (rc != 0)
        ythrow yexception() << "libssh2 initialization failed (" << rc << ")";
}

TSSHLib::~TSSHLib() {
    libssh2_exit();
}

static TSSHLib SSHLib;

TSSHThinSession::TSSHThinSession()
    : Session_(libssh2_session_init())
{
    if (!Session_)
        ythrow yexception() << "Session init fail";
}

TSSHThinSession::~TSSHThinSession() {
    libssh2_session_free(Session_);
}

LIBSSH2_SESSION* TSSHThinSession::GetSession() {
    return Session_;
}

void TSSHThinSession::RaiseLast() {
    ythrow yexception() << "problem with ssh session '" << GetLastError() << "'";
}

TString TSSHThinSession::GetLastError() {
    char* buffer = nullptr;
    libssh2_session_last_error(Session_, &buffer, nullptr, 0);
    return buffer;
}

int TSSHThinSession::GetLastErrorCode() {
    return libssh2_session_last_error(Session_, nullptr, nullptr, 0);
}

TSSHSession::TSSHSession(const TString& host, ui16 port, const TString& user, const TMaybe<TString> password)
    : Host_(host)
    , User_(user)
    , Address_(host, port)
    , Socket_(Address_)
{
    libssh2_session_set_blocking(Session_, 1);
    int rc = libssh2_session_startup(Session_, Socket_);
    if (rc) {
        ythrow yexception() << "Failure establishing SSH session: " << rc << "'" << GetLastError() << "'";
    }

    if (password) {
        libssh2_userauth_password(Session_, user.data(), password->data());
    }
}

TSSHSession::TSSHSession(const TString& host, const TString& user, const TMaybe<TString> password)
    : TSSHSession(host, DefaultSSHPort, user, password)
{
}

void TSSHSession::SetTimeout(TDuration timeout) {
    libssh2_session_set_timeout(Session_, timeout.MilliSeconds());
}

void TSSHSession::WaitSocket() const {
    fd_set fd;
    fd_set* writefd = NULL;
    fd_set* readfd = NULL;

    FD_ZERO(&fd);
    FD_SET(Socket_, &fd);

    // Make sure we wait in the correct direction
    int dir = libssh2_session_block_directions(Session_);
    if(dir & LIBSSH2_SESSION_BLOCK_INBOUND) {
        readfd = &fd;
    }

    if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND) {
        writefd = &fd;
    }

    struct timeval timeout = {5, 0};
    select(Socket_ + 1, readfd, writefd, NULL, &timeout);
}

int TSSHSession::AuthorizeByPassword(const TMaybe<TString>& password) {
    if (!password) {
        Cout << "Username: " << User_ << Endl;
        Cout << "Passphrase: ";
    }
    if (libssh2_userauth_password(
            Session_,
            User_.c_str(),
            password ? password->c_str() : HiddenInput().data())
    ) {
        ythrow yexception() << "Authentication failed: " << "'" << GetLastError() << "'";
    }
    return 0;
}

int TSSHSession::AuthorizeByPublicKeyFromFile(const TString& pathToPublic, const TString& pathToPrivate, const TMaybe<TString>& password) {
    if (!password) {
        Cout << "Username: " << User_ << Endl;
        Cout << "Passphrase for key \"" << pathToPublic << "\": ";
    }

    if (libssh2_userauth_publickey_fromfile(
            Session_,
            User_.c_str(),
            pathToPublic.c_str(),
            pathToPrivate.c_str(),
            password ? password->c_str() : HiddenInput().data())
    ) {
        ythrow yexception() << "Authentication by public key failed: " << GetLastError();
    }

    return 0;
}

bool TSSHSession::IsAuthenticated() const {
    return libssh2_userauth_authenticated(Session_);
}

int TSSHSession::RunCommandImpl(
    TSSHChannel& channel,
    const TString& cmd,
    std::function<void (TStringBuf)> onOut,
    std::function<void (TStringBuf)> onErr,
    const TString path,
    const THashMap<TString, TString>& envVars
) {
    for (const auto& var : envVars) {
        libssh2_channel_setenv(
            channel,
            var.first.c_str(),
            var.second.c_str()
        );
    }
    TStringBuilder command;
    if (path) {
        command << "cd " << path << ";";
    }
    command << cmd;
    int status;
    while ((status = libssh2_channel_exec(channel, command.data())) == LIBSSH2_ERROR_EAGAIN) {
        WaitSocket();
    }
    if (status) {
        ythrow yexception() << "Can't execute command: " << GetLastError();
    }

    libssh2_session_set_blocking(Session_, false);
    auto handleRemoteStream = [this, &channel] (size_t ioId, std::function<void (TStringBuf)> cb) -> bool {
        char buffer[0x4000];
        int rc = libssh2_channel_read_ex(channel, ioId, buffer, sizeof(buffer));
        bool hasMore = false;
        if(rc > 0) {
            cb(TStringBuf(buffer, rc));
            hasMore = true;
        } else if (rc == 0) {
            // pass
        } else if (rc == LIBSSH2_ERROR_EAGAIN) {
            libssh2_channel_flush_ex(channel, ioId);
            hasMore = true;
        } else {
            ythrow yexception() << "Error while reading " << (ioId == 0 ? "stdout" : "stderr") << " (" << rc << "): " << GetLastError();
        }
        return hasMore;
    };

    bool continueReading = true;
    while (continueReading) {
        continueReading = handleRemoteStream(0, onOut);
        continueReading = handleRemoteStream(1, onErr) || continueReading;
        WaitSocket();
    }

    libssh2_session_set_blocking(Session_, false);
    return libssh2_channel_get_exit_status(channel);
}

int TSSHSession::RunCommand(
    const TString& cmd,
    std::function<void (TStringBuf)> onOut,
    std::function<void (TStringBuf)> onErr,
    const TString path,
    const THashMap<TString, TString>& envVars
) {
    libssh2_session_set_blocking(Session_, true);
    TSSHChannel channel(this);
    return RunCommandImpl(channel, cmd, onOut, onErr, path, envVars);
}

int TSSHSession::RunCommandUsingPty(
    const TString& cmd,
    std::function<void (TStringBuf)> onOut,
    const TString path,
    const THashMap<TString, TString>& envVars
) {
    libssh2_session_set_blocking(Session_, true);
    TSSHChannel channel(this);
    // Pseudo-terminal allows us to run the command in a separate environment,
    // so all running processes will be terminated with the end of the session
    if (libssh2_channel_request_pty(channel, "vanilla")) {
        ythrow yexception() << "Failed to request pty: " << GetLastError();
    }
    return RunCommandImpl(channel, cmd, onOut, onOut, path, envVars);
}

TSSHSession::~TSSHSession() {
    libssh2_session_disconnect(Session_, "Normal Shutdown, Thank you for playing");
}

TSFTPSession::TSFTPSession(TSSHSession* session)
    : Session_(session)
{
    SftpSession_ = libssh2_sftp_init(Session_->GetSession());

    if (!SftpSession_)
        ythrow yexception() << "Unable to init SFTP session";
}

LIBSSH2_SFTP* TSFTPSession::GetSession() {
    return SftpSession_;
}

void TSFTPSession::RaiseLast() {
    Session_->RaiseLast();
}

TString TSFTPSession::GetLastError() {
    return Session_->GetLastError();
}

TSFTPSession::~TSFTPSession() {
    libssh2_sftp_shutdown(SftpSession_);
}

void TSFTPSession::MkDir(const TFsPath& remotePath) {
    int rc = libssh2_sftp_mkdir(
        SftpSession_,
        remotePath.GetPath().c_str(),
        LIBSSH2_SFTP_S_IRWXU |
        LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IXGRP|
        LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH
    );
    if (rc) {
        RaiseLast();
    }
}

TSFTPDir::TSFTPDir(TSFTPSession* session, const TString& path)
    : Session_(session)
{
    SftpHandle_ = libssh2_sftp_opendir(Session_->GetSession(), path.data());

    if (!SftpHandle_)
        ythrow yexception() << "Unable to open file with SFTP";
}

TSFTPDir::TItem::TItem() {
}

TSFTPDir::TItem::TItem(const TString& name, bool dir)
    : Name(name)
    , Dir(dir)
{
}

void TSFTPDir::Ls(TItems* items) {
    items->clear();
    do {
        char mem[512];
        char longentry[512];
        LIBSSH2_SFTP_ATTRIBUTES attrs;

        int rc = libssh2_sftp_readdir_ex(SftpHandle_, mem, sizeof(mem), longentry, sizeof(longentry), &attrs);
        if (rc > 0) {
            bool isDir = 0 == ((int)attrs.permissions & 1 << 15);
            items->push_back(TItem(mem, isDir));
        } else {
            if (rc < 0)
                Session_->RaiseLast();
            break;
        }

    } while (1);
}

TSFTPDir::~TSFTPDir() {
    libssh2_sftp_close(SftpHandle_);
}

TSFTPFile::TSFTPFile(TSFTPSession* session, const TString& file)
    : Session_(session)
    , File_(file)
{
}

void TSFTPFile::Download(IOutputStream* out) {
    SftpHandle_ = libssh2_sftp_open(Session_->GetSession(), File_.data(), LIBSSH2_FXF_READ, 0);

    if (!SftpHandle_) {
        ythrow yexception() << "Unable to open file with SFTP: " << Session_->GetLastError() << " '" << File_ << "'";
    }

    do {
        char mem[1 << 18];

        int rc = libssh2_sftp_read(SftpHandle_, mem, sizeof(mem));
        if (rc > 0) {
            out->Write(mem, rc);
        } else {
            if (rc < 0)
                Session_->RaiseLast();
            break;
        }
    } while (1);
}

void TSFTPFile::Download(const TFsPath& pathToStore) {
    if (pathToStore.Exists()) {
        ythrow yexception() << "Can't download file to'" << pathToStore << "': path already exists.";
    }
    TFileOutput fileOStream(TFile(pathToStore, WrOnly));
    Download(&fileOStream);
}

void TSFTPFile::Upload(IInputStream* input, bool overwrite) {
    unsigned long flags = LIBSSH2_FXF_CREAT | LIBSSH2_FXF_WRITE;
    if (!overwrite) {
        flags |= LIBSSH2_FXF_EXCL;
    }
    SftpHandle_ = libssh2_sftp_open(Session_->GetSession(), File_.data(), flags, 0);

    if (!SftpHandle_) {
        ythrow yexception() << "Unable to open file with SFTP: " << Session_->GetLastError() << " '" << File_ << "'";
    }

    do {
        char mem[1 << 18];
        size_t nread = input->Read(mem, sizeof(mem));
        if(nread <= 0) {
            break;
        }
        char* memPtr = mem;

        do {
            int rc = libssh2_sftp_write(SftpHandle_, memPtr, nread);
            if(rc < 0) {
                Session_->RaiseLast();
            }
            memPtr += rc;
            nread -= rc;
        } while(nread);
    } while (1);
}

void TSFTPFile::Upload(const TFsPath& fileToUpload, bool overwrite) {
    if (!fileToUpload.Exists()) {
        ythrow yexception() << "Can't upload '" << fileToUpload << "': path not exists.";
    }
    if (fileToUpload.IsDirectory()) {
        ythrow yexception() << "Can't upload '" << fileToUpload << "': path is a directory.";
    }
    TFileInput fileIStream(TFile(fileToUpload, RdOnly));
    Upload(&fileIStream, overwrite);
}

TSFTPFile::~TSFTPFile() {
    libssh2_sftp_close(SftpHandle_);
}
