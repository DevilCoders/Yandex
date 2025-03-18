#pragma once

#include <util/system/winint.h>
#include <util/generic/noncopyable.h>

class TWinService : public TNonCopyable
{
public:
    TWinService();
    virtual ~TWinService();
    void ServiceCtrl(DWORD dwCtrlCode);
    bool ServiceMain(DWORD argc, char **argv);
    bool Run(int argc, char **argv);
protected:
    const char* ServiceName;
    const char* DisplayName;
    const char* Dependencies; // "dep1\0dep2\0\0"
private:
    bool IsDebug;
    DWORD Err;
    DWORD CheckPoint;
    SERVICE_STATUS        Status;
    SERVICE_STATUS_HANDLE StatusHandle;
protected:
    bool ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
    void AddToMessageLog(const char* msg);
private:
    virtual bool ServiceStart(DWORD argc, char **argv);
    virtual void ServiceStop();
    void CmdInstallService();
    void CmdRemoveService();
};
