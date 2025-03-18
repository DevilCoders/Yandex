#include "winservice.h"
#include "srvman.h"

#include <util/system/error.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>

#include <new>

static TWinService* WinServicePtr = 0;

static void WINAPI service_ctrl(DWORD dwCtrlCode)
{
    assert(WinServicePtr);
    WinServicePtr->ServiceCtrl(dwCtrlCode);
}

static void WINAPI service_main(DWORD argc, char **argv)
{
    assert(WinServicePtr);
    WinServicePtr->ServiceMain(argc, argv);
}

TWinService::TWinService()
    : ServiceName("")
    , DisplayName("")
    , Dependencies("")
    , IsDebug(false)
    , Err(0)
    , CheckPoint(1)
{
    if (WinServicePtr)
        ythrow yexception() << "The only service per the program";
    WinServicePtr = this;
}

TWinService::~TWinService()
{
}

void TWinService::ServiceCtrl(DWORD dwCtrlCode)
{
    switch (dwCtrlCode) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            ReportStatusToSCMgr(SERVICE_STOP_PENDING, 0, 3000);
            ServiceStop();
            ReportStatusToSCMgr(SERVICE_STOPPED, 0, 0);
            return;
        default:
            break;
    }
    ReportStatusToSCMgr(Status.dwCurrentState, 0, 0);
}

bool TWinService::ServiceMain(DWORD argc, char **argv)
{
    StatusHandle = RegisterServiceCtrlHandler(ServiceName, service_ctrl);
    if (StatusHandle) {
        Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        Status.dwServiceSpecificExitCode = 0;
        if (ReportStatusToSCMgr(SERVICE_START_PENDING, 0, 3000))
            return ServiceStart(argc, argv);
        else
            ReportStatusToSCMgr(SERVICE_STOPPED, Err, 0);
            return false;
    }
    return false;
}

bool TWinService::ServiceStart(DWORD /*argc*/, char ** /*argv*/)
{
    if (!ReportStatusToSCMgr(SERVICE_START_PENDING, 0, 3000))
        return false;
    // Initialize here...
    if (!ReportStatusToSCMgr(SERVICE_RUNNING, 0, 0))
        return false;
    // Listen here...
    // Free here...
    return true;
}

void TWinService::ServiceStop()
{
}

bool TWinService::ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    if (!IsDebug) {
        if (dwCurrentState == SERVICE_START_PENDING)
            Status.dwControlsAccepted = 0;
        else
            Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        Status.dwCurrentState = dwCurrentState;
        Status.dwWin32ExitCode = dwWin32ExitCode;
        Status.dwWaitHint = dwWaitHint;
        if (dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED)
            Status.dwCheckPoint = 0;
        else
            Status.dwCheckPoint = CheckPoint++;
        if (!::SetServiceStatus(StatusHandle, &Status)) {
            AddToMessageLog("SetServiceStatus");
            return false;
        }
    }
    return true;
}

void TWinService::AddToMessageLog(const char* lpszMsg)
{
    char szMsg[256];
    Err = GetLastError();
    sprintf(szMsg, "%s error: %d", ServiceName, Err);
    char* lpszStrings[2];
    lpszStrings[0] = szMsg;
    lpszStrings[1] = (char*)lpszMsg;
    HANDLE hEventSource = RegisterEventSource(NULL, ServiceName);
    if (hEventSource != NULL) {
        ReportEvent(hEventSource,           // handle of event source
                    EVENTLOG_ERROR_TYPE,    // event type
                    0,                      // event category
                    Err,                  // event ID
                    NULL,                   // current user's SID
                    2,                      // strings in lpszStrings
                    0,                      // no bytes of raw data
                    (LPCTSTR *) lpszStrings,// array of error strings
                    NULL);                  // no raw data
        DeregisterEventSource(hEventSource);
    }
    if (IsDebug) {
        char OemErrorText[1024];
        AnsiToOem(lpszStrings[1], OemErrorText);
        Clog << lpszStrings[0] << Endl << OemErrorText << Endl;
    }
}

void TWinService::CmdInstallService()
{
    char* oldPath = NULL;
    TSrvMan SrvMng;
    if (SrvMng.GetPathName(ServiceName, &oldPath)) {
        Clog << DisplayName << " yet exists at " << oldPath << Endl;
        free(oldPath);
        return;
    }
    char Path[512];
    if (GetModuleFileName(0, Path, 512) == 0) {
        Clog << "Unable to install " << DisplayName << " - " << LastSystemErrorText() << Endl;
        return;
    }
    if (SrvMng.Install(Path, ServiceName, DisplayName, Dependencies)) {
        Clog << DisplayName << " installed." << Endl;
    } else {
        Clog << "CreateService failed - " << LastSystemErrorText() << Endl;
        return;
    }
    HKEY hKey = NULL;
    DWORD dword;
    char keybuf[512];
    strcpy(keybuf, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
    strcat(keybuf, ServiceName);
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, keybuf,
                                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dword))
    {
        RegSetValueEx(hKey, "EventMessageFile", 0, REG_SZ, (CONST BYTE*)Path, DWORD(strlen(Path)+1));
        DWORD ts = 7;
        RegSetValueEx(hKey, "TypesSupported", 0, REG_DWORD, (CONST BYTE*)&ts, sizeof(DWORD));
    }
    if (hKey != NULL) {
        RegFlushKey(hKey);
        RegCloseKey(hKey);
    }
}

void TWinService::CmdRemoveService()
{
    TSrvMan SrvMng;
    if (!SrvMng.Remove(ServiceName)) {
        Clog << "DeleteService failed - " << LastSystemErrorText() << Endl;
        return;
    }
    HKEY hKey = NULL;
    DWORD dword;
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application",
            0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dword))
    {
        RegDeleteKey(hKey, ServiceName);
    }
    if (hKey != NULL) {
        RegFlushKey(hKey);
        RegCloseKey(hKey);
    }
}

BOOL WINAPI ControlHandler (DWORD dwCtrlType)
{
    switch (dwCtrlType) {
        case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
        case CTRL_SHUTDOWN_EVENT:
            assert(WinServicePtr);
            WinServicePtr->ServiceCtrl(SERVICE_CONTROL_STOP);
            return TRUE;
            break;

    }
    return FALSE;
}

bool TWinService::Run(int argc, char **argv)
{
    if (argc > 1 && (*argv[1] == '-' || *argv[1] == '/')) {
        switch (tolower((unsigned)*(argv[1]+1))) {
            case 'i':
                CmdInstallService();
                return true;
                break;
            case 'r':
                CmdRemoveService();
                return true;
                break;
            case 'd':
                Clog << "Debugging " << DisplayName << "." << Endl;
                // fallthrough to handling -d (debug) as -v (verbose)
                [[fallthrough]];
            case 'v':
                IsDebug = true;
                SetConsoleCtrlHandler(ControlHandler, TRUE);
                return ServiceStart((DWORD)argc, argv);
                break;
            default:
                break;
        }
    }

    SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { (char*)ServiceName, (LPSERVICE_MAIN_FUNCTION)service_main },
        { NULL, NULL }
    };
    if (!StartServiceCtrlDispatcher(dispatchTable)) {
        AddToMessageLog("StartServiceCtrlDispatcher failed.");
        if (argc == 1) {
            IsDebug = true;
            SetConsoleCtrlHandler(ControlHandler, TRUE);
            return ServiceStart((DWORD)argc, argv);
        }
    }
    return false;
}
