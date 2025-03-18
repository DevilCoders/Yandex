import subprocess
import contextlib
import time
import sys

from antirobot.daemon.arcadia_test import util


START_TIMEOUT = 200
START_POLL_DELAY = 0.1
STOP_TIMEOUT = 10
STOP_POLL_DELAY = 0.1


class Subprocess:
    def __init__(self, path_or_process, args=[], mute=False):
        if isinstance(path_or_process, str):
            self.argv = [path_or_process] + args

            if mute:
                self.redir = subprocess.DEVNULL
            else:
                self.redir = None

            self.process = subprocess.Popen(self.argv, stdout=self.redir, stderr=self.redir)
        elif isinstance(path_or_process, subprocess.Popen):
            self.argv = None
            self.process = path_or_process
        else:
            raise TypeError("Invalid type of 'path_or_process'")

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        self.terminate()
        self.process.__exit__(exc_type, exc, tb)

    def terminate(self):
        self.process.terminate()
        self.process.wait()

    @contextlib.contextmanager
    def pause(self):
        if self.argv is None:
            raise ValueError(
                "Subprocess.pause can only be called on processes constructed with explicit "
                "arguments"
            )

        self.terminate()

        try:
            yield self
        finally:
            self.process = subprocess.Popen(self.argv, stdout=self.redir, stderr=self.redir)


class NetworkWaitable:
    def __init__(self, port, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.host = f"localhost:{port}"
        self.port = port

    def all_ports(self):
        return [self.port]

    def wait(self):
        for _ in range(round(START_TIMEOUT / START_POLL_DELAY)):
            time.sleep(START_POLL_DELAY)

            poll_returncode = self.process.poll()
            assert poll_returncode is None, f"Network subprocess of type {type(self)} died: {poll_returncode}"

            if all(util.service_available(port) for port in self.all_ports()):
                return

        raise Exception(f"Timed out waiting for network subprocess of type {type(self)}")


class NetworkSubprocess(NetworkWaitable, Subprocess):
    def __init__(self, path_or_process, port, args=[], **kwargs):
        super().__init__(port, path_or_process, args, **kwargs)

    @contextlib.contextmanager
    def pause(self):
        with super().pause():
            self.wait()
            yield self

    def wait_port(self, port):
        """
        Порт не освобождается сразу же после process.terminate() и process.wait(), какое-то время ядро его ещё держит
        Нам для старта моков важно дождаться освобождения, чтобы после рестарта на этом же порту не было ошибки "address already in use"
        """

        used = []
        first = True
        for _ in range(round(STOP_TIMEOUT / STOP_POLL_DELAY)):
            p = subprocess.Popen(['ss', '-na', '--tcp'],
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)

            (stdout, stderr) = p.communicate(input='')
            assert p.returncode == 0
            used = []
            for line in stdout.decode().split('\n')[1:-1]:
                # skip title and last empty string

                try:
                    state, recv_q, send_q, local_addr, peer_addr = line.split()
                except:
                    print(line, file=sys.stderr)
                    raise

                if local_addr.endswith(f":{port}"):
                    used.append((state, recv_q, send_q, local_addr, peer_addr))

            if first:
                first = False
                print(f"AAA {port}: {', '.join(map(lambda x: ' '.join(x), used))}", file=sys.stderr)

            if len(used) == 0:
                return

            time.sleep(STOP_POLL_DELAY)

        assert False, f"Port {port} already in use: {', '.join(map(lambda x: ' '.join(x), used))}"
