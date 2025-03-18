#include "srvman.h"
#include <malloc.h>

TSrvMan::TSrvMan()
    : SchMan(NULL)
{
        SchMan = OpenSCManagerA(NULL,                    // machine (NULL == local)
                               NULL,                     // database (NULL == default)
                               SC_MANAGER_ALL_ACCESS);   // access required
}

TSrvMan::~TSrvMan()
{
    if (SchMan)
        CloseServiceHandle(SchMan);
}

bool TSrvMan::Install(const char* path, const char* name, const char* displayName, const char* dependencies)
{
    if (!path || !*path)
        return false;
    if (!name || !*name)
        return false;
    if (!displayName || !*displayName)
        displayName = name;
    SC_HANDLE schService = CreateServiceA(
                      SchMan,                     // SCManager database
                      name,                       // name of service
                      displayName,                // name to display
                      SERVICE_ALL_ACCESS,         // desired access
                      SERVICE_WIN32_OWN_PROCESS,  // service type
                      SERVICE_DEMAND_START,       // start type
                      SERVICE_ERROR_NORMAL,       // error control type
                      path,                       // service's binary
                      NULL,                       // no load ordering group
                      NULL,                       // no tag identifier
                      dependencies,               // dependencies
                      NULL,                       // LocalSystem account
                      NULL);                      // no password

    if (!schService) {
        if (GetLastError() == ERROR_SERVICE_EXISTS)
            return true;
        return false;
    }
    CloseServiceHandle(schService);
    return true;
}

bool TSrvMan::Remove(const char* name)
{
    if (!name || !*name)
        return false;
    SC_HANDLE schService = OpenService(SchMan, name, SERVICE_ALL_ACCESS);
    if (schService) {
        // try to stop the service
        SERVICE_STATUS ssStatus;
        QueryServiceStatus(schService, &ssStatus);
        if (ssStatus.dwCurrentState == SERVICE_RUNNING || ssStatus.dwCurrentState == SERVICE_PAUSED) {
            if (ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus)) {
                while (QueryServiceStatus(schService, &ssStatus)) {
                    if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
                        Sleep(1000);
                    else
                        break;
                }
                if (ssStatus.dwCurrentState != SERVICE_STOPPED) {
                    CloseServiceHandle(schService);
                    return false;
                }
            } else {
                CloseServiceHandle(schService);
                return false;
            }
        }
        // now remove the service
        if (DeleteService(schService)) {
            CloseServiceHandle(schService);
            return true;
        }
        CloseServiceHandle(schService);
        return false;
    }
    if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
        return true;
    return false;
}

bool TSrvMan::GetStatus(const char* name, SERVICE_STATUS& ssStatus)
{
    if (!name || !*name)
        return false;
    SC_HANDLE schService = OpenService(SchMan, name, SERVICE_ALL_ACCESS);
    if (schService == NULL)
        return false;
    if (!QueryServiceStatus(schService, &ssStatus)) {
        CloseServiceHandle(schService);
        return false;
    }
    CloseServiceHandle(schService);
    return true;
}

bool TSrvMan::Start(const char* name, DWORD argc, const char **argv)
{
    if (!name || !*name)
        return false;
    SC_HANDLE schService = OpenServiceA(SchMan, name, SERVICE_ALL_ACCESS);
    if (schService == NULL)
        return false;
    if (!StartServiceA(schService, argc, argv)) {
        CloseServiceHandle(schService);
        return false;
    }
    SERVICE_STATUS ssStatus;
    DWORD dwOldCheckPoint;
    if (!QueryServiceStatus(schService, &ssStatus)) {
        CloseServiceHandle(schService);
        return false;
    }
    while (ssStatus.dwCurrentState == SERVICE_START_PENDING) {
        // Save the current checkpoint.
        dwOldCheckPoint = ssStatus.dwCheckPoint;
        // Wait for the specified interval.
        Sleep(1000);
        // Check the status again.
        if (!QueryServiceStatus(schService, &ssStatus))
            break;
        // Break if the checkpoint has not been incremented.
        if (dwOldCheckPoint >= ssStatus.dwCheckPoint)
            break;
    }
    if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
        CloseServiceHandle(schService);
        return true;
    }
    CloseServiceHandle(schService);
    return false;
}

