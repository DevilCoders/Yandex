import os
import stat


def get_abs_path(*relative):
    script_dir = os.path.dirname(os.path.realpath(__file__))
    return os.path.normpath(os.path.join(script_dir, *relative))


# FIXME: these functions represent test parameters and prerequisites
# May be its better to setup this by config.ini
# They should be part of setup process, can be
# configured but must have sane default values.
def get_test_runner():
    return os.getenv('TEST_RUNNER', '')


def get_qemu_bin():
    default_qemu_bin = 'qemu-system-x86_64'
    # try envar first.
    qemu_bin = os.getenv('TEST_QEMU_BIN', default_qemu_bin)
    return qemu_bin


def get_aio_server_binary():
    return get_abs_path('../aio-server')


def get_virtiofs_server_binary():
    return get_abs_path('../virtiofs-server/virtiofs-server')


def get_ssh_key():
    key_path = get_abs_path('../id_rsa')
    permission_mask = stat.S_IRUSR | stat.S_IWUSR
    if os.stat(key_path).st_mode & ~permission_mask:
        os.chmod(key_path, permission_mask)
    return key_path
