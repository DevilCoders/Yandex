import io
import os
import sys
import tty
import time
import locale
import codecs
import select
import socket
import termios
import paramiko
import subprocess


CalledProcessError = subprocess.CalledProcessError
TimeoutExpired = subprocess.TimeoutExpired
CompletedProcess = subprocess.CompletedProcess


DEVNULL = subprocess.DEVNULL
PIPE = subprocess.PIPE
STDOUT = subprocess.STDOUT


class _ProxySocket(object):
    """
    Timeout in paramiko is broken.
    This wrapper implements hard timeout.
    """

    def __init__(self, sock, timeout=None):
        self._sock = sock
        self._timeout = timeout
        self._last_time = None
        self._blackhole = False

    def _set_time(self):
        self._last_time = time.time()

    def _check_time(self):
        if self._last_time is not None and self._timeout is not None and \
                time.time() - self._last_time > self._timeout:
            self._sock.shutdown(socket.SHUT_RDWR)
            self._blackhole = True

    def send(self, data):
        if self._blackhole:
            return len(data)
        try:
            ret = self._sock.send(data)
            if ret != 0:
                self._set_time()
            return ret
        except socket.timeout:
            self._check_time()
            raise

    def recv(self, size):
        if self._blackhole:
            return b''
        try:
            ret = self._sock.recv(size)
            if ret != 0:
                self._set_time()
            return ret
        except socket.timeout:
            self._check_time()
            raise

    def close(self):
        self._sock.close()

    @property
    def closed(self):
        return self._sock.closed

    @property
    def _closed(self):
        return self._sock._closed

    def settimeout(self, timeout):
        self._sock.settimeout(timeout)