bool TSrvMan::Stop(const char* name)
{
    if (!name || !*name)
        return false;
    return Control(name, SERVICE_CONTROL_STOP);
}
bool TSrvMan::Control(const char* name, DWORD fdwControl)
{
    if (!name || !*name)
        return false;
    SERVICE_STATUS ssStatus;
    DWORD fdwAccess;
    // The required service object access depends on the control.
    switch (fdwControl) {
        case SERVICE_CONTROL_STOP:
            fdwAccess = SERVICE_STOP;
            break;
        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
            fdwAccess = SERVICE_PAUSE_CONTINUE;
            break;
        case SERVICE_CONTROL_INTERROGATE:
            fdwAccess = SERVICE_INTERROGATE;
            break;
        default:
            fdwAccess = SERVICE_INTERROGATE;
    }
    // Open a handle to the service.
    SC_HANDLE schService = OpenServiceA(
        SchMan,       // SCManager database
        name,         // name of service
        fdwAccess);   // specify access
    if (schService == NULL)
        return false;
    // Send a control value to the service.
    if (!ControlService(
            schService,   // handle of service
            fdwControl,   // control value to send
            &ssStatus))   // address of status info
    {
        CloseServiceHandle(schService);
        return false;
    }
    CloseServiceHandle(schService);
    return true;
}

//free *lpqscBuf if it returns true!
bool TSrvMan::GetConfig(const char* name, LPQUERY_SERVICE_CONFIG *lpqscBuf)
{
    if (!name || !*name)
        return false;
    DWORD dwBytesNeeded;
    *lpqscBuf = NULL;
    // Open a handle to the service.
    SC_HANDLE schService = OpenServiceA(
        SchMan,               // SCManager database
        name,                 // name of service
        SERVICE_QUERY_CONFIG);// need QUERY access
    if (schService == NULL)
        return false;
    // Allocate a buffer for the information configuration.
    *lpqscBuf = (LPQUERY_SERVICE_CONFIG) malloc(4096);
    if (*lpqscBuf == NULL) {
        CloseServiceHandle(schService);
        return false;
    }
    // Get the information configuration.
    if (!QueryServiceConfig(schService, *lpqscBuf, 4096, &dwBytesNeeded)) {
        free(*lpqscBuf);
        *lpqscBuf = NULL;
        DWORD LastError = GetLastError();
        if (LastError == ERROR_INSUFFICIENT_BUFFER) {
            *lpqscBuf = (LPQUERY_SERVICE_CONFIG) malloc(dwBytesNeeded);
            if (*lpqscBuf == NULL) {
                CloseServiceHandle(schService);
                return false;
            }
            if (!QueryServiceConfig(schService, *lpqscBuf, dwBytesNeeded, &dwBytesNeeded)) {
                free(*lpqscBuf);
                *lpqscBuf = NULL;
                CloseServiceHandle(schService);
                return false;
            }
        } else {
            CloseServiceHandle(schService);
            return false;
        }
    }

    CloseServiceHandle(schService);
    return true;
}

//free *pPath if it returns true!
bool TSrvMan::GetPathName(const char* name, char **pPath)
{
    if (!name || !*name)
        return false;
    LPQUERY_SERVICE_CONFIG lpqscBuf;
    if (!GetConfig(name, &lpqscBuf))
        return false;
    *pPath = strdup(strlwr(lpqscBuf->lpBinaryPathName));
    free(lpqscBuf);
    return true;
}

bool TSrvMan::IsRunning(const char* name)
{
    if (!name || !*name)
        return false;
    SERVICE_STATUS ssStatus;
    if (!GetStatus(name, ssStatus))
        return false;
    return ssStatus.dwCurrentState == SERVICE_RUNNING || ssStatus.dwCurrentState == SERVICE_START_PENDING;
}

