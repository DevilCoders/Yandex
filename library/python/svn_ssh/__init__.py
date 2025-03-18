import collections
import contextlib
import os
import logging
import tempfile
import threading

try:
    import urllib.parse as urlparse
except ImportError:
    import urlparse

logger = logging.getLogger(__name__)


@contextlib.contextmanager
def svn_ssh(login, key):
    if callable(key):
        key = key()

    old_svn_ssh = os.environ.get("SVN_SSH")

    with tempfile.NamedTemporaryFile(prefix="ssh") as f:
        f.write(key)
        f.flush()

        logger.debug("Using login %s and key %s", login, f.name)
        clean_svn_ssh = _sanitize_svn_ssh(
            os.environ.get("SVN_SSH", "ssh"),
            rm_options=["UserKnownHostsFile", "StrictHostKeyChecking"],
            rm_args=["-i", "-l"],
        )
        os.environ["SVN_SSH"] = (
            "{svn_ssh} -i {key} {login} -o UserKnownHostsFile={devnull} -o StrictHostKeyChecking=no"
        ).format(
            svn_ssh=clean_svn_ssh,
            key=f.name,
            login='-l {}'.format(login) if login else '',
            devnull=os.devnull
        )

        logger.debug("svn_ssh: Using SVN_SSH %s", os.environ["SVN_SSH"])

        yield

        if old_svn_ssh:
            os.environ["SVN_SSH"] = old_svn_ssh
        else:
            del os.environ["SVN_SSH"]


@contextlib.contextmanager
def ssh_multiplex(url, close_on_exit=True, tunnels_mgr_cls=None):
    if tunnels_mgr_cls is None:
        tunnels_mgr_cls = Tunnels

    old_svn_ssh = os.environ.get("SVN_SSH")

    parsed_url = urlparse.urlparse(url)
    user, host, port = parsed_url.username, parsed_url.hostname, str(parsed_url.port or '')

    multiplex_opts = tunnels_mgr_cls.ensure_tunnel(user, host, port)
    if not multiplex_opts:
        logger.debug("SSH multiplexing is disabled")
    else:
        clean_svn_ssh = _sanitize_svn_ssh(os.environ.get("SVN_SSH", "ssh"), rm_options=[
            'ControlMaster',
            'ControlPersist',
            'ControlPath',
        ])
        os.environ["SVN_SSH"] = "{} {}".format(clean_svn_ssh, multiplex_opts)

    logger.debug("ssh_multiplex: Using SVN_SSH %s", os.environ["SVN_SSH"])

    yield

    if old_svn_ssh:
        os.environ["SVN_SSH"] = old_svn_ssh
    else:
        del os.environ["SVN_SSH"]

    if close_on_exit:
        Tunnels.close_tunnel(user, host, port)


def sanitize_svn_ssh(svn_ssh, rm_args=None, rm_options=None):
    return _sanitize_svn_ssh(svn_ssh, rm_args, rm_options)


def _sanitize_svn_ssh(svn_ssh, rm_args=None, rm_options=None):
    rm_options = rm_options or []
    rm_args = rm_args or []
    clean_svn_ssh = []

    arg_name = None
    for arg in svn_ssh.split():
        if arg.startswith("-"):
            if arg_name and arg_name not in rm_args:
                clean_svn_ssh.append(arg_name)
            arg_name = arg
        else:
            if arg_name not in rm_args and not (arg_name == "-o" and arg.split("=", 1)[0] in rm_options):
                if arg_name:
                    clean_svn_ssh.append(arg_name)
                clean_svn_ssh.append(arg)
            arg_name = None

    if arg_name and arg_name not in rm_args:
        clean_svn_ssh.append(arg_name)

    clean_svn_ssh = ' '.join(clean_svn_ssh)

    return clean_svn_ssh


class Tunnels(object):
    _tunnels = {}  # (user, host, port) -> (svn ssh options string, control socket path)
    _lock = threading.Lock()

    _Multiplex = collections.namedtuple("_Multiplex", ["opts", "control_path"])

    @classmethod
    def ensure_tunnel_url(cls, url, custom_control_path=None):
        parsed_url = urlparse.urlparse(url)
        cls.ensure_tunnel(parsed_url.username, parsed_url.hostname, str(parsed_url.port or ''))

    @classmethod
    def close_tunnel_url(cls, url):
        parsed_url = urlparse.urlparse(url)
        cls.close_tunnel(parsed_url.username, parsed_url.hostname, str(parsed_url.port or ''))

    @classmethod
    def ensure_tunnel(cls, user, host, port):
        with cls._lock:
            mx = cls._tunnels.get((user, host, port), None)
            if mx is None:

                mx = cls._create_tunnel(user, host, port)
                cls._tunnels[(user, host, port)] = mx

        return mx.opts

    @classmethod
    def close_tunnel(cls, user, host, port):
        with cls._lock:
            mx = cls._tunnels.pop((user, host, port), None)
            if mx:
                try:
                    os.unlink(mx.control_path)
                    logger.debug(
                        "multiplexing tunnel for (%s, %s, %s) in %s has been closed",
                        user, host, port, mx.control_path,
                    )
                except OSError:
                    # probably someone else has removed this socket
                    pass

    @classmethod
    def _get_control_path(cls, user, host, port):
        dirname = os.getenv("SVN_SSH_MUX_CP_DIR") or tempfile.mkdtemp(prefix="lpsvn")
        return os.path.join(dirname, "m{}{}{}".format(user, host, port))

    @classmethod
    def _create_tunnel(cls, user, host, port):
        control_path = cls._get_control_path(user, host, port)

        if len(control_path) >= 100:
            logger.debug(
                "SSH multiplexing is disabled because of too long socket path: (%s) %s",
                len(control_path), control_path
            )
            return None
        else:
            logger.debug("SSH multiplexing is enabled: control path: %s (%s)", control_path,
                         os.path.exists(os.path.dirname(control_path)))
            multiplex_opts = "-o ControlMaster=auto -o ControlPersist=3s -o ControlPath={}".format(control_path)
            logger.debug("SVN_SSH multiplex_opts: %s", multiplex_opts)

            return cls._Multiplex(multiplex_opts, control_path)
