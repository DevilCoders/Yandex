#include "daemon.h"

#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/system/fs.h>

void CreatePidFile(const TString& pidFile) {
    if (NFs::Exists(pidFile)) {
        ythrow yexception() << "pid file " << pidFile
            << " already exist. Will not start." << Endl;
    } else {
        try {
            TFixedBufferFileOutput pid(pidFile);
            pid << getpid();
        } catch (TFileError& ex) {
            ythrow yexception() << "Could not write pid file: " << pidFile
                << ", error: " << ex.what() << Endl;
        }
    }
    DEBUG_LOG << "pid file " << pidFile << " created" << Endl;
}
