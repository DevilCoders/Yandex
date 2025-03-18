import os


class RunMode:
    DRIVER = 'spyt_driver'
    WORKER = 'spyt_worker'
    DAEMON = 'spyt_daemon'


def _run_daemon():
    from pyspark.daemon import manager
    manager()


def _run_worker():
    from pyspark.worker import main as worker_main
    from pyspark.worker import local_connect_and_auth
    # Read information about how to connect back to the JVM from the environment.
    java_port = int(os.environ["PYTHON_WORKER_FACTORY_PORT"])
    auth_secret = os.environ["PYTHON_WORKER_FACTORY_SECRET"]
    (sock_file, _) = local_connect_and_auth(java_port, auth_secret)
    worker_main(sock_file, sock_file)


def spyt_main(func):
    """
    Adds support for running an executable on a SPYT cluster
    :param func: Callable
    :return: Callable
    """
    mode = os.environ.get("SPYT_RUN_MODE")
    if not mode:
        raise EnvironmentError("SPYT_RUN_MODE must be present in env")
    if mode == RunMode.DRIVER:
        return func
    if mode == RunMode.WORKER:
        return _run_worker
    if mode == RunMode.DAEMON:
        return _run_daemon
    raise ValueError("Invalid SPYT_RUN_MODE `{}`. Valid modes are: {}, {}, {}".format(mode, RunMode.WORKER,
                                                                                      RunMode.DRIVER, RunMode.DAEMON))
