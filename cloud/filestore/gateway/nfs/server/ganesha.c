#include "nfs_init.h"

#include <contrib/restricted/nfs_ganesha/src/include/nfs_core.h>

// this function is perfectly exported, but not defined in headers
extern int nfs_libmain(
    const char* ganesha_conf,
    const char* lpath,
    int debug_level);

////////////////////////////////////////////////////////////////////////////////

int nfs_ganesha_start(
    const char* config_file,
    const char* log_file,
    const char* pid_file,
    const char* recov_root)
{
    if (config_file && strlen(config_file) == 0) {
        config_file = NULL;
    }

    if (log_file && strlen(log_file) == 0) {
        log_file = NULL;
    }

    if (pid_file && strlen(pid_file)) {
        nfs_pidfile_path = gsh_strdup(pid_file);
    }

    if (recov_root && strlen(recov_root)) {
        nfs_v4_recov_root = gsh_strdup(recov_root);
    }

    return nfs_libmain(config_file, log_file, -1);
}

void nfs_ganesha_stop()
{
    LogEvent(COMPONENT_MAIN, "Stopping admin thread");

    admin_halt();
}
