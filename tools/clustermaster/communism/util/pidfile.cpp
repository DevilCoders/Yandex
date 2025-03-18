#include "pidfile.h"

#include <library/cpp/coroutine/engine/impl.h> // for EAGAIN definition only

#include <util/folder/path.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/string/util.h>
#include <util/system/error.h>
#include <util/system/file.h>

TPidFile::TPidFile(const TString& path)
    : Path(path)
    , File(path, OpenAlways | RdWr)
    , DeleteOnExit(false)
{
    try {
        File.Flock(LOCK_EX | LOCK_NB);
    } catch (const TFileError& e) {
        if (LastSystemError() == EAGAIN) {
            TString pid;

            TUnbufferedFileInput(File).ReadLine(pid);

            throw yexception() << "the pidfile " << path << " is already in use by process " << pid;
        }
        throw yexception() << "cannot lock the pidfile " << path << ": " << e.what();
    }

    Update();
}

TPidFile::~TPidFile() {
    // If the process calling destructor is not the one which is
    // pidfile owner, do nothing - handles should be closed
    // in system(), popen(), or child code after fork
    if (getpid() == Pid) try {
        File.Close();
        // Don't remove the file - this should be handled by rc script
    } catch (...) {
    }

    if (DeleteOnExit) {
        try {
            TFsPath(Path).DeleteIfExists();
        } catch (...) {
        }
    }
}

void TPidFile::Update() {
    File.Seek(0, sSet);
    File.Resize(0);

    Pid = getpid();

    TUnbufferedFileOutput out(File);
    out << static_cast<int>(Pid) << '\n';
}

void TPidFile::SetDeleteOnExit() {
    DeleteOnExit = true;
}
