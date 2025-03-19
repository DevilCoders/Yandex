#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////

int nfs_ganesha_start(
    const char* config_file,
    const char* log_file,
    const char* pid_file,
    const char* recov_root);

void nfs_ganesha_stop();

#if defined(__cplusplus)
}   // extern "C"
#endif
