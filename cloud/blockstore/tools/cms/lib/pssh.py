import json
import logging
import os
import subprocess
import sys
import threading


OUTPUT_PREFIX = 'OUT[0]:'
DEFAULT_PATH = 'yc-pssh'


class ErrorHolder(object):

    def __init__(self):
        self.failed = False

    def set(self):
        self.failed = True


class Pssh(object):

    def __init__(self, exe=None):
        self.__exe = exe if exe else os.getenv('YC_PSSH', DEFAULT_PATH)

    def scp(self, host, src, dst, attempts=10):
        return self.__scp(f'{host}:{src}', dst, attempts)

    def scp_to(self, host, src, dst, attempts=10):
        return self.__scp(src, f'{host}:{dst}', attempts)

    def __scp(self, src, dst, attempts):
        def f():
            logging.info(f'[pssh.scp] {src} -> {dst}')

            p = subprocess.Popen(
                [self.__exe, 'scp', src, dst],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                bufsize=1,
                close_fds=True,
                encoding="utf-8",
                text=True
            )

            error_holder = ErrorHolder()
            t = threading.Thread(
                target=self.__process_stderr,
                args=(p.stderr, error_holder)
            )
            t.start()

            for data in iter(p.stdout.read, ''):
                s = data.strip()
                logging.debug(f'[pssh.scp] output: {s}')
                if s != "Completed 1/1":
                    return None

            t.join()

            return True if p.poll() == 0 and not error_holder.failed else None

        return self.__retry(f, attempts)

    def run(self, cmd, target, attempts=10):
        def f():
            logging.info(f'[pssh.run] {cmd} {target}')

            p = subprocess.run(
                [self.__exe, 'run', '--format', 'json', cmd, target],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False)

            if p.returncode != 0:
                logging.error(f"[pssh.run] {p.stderr} ({p.returncode})")
                return None

            logging.info(f'[pssh.run] response: {p.stdout}')

            resp = json.loads(p.stdout)

            resp['stdout'] = resp['stdout'].rstrip()
            resp['stderr'] = resp['stderr'].rstrip()

            if len(resp['stderr']) != 0:
                logging.error(f"[pssh.run] response: {resp['stderr']}")

            if resp['exit_status'] != 0:
                logging.error(
                    f"[pssh.run] cmd error: {resp['exit_status']} {resp['error']}")
                return None

            return resp['stdout']

        return self.__retry(f, attempts)

    def __retry(self, f, attempts):
        while attempts > 0:
            res = f()
            if res is not None:
                return res
            else:
                attempts -= 1

        logging.error('pssh fatal error - max attempt count exceeded')
        return None

    def __process_stderr(self, out, error_holder):
        for line in iter(out.readline, ''):
            if not line.strip().startswith("Completed "):
                print(line.lstrip(), end='', file=sys.stderr)
            if line.find("ERROR") >= 0 or line.find("Remote exited without signal") >= 0:
                error_holder.set()
        out.close()

    def resolve(self, query):
        return subprocess.run(
            [self.__exe, 'list', '-l', query],
            check=True,
            text=True,
            stdout=subprocess.PIPE).stdout
