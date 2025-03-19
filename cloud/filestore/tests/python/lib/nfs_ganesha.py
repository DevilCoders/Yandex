import os
import retrying

import yatest.common as common

from cloud.filestore.tests.python.lib.common import daemon_log_files

from ydb.tests.library.harness.daemon import Daemon
from ydb.tests.library.harness.kikimr_runner import \
    get_unique_path_for_current_test, ensure_path_exists

import ydb.tests.library.common.yatest_common as yatest_common

NFS_PROGNUM = 100003


CLIENT_CONFIG_TEMPLATE = """
Host: "localhost"
Port: {server_port}
"""

GANESHA_CONFIG_TEMPLATE = """
NFS_Core_Param {{
    NFS_Port = {ganesha_nfs_port};

    # We are only interested in NFSv4 in this configuration
    NFS_Protocols = 4;

    # NFSv4 does not allow UDP transport
    Enable_UDP = False;

    # Disable all other services
    Enable_NLM = False;
    Enable_RQUOTA = False;
}}

LOG {{
    Default_Log_Level = DEBUG;

    COMPONENTS {{
        FSAL = FULL_DEBUG;
    }}
}}
"""

GANESHA_CONFIG_YFS_TEMPLATE = """
EXPORT {{
    # Unique export ID number for this export
    Export_Id = 12345;

    # Path into the NFS tree
    Path = "/";
    Pseudo = "/";

    # We want to be able to read and write
    Access_Type = RW;

    FSAL {{
        Name = YFS;
    }}
}}

YFS {{
    # Client configuration file
    Config_Path = "{config_path}";

    # Filesystem to export
    Filesystem = "{filesystem}";

    # Unique client identifier
    Client = "{client}";
}}
"""

GANESHA_CONFIG_YFS_CLUSTER_TEMPLATE = """
NFSv4 {
    # NFSv4.0 clients do not send a RECLAIM_COMPLETE, so we end up having
    # to wait out the entire grace period if there are any. Avoid them.
    Minor_Versions = 1,2;

    # Setup our recovery backend
    RecoveryBackend = YFS;
}
"""

GANESHA_CONFIG_MEM_TEMPLATE = """
EXPORT {
    # Unique export ID number for this export
    Export_Id = 12345;

    # Path into the NFS tree
    Path = "/";
    Pseudo = "/";

    # We want to be able to read and write
    Access_Type = RW;

    FSAL {
        Name = MEM;
    }
}
"""

GANESHA_CONFIG_VFS_TEMPLATE = """
EXPORT {{
    # Unique export ID number for this export
    Export_Id = 12345;

    # Path into the NFS tree
    Path = "{export_path}";
    Pseudo = "/";

    # We want to be able to read and write
    Access_Type = RW;

    # Squash all users to root (NEVER do it in production setup!)
    Squash = All;
    Anonymous_UID = 0;
    Anonymous_GID = 0;

    FSAL {{
        Name = VFS;
    }}
}}
"""


class _GaneshaConfig():

    def __init__(self, ganesha_port, mon_port):
        self._ganesha_port = ganesha_port
        self._mon_port = mon_port

    def configure(self, working_dir):
        ensure_path_exists(working_dir)

        self._config_file = os.path.join(working_dir, "ganesha.conf")
        self._log_file = os.path.join(working_dir, "ganesha.log")
        self._pid_file = os.path.join(working_dir, "ganesha.pid")
        self._recovery_dir = os.path.join(working_dir, "recovery")

    @property
    def config_file(self):
        return self._config_file

    @property
    def log_file(self):
        return self._log_file

    @property
    def pid_file(self):
        return self._pid_file

    @property
    def recovery_dir(self):
        return self._recovery_dir

    @property
    def mon_port(self):
        return self._mon_port


class _GaneshaMemConfig(_GaneshaConfig):

    def __init__(self, ganesha_port, mon_port):
        super(_GaneshaMemConfig, self).__init__(ganesha_port, mon_port)

    def configure(self, working_dir):
        super(_GaneshaMemConfig, self).configure(working_dir)

        with open(self._config_file, "w") as f:
            f.write(GANESHA_CONFIG_TEMPLATE.format(
                ganesha_nfs_port=self._ganesha_port))
            f.write(GANESHA_CONFIG_MEM_TEMPLATE)


class _GaneshaVfsConfig(_GaneshaConfig):

    def __init__(self, ganesha_port, mon_port, filesystem):
        super(_GaneshaVfsConfig, self).__init__(ganesha_port, mon_port)
        self._filesystem = filesystem

    def configure(self, working_dir):
        super(_GaneshaVfsConfig, self).configure(working_dir)

        export_path = os.path.join(working_dir, self._filesystem)
        ensure_path_exists(export_path)

        with open(self._config_file, "w") as f:
            f.write(GANESHA_CONFIG_TEMPLATE.format(
                ganesha_nfs_port=self._ganesha_port))
            f.write(GANESHA_CONFIG_VFS_TEMPLATE.format(
                export_path=export_path))


