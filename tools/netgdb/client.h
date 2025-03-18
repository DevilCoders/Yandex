#pragma once

/*
 * client.hd
 *
 *  Created on: 13.11.2009
 *      Author: solar
 */

#include <util/random/random.h>
#include <util/stream/file.h>
#include <util/string/vector.h>
#include "comm.h"

// netgdb options values
extern TString ClientTempPath;
extern TString ClientWorkDir;
extern TString NetGDBName;
extern TString OutputPath;
extern TString HostsFile;
extern TString MasterHost;
extern TString User;
extern bool Verbose;
extern bool RemoteStart;
enum ESHTransport {
    SSH,
    SKY
};
extern ESHTransport RemoteTransport;
extern int ThreadsCount;
extern bool ForceMode;

void PrintFailed(const TVector<TString>& failed);

class TClientCommand {
protected:
    virtual TString GetName() = 0;
    virtual bool PerformInner(const TVector<TString>& hosts, TVector<TString>& args, TVector<TString>* failed) = 0;
public:
    virtual ~TClientCommand() {}
    bool Perform(const TVector<TString>& hosts, TVector<TString>& args, TVector<TString>* failed)
    {
        if (Verbose)
            Cout << GetName() << "..." << Endl;
        bool rc = PerformInner(hosts, args, failed);
        if (Verbose) {
            Cout << GetName() << (failed->empty() ? " success" : (" failed for " + ::ToString(failed->size()) + " hosts")) << Endl;
        }

        if (ForceMode) {
            TOFStream failedFile("failed.txt");

            if (!failed->empty()) {
                TVector<TString>& mutableHosts = const_cast<TVector<TString>&>(hosts);
                TVector<TString>::const_iterator iter = failed->begin();
                while (iter != failed->end()) {
                    TVector<TString>::iterator found = std::find(mutableHosts.begin(), mutableHosts.end(), *iter);
                    if (found != mutableHosts.end())
                        mutableHosts.erase(found);
                    failedFile << *iter++ << Endl;
                }
            }
        }
        return !ForceMode ? rc : !hosts.empty();
    }
};

class TStartDaemonCommand: public TClientCommand {
protected:
    TString GetName() override
    {
        return "Daemon";
    }

    bool PerformInner(const TVector<TString>&, TVector<TString>&, TVector<TString>*) override
    {
        return false;
    }
};

class TStartCommand: public TClientCommand
{
protected:
    TString GetName() override
    {
        return "Start";
    }

    bool PerformInner(const TVector<TString>& hosts, TVector<TString>& args, TVector<TString>* failed) override;
};

TClientCommand* CreateClientCommand(TString name);
inline TString GenID(TString commandName)
{
    TString user = User;
    TString ts = ::ToString(GetTimeInMillis());
    return user + "-" + commandName + "-" + ts + "-" + ::ToString(RandomNumber(1000000u));
}
TVector<TString> ReReadHostsList();
