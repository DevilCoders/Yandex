#pragma once

#include <util/system/winint.h>

class TSrvMan
{
public:
    TSrvMan();
    ~TSrvMan();
    bool Install(const char* path, const char* name, const char* displayName = NULL, const char* dependencies = NULL);
    bool Remove(const char* name);
    bool Start(const char* name, DWORD argc = 0, const char** argv = NULL);
    bool Stop(const char* name);
    bool GetPathName(const char* name, char** path);
    bool IsRunning(const char* name);
    bool Control(const char* name, DWORD control);
    bool GetConfig(const char* name, LPQUERY_SERVICE_CONFIG* lBuf);
    bool GetStatus(const char* name, SERVICE_STATUS& Status);
private:
    SC_HANDLE SchMan;
};