class _GaneshaYfsConfig(_GaneshaConfig):

    def __init__(self, ganesha_port, mon_port,
                 server_port, filesystem, clustered):
        super(_GaneshaYfsConfig, self).__init__(ganesha_port, mon_port)
        self._server_port = server_port
        self._filesystem = filesystem
        self._clustered = clustered

    def configure(self, working_dir):
        super(_GaneshaYfsConfig, self).configure(working_dir)
        self._client_config = os.path.join(working_dir, "client.txt")

        with open(self._client_config, "w") as f:
            f.write(CLIENT_CONFIG_TEMPLATE.format(
                server_port=self._server_port))

        with open(self._config_file, "w") as f:
            f.write(GANESHA_CONFIG_TEMPLATE.format(
                ganesha_nfs_port=self._ganesha_port))
            f.write(GANESHA_CONFIG_YFS_TEMPLATE.format(
                config_path=self._client_config,
                filesystem=self._filesystem,
                client="test"))
            if self._clustered:
                f.write(GANESHA_CONFIG_YFS_CLUSTER_TEMPLATE)


class _GaneshaDaemon(Daemon):

    def __init__(self, working_dir, config,
                 verbose=False,
                 restart_interval=None,
                 restart_flag=None):
        config.configure(working_dir)

        server_binary = yatest_common.binary_path(
            "cloud/filestore/gateway/nfs/server/filestore-nfs")

        cmd = [
            server_binary,
            "--config-file", config.config_file,
            "--log-file", config.log_file,
            "--pid-file", config.pid_file,
            "--recovery-dir", config.recovery_dir,
            "--mon-port", str(config.mon_port),
        ]
        if verbose:
            cmd += [
                "--verbose", "trace",
            ]

        if restart_interval:
            launcher_binary = yatest_common.binary_path(
                "cloud/storage/core/tools/testing/unstable-process/storage-unstable-process")

            cmd = [
                launcher_binary,
                "--ping-port", str(config.mon_port),
                "--ping-timeout", "2",
                "--restart-interval", str(restart_interval),
                "--cmdline", " ".join(cmd),
            ]
            if restart_flag:
                cmd += [
                    "--allow-restart-flag", restart_flag,
                ]

        super(_GaneshaDaemon, self).__init__(
            command=cmd,
            cwd=working_dir,
            timeout=180,
            **daemon_log_files(prefix="filestore-nfs", cwd=working_dir))

    def __enter__(self):
        self.start()

    def __exit__(self, type, value, tb):
        self.stop()

    @property
    def pid(self):
        return super(_GaneshaDaemon, self).daemon.process.pid


def _create_nfs_ganesha(config, **kwargs):
    working_dir = get_unique_path_for_current_test(
        output_path=yatest_common.output_path(),
        sub_folder="")

    return _GaneshaDaemon(working_dir, config, **kwargs)


def create_nfs_ganesha_mem(ganesha_port, mon_port, **kwargs):
    config = _GaneshaMemConfig(ganesha_port, mon_port)
    return _create_nfs_ganesha(config, **kwargs)


def create_nfs_ganesha_vfs(ganesha_port, mon_port, filesystem, **kwargs):
    config = _GaneshaVfsConfig(ganesha_port, mon_port, filesystem)
    return _create_nfs_ganesha(config, **kwargs)


def create_nfs_ganesha_yfs(ganesha_port, mon_port,
                           server_port, filesystem, clustered,
                           **kwargs):
    config = _GaneshaYfsConfig(ganesha_port, mon_port,
                               server_port, filesystem, clustered)
    return _create_nfs_ganesha(config, **kwargs)


def _get_universal_address(addr, port):
    hi = int(port / 256)
    lo = int(port) % 256
    return ".".join([addr, str(hi), str(lo)])


@ retrying.retry(stop_max_delay=60000, wait_fixed=1000)
def wait_for_nfs_ganesha(daemon, port):
    '''
    Ping NFS server with delay between attempts to ensure
    it is running and listening by the moment the actual test execution begins
    '''
    if not daemon.is_alive():
        raise RuntimeError("ganesha server is dead")

    cmd = [
        "rpcinfo",
        "-a", _get_universal_address("127.0.0.1", port),
        "-T", "tcp",
        str(NFS_PROGNUM),
    ]

    out = str(common.execute(cmd).stdout)
    if out.find("ready and waiting") == -1:
        raise RuntimeError("rpcinfo status: " + out)
