#include <algorithm>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include <util/system/daemon.h>
#include <util/string/util.h>
#include <util/generic/hash.h>
#include <util/generic/buffer.h>
#include <util/string/split.h>

#include <util/stream/output.h>

#include "comm.h"

int RunSHCommand(const TString& command, const TString& wd, pid_t* expid, bool createPG, TOutputHandler* oh)
{
    createPG = true;
    int pipeFd[2];
    if (oh) {
        pipe(pipeFd);
    }
    int pid = fork();
    if(pid) {
        setpgid(pid, pid);
        int gpid = pid;
        if (expid)
            *expid = gpid;
        int status;
        if (oh) {
            close(pipeFd[1]);
            TBuffer buffer(1024 * 1024);
            int rd;

            while (waitpid(pid, &status, WNOHANG) == 0) {
                if ((rd = read(pipeFd[0], buffer.Data(), buffer.Capacity())) > 0)
                    oh->Write(buffer.Begin(), rd);
            }
            while ((rd = read(pipeFd[0], buffer.Data(), buffer.Capacity())) > 0)
                oh->Write(buffer.Begin(), rd);
        }

        int rc = 0;
        while ((rc = waitpid(-gpid, &status, WNOHANG)) == 0) { // children exist
//            Cout << "Found children" << rc << Endl;
            waitpid(-gpid, &status, 0);
        }
//        if (rc == -1)
//            perror("");
//        waitpid(-gpid, &status, 0);
//        Cout << "RC: " << status << Endl;
//        waitpid(-gpid, &status, 0);
        return status;
    }
    else {
        if (createPG) {
            setsid();
            //setpgid(0, 0);    // separate group in case we'll kill it
            signal(SIGINT, SIG_IGN);
        }
        fclose(stdin);
        if (oh) {
            close(pipeFd[0]);
            dup2(pipeFd[1], 1);
            dup2(pipeFd[1], 2);
        }
//        Clog << command << Endl;
        TString sh = "sh";
        TString c = "-c";
        TString commandCopy;

        if (command.find('\t') != TString::npos) {
            TVector<TString> split;
            TString quoted;
            StringSplitter(command).Split('\t').AddTo(&split);
            for (int i = 0; i < split.ysize(); i++) {
                if (i)
                    quoted.append(" ");

                if (split[i].find_first_of("=<>") != TString::npos) {
                    quoted.append(split[i]);
                }
                else {
                    quoted.append("\'");
                    TStringBuf argSrc(split[i]);
                    TStringBuf l, r;
                    while (argSrc.TrySplit('\'', l, r)) {
                        quoted.append(l);
                        quoted.append("\'\\\'\'");
                        argSrc = r;
                    }
                    quoted.append(argSrc);
                    quoted.append("\'");
                }
            }
            commandCopy = quoted;
        } else {
            commandCopy = command;
        }

        char* const params[] = {
            sh.begin(),
            c.begin(),
            commandCopy.begin(),
            nullptr
        };
        NDaemonMaker::CloseFrom(3);
        if (!wd.empty()) {
            chdir(wd.data());
        }
        execv("/bin/sh", params);
        exit(-1); // never ever got this executed
    }
}