class SshClient(object):
    def __init__(self, hostname, username=None, password=None,
                 private_key=None, passphrase=None, look_for_keys=None,
                 port=None, connect_timeout=60, keepalive=10):
        """
        SSH client (almost) compatible with subprocess API

        hostname: None - run locally
        username: None - local username
        private_key: bytes
        look_for_keys: try ~/.ssh and agent, None - if no pkey and password

        Supported:
        run()
        call()
        check_call()
        check_output()

        Arguments: args, executable, env, cwd, input, timeout
        encoding, errors, text, universal_newlines
        stdin: sys.stdin (default), DEVNULL, file()
        stdout: sys.stdout (default), PIPE (capture), DEVNULL, file()
        stderr: sys.stderr (default), PIPE (capture), STDOUT, DEVNULL, file()

        Ignored:
        bufsize
        restore_signals
        start_new_session

        Not implemented:
        stdin=PIPE
        preexec_fn
        pass_fds
        close_fds
        Popen()

        Additions:
        shell() or args=None - run shell
        check=False - disable check for file not found
        """

        self.hostname = hostname
        self.username = username
        self._password = password
        self.port = port or paramiko.config.SSH_PORT
        self.connect_timeout = connect_timeout
        self.keepalive = keepalive

        if private_key is not None:
            # guess private key algorithm
            for alg in [paramiko.RSAKey, paramiko.DSSKey, paramiko.ECDSAKey, paramiko.Ed25519Key]:
                try:
                    self._private_key = alg.from_private_key(io.StringIO(private_key.decode('latin1')), passphrase)
                    break
                except paramiko.SSHException as e:
                    saved_exception = e
            else:
                raise saved_exception
        else:
            self._private_key = None

        # by default look for keys or agent if no key or password given
        if look_for_keys is None:
            look_for_keys = password is None and private_key is None
        self.look_for_keys = look_for_keys

        self._client = paramiko.SSHClient()
        self._client.set_missing_host_key_policy(paramiko.client.AutoAddPolicy())
        self._transport = None
        self.connected = False

    def connect(self):
        if self.hostname is not None:
            sock = socket.create_connection((self.hostname, self.port),
                                            timeout=self.connect_timeout)
            proxy = _ProxySocket(sock, timeout=self.connect_timeout)
            self._client.connect(sock=proxy,
                                 hostname=self.hostname,
                                 port=self.port,
                                 timeout=self.connect_timeout,
                                 username=self.username,
                                 password=self._password,
                                 pkey=self._private_key,
                                 allow_agent=self.look_for_keys,
                                 look_for_keys=self.look_for_keys)
            self._transport = self._client.get_transport()
            if self.keepalive:
                self._transport.set_keepalive(self.keepalive)
        self.connected = True

    def close(self):
        if self.hostname is not None:
            self._client.close()
            self._transport = None
        self.connected = False

    def __enter__(self):
        if not self.connected:
            self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.connected:
            self.close()
        return False

    def _run(self, args=None, bufsize=-1, executable=None, check=None,
             stdin=None, stdout=None, stderr=None, input=None,
             encoding=None, errors=None, text=None, universal_newlines=None,
             shell=False, cwd=None, env=None, timeout=None,
             restore_signals=True, start_new_session=False):

        if not self.connected:
            self.connect()

        endtime = None if timeout is None else time.time() + timeout

        chan = self._transport.open_session(timeout=timeout)

        if env is not None:
            chan.update_environment(env)

        if (text or universal_newlines) and encoding is None:
            encoding = locale.getpreferredencoding(False)

        if encoding is None:
            stdout_decoder = None
            stderr_decoder = None
        else:
            stdout_decoder = codecs.getincrementaldecoder(encoding)(errors=errors)
            stderr_decoder = codecs.getincrementaldecoder(encoding)(errors=errors)

        if stdin == DEVNULL:
            stdin_file = None
        elif stdin == PIPE:
            raise NotImplementedError('stdin=PIPE not implemented')
        elif stdin is None:
            # paramiko allows both bytes and unicode as input
            stdin_file = getattr(sys.stdin, 'buffer', sys.stdin)
        else:
            stdin_file = stdin

        if stdout == DEVNULL:
            stdout_file = None
        elif stdout == PIPE:
            # capture stdout
            if encoding is None:
                stdout_file = io.BytesIO()
            else:
                stdout_file = io.StringIO()
        elif stdout is None:
            stdout_file = sys.stdout
            if encoding is None:
                # paramiko output is bytes
                stdout_file.flush()
                stdout_file = getattr(stdout_file, 'buffer', stdout_file)
        else:
            stdout_file = stdout

        if stderr == DEVNULL:
            stderr_file = None
        elif stderr == STDOUT:
            chan.set_combine_stderr(True)
            stderr_file = None
        elif stderr == PIPE:
            # capture stderr
            if encoding is None:
                stderr_file = io.BytesIO()
            else:
                stderr_file = io.StringIO()
        elif stderr is None:
            stderr_file = sys.stderr
            if encoding is None:
                # paramiko output is bytes
                stderr_file.flush()
                stderr_file = getattr(stderr_file, 'buffer', stderr_file)
        else:
            stderr_file = stderr

        alloc_tty = stdin is None and stdout is None and stderr is None and \
                    os.isatty(stdin_file.fileno()) and os.isatty(stdout_file.fileno())
        if alloc_tty:
            term = os.getenv('TERM', 'xterm')
            try:
                width, height = os.get_terminal_size()
            except:
                width, height = 80, 25
            chan.get_pty(term=term, width=width, height=height)
            saved_tty = termios.tcgetattr(stdin_file)

        if shell and args is None:
            chan.invoke_shell()
        else:
            if executable:
                if isinstance(args, str):
                    args = args.split(' ')
                args = ['exec', '-a', args[0], executable] + args[1:]
            else:
                if isinstance(args, str):
                    executable = args.split(' ', 2)[0]
                else:
                    executable = args[0]
            if isinstance(args, str):
                if shell:
                    cmd = args
                else:
                    cmd = "'" + args.replace("'", "'\\''") + "'"
            elif shell:
                cmd = '"' + '" "'.join([a.replace('"', '"\\""') for a in args]) + '"'
            else:
                cmd = "'" + "' '".join([a.replace("'", "'\\''") for a in args]) + "'"
            if env is not None:
                # env set by update_environment() filtered by server
                for name, value in env.items():
                    cmd = 'export ' + name + "='" + value.replace("'", "'\\''") + "' && " + cmd
            if cwd is not None:
                cmd = "cd '" + cwd.replace("'", "'\\''") + "' && " + cmd
            chan.exec_command(cmd)

        if stdin == DEVNULL:
            chan.shutdown_write()

        if stdout == DEVNULL and stderr in (DEVNULL, STDOUT):
            chan.shutdown_read()

        try:
            if alloc_tty:
                tty.setraw(stdin_file.fileno())
                tty.setcbreak(stdin_file.fileno())

            rlist = [chan]
            if stdin_file is not None:
                rlist += [stdin_file]

            while True:
                r, w, e = select.select(rlist, [], [], 1)

                if chan.recv_stderr_ready():
                    x = chan.recv_stderr(65536)
                    if stderr_decoder is not None:
                        x = stderr_decoder.decode(x)
                    if stderr_file is not None:
                        stderr_file.write(x)
                        stderr_file.flush()

                if chan.recv_ready():
                    x = chan.recv(65536)
                    if stdout_decoder is not None:
                        x = stdout_decoder.decode(x)
                    if stdout_file is not None:
                        stdout_file.write(x)
                        stdout_file.flush()

                if chan.send_ready():
                    if input is not None:
                        if len(input) and chan.send_ready():
                            sent = chan.send(input)
                            input = input[sent:]
                            if not input:
                                chan.shutdown_write()
                    elif stdin_file is not None and stdin_file in r:
                        # file.read() reads second time and blocks
                        x = os.read(stdin_file.fileno(), 65536)
                        if x:
                            chan.sendall(x)
                        else:
                            chan.shutdown_write()

                read_more = True
                if stdout == DEVNULL and stderr in (DEVNULL, STDOUT):
                    read_more = False
                elif chan.recv_ready() or chan.recv_stderr_ready():
                    read_more = True
                elif chan.eof_received or chan.closed:
                    read_more = False

                if not read_more and chan.exit_status_ready():
                    ret = chan.recv_exit_status()
                    break

                if endtime is not None and time.time() >= endtime:
                    out = stdout_file.getvalue() if stdout == PIPE else None
                    err = stderr_file.getvalue() if stderr == PIPE else None
                    raise TimeoutExpired(args, timeout, output=out, stderr=err)
        finally:
            if alloc_tty:
                termios.tcsetattr(stdin_file, termios.TCSADRAIN, saved_tty)

        if stderr_decoder is not None and stderr_file is not None:
            try:
                x = stderr_decoder.decode(b'', final=True)
                stderr_file.write(x)
            except:
                pass

        if stdout_decoder is not None and stdout_file is not None:
            try:
                x = stdout_decoder.decode(b'', final=True)
                stdout_file.write(x)
            except:
                pass

        if check is None and ret == 127:
            if sys.version_info[0] > 2:
                raise FileNotFoundError("File not found: " + executable)    # noqa
            else:
                raise OSError("File not found: " + executable)

        out = stdout_file.getvalue() if stdout == PIPE else None
        err = stderr_file.getvalue() if stderr == PIPE else None

        return ret, out, err

    def call(self, args, check=None, **kwargs):
        if self.hostname is None:
            return subprocess.run(args, check=check, **kwargs).returncode
        ret, _, _ = self._run(args, check=check, **kwargs)
        if check and ret != 0:
            raise CalledProcessError(ret, args)
        return ret

    def check_call(self, args, **kwargs):
        if self.hostname is None:
            return subprocess.run(args, check=True, **kwargs).returncode
        ret, out, err = self._run(args, check=True, **kwargs)
        if ret != 0:
            raise CalledProcessError(ret, args, output=out, stderr=err)
        return ret

    def check_output(self, args, **kwargs):
        if self.hostname is None:
            return subprocess.run(args, stdout=PIPE, check=True, **kwargs).stdout
        ret, out, err = self._run(args, stdout=PIPE, check=True, **kwargs)
        if ret != 0:
            raise CalledProcessError(ret, args, output=out, stderr=err)
        return out

    def run(self, args, check=None, **kwargs):
        if self.hostname is None:
            return subprocess.run(args, check=check, **kwargs)
        ret, out, err = self._run(args, check=check, **kwargs)
        if check and ret != 0:
            raise CalledProcessError(ret, args, output=out, stderr=err)
        return CompletedProcess(args=args, returncode=ret, stdout=out, stderr=err)

    def shell(self, args=None, **kwargs):
        if self.hostname is None:
            if args is None:
                args = [os.getenv('SHELL', '/bin/sh')]
            return subprocess.call(args, shell=True, stdin=None, stdout=None, stderr=None, **kwargs)
        ret, _, _ = self._run(args, shell=True, stdin=None, stdout=None, stderr=None, **kwargs)
        return ret
