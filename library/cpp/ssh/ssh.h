#pragma once

#include <util/system/defaults.h>
#ifdef _MSC_VER
#define _SSIZE_T_DEFINED
#endif

#include <contrib/libs/libssh2/include/libssh2.h>
#include <contrib/libs/libssh2/include/libssh2_sftp.h>

#define HAVE_SSIZE_T 1

#include <util/folder/path.h>
#include <util/network/socket.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/maybe.h>

class TSSHLib {
public:
    TSSHLib();
    ~TSSHLib();
};

class TSSHThinSession {
protected:
    LIBSSH2_SESSION* Session_;

public:
    TSSHThinSession();
    ~TSSHThinSession();

    LIBSSH2_SESSION* GetSession();
    TString GetLastError();
    int GetLastErrorCode();
    void RaiseLast();
};

class TSSHSession: public TSSHThinSession {
private:
    const TString Host_;
    const TString User_;
    TNetworkAddress Address_;
    TSocket Socket_;

    static inline const ui16 DefaultSSHPort = 22;

public:
    /* Initialization of this instance will establish ssh session
     * @param password  if password specified, session will be authorized with \p user and \p password (backward compatibility) 
     *                  otherwise authentication will not be performed here
     */
    TSSHSession(const TString& host, const TString& user, const TMaybe<TString> password = Nothing());
    TSSHSession(const TString& host, ui16 port, const TString& user, const TMaybe<TString> password = Nothing());

    // Note: you may want to authorize using ssh-agent
    // so see TSSHAgent::AuthorizeSession(...)
    /* Initiate interactive authorization process with username and password
      *
      * @password  if provided, then \p password will be used for authorization
      *            otherwise user will need to enter the password in the console
      * @throw yexception if authorization failed
      */
    int AuthorizeByPassword(const TMaybe<TString>& password = Nothing());

    /* Autorize session via public key
      *
      * @password  password for public key.
      *            if provided, then \p password will be used for authorization
      *            otherwise user will need to enter the password in the console
      * @throw yexception if authorization failed
      */
    int AuthorizeByPublicKeyFromFile(const TString& pathToPublic, const TString& pathToPrivate, const TMaybe<TString>& password = Nothing());

    bool IsAuthenticated() const;

    /* Execute command on remote host
      *
      * @param cmd      command to be executed
      * @param onOut    callback for recieved stdout of command
      * @param onErr    callback for recieved stderr of command
      * @param path     path where command should be executed
      *                 if path == "", home directory of User will be used
      * @param envVars  environments variables to be set on the remote side
      *  Attention: ssh in not responsible for stopping running commmand on remote host after closing connection
      *             so if you want to run long-living command, better use RunCommandUsingPty()
      */
    int RunCommand(
        const TString& cmd,
        std::function<void (TStringBuf)> onOut,
        std::function<void (TStringBuf)> onErr,
        const TString path = "",
        const THashMap<TString, TString>& envVars = THashMap<TString, TString>());

    /* Execute command on remote host with opening PTY
      *
      * @param cmd    command to be executed
      * @param onOut  callback for recieved stdout and stderr of command.
      * @param path   path where command should be executed
      *               if path == "", home directory of User will be used
      * @param envVars  environments variables to be set on the remote side
      */
    int RunCommandUsingPty(
        const TString& cmd,
        std::function<void (TStringBuf)> onOut,
        const TString path = "",
        const THashMap<TString, TString>& envVars = THashMap<TString, TString>());

    void SetTimeout(TDuration timeout);

    ~TSSHSession();

private:
    class TSSHChannel;

private:
    void WaitSocket() const;
    int RunCommandImpl(
        TSSHChannel& channel,
        const TString& cmd,
        std::function<void (TStringBuf)> onOut,
        std::function<void (TStringBuf)> onErr,
        const TString path = "",
        const THashMap<TString, TString>& envVars = THashMap<TString, TString>());
};

class TSFTPSession {
private:
    TSSHSession* Session_;
    LIBSSH2_SFTP* SftpSession_;

public:
    TSFTPSession(TSSHSession* session);
    ~TSFTPSession();

    LIBSSH2_SFTP* GetSession();
    void RaiseLast();
    TString GetLastError();
    
    void MkDir(const TFsPath& remotePath);
};

class TSFTPDir {
private:
    TSFTPSession* Session_;
    LIBSSH2_SFTP_HANDLE* SftpHandle_;

public:
    TSFTPDir(TSFTPSession* session, const TString& path);
    ~TSFTPDir();

    struct TItem {
        TString Name;
        bool Dir;

        TItem();
        TItem(const TString& name, bool dir);
    };

    typedef TVector<TItem> TItems;
    void Ls(TItems* items);
};

class TSFTPFile {
private:
    TSFTPSession* Session_;
    LIBSSH2_SFTP_HANDLE* SftpHandle_;
    const TString File_;

public:
    TSFTPFile(TSFTPSession* session, const TString& file);
    ~TSFTPFile();

    void Download(IOutputStream* out);
    void Download(const TFsPath& pathToStore);

    void Upload(IInputStream* in, bool overwrite = false);
    void Upload(const TFsPath& fileToUpload, bool overwrite = false);
};
