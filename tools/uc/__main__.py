from io import open

import library.python.compress as lpc
import library.python.init_log as lpi

import os
import contextlib
import sys
import argparse
import logging
import multiprocessing
import tarfile
import subprocess
import time


def fake_chown(*args, **kwargs):
    pass


class NoTarFound(Exception):
    pass


def system_tar():
    for p in ['/usr/bin/tar', '/usr/local/bin/tar', '/bin/tar']:
        if os.path.isfile(p):
            return p

    raise NoTarFound()


def set_mtime(fr, to):
    s = os.stat(fr)

    os.utime(to, (int(time.time()), s.st_mtime))

    try:
        os.chown(to, s.st_uid, s.st_gid)
        os.chmod(to, s.st_mode)
    except Exception as e:
        logging.error('%s', e)


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('-c', '--compress', action='store_true')
    parser.add_argument('-d', '--decompress', action='store_true')
    parser.add_argument('-f', '--from', dest='fr', default='')
    parser.add_argument('-t', '--to', default='')
    parser.add_argument('-j', '--threads', type=int, default=0)
    parser.add_argument('-C', '--codec')
    parser.add_argument('-l', '--list', action='store_true', dest='list_codecs')
    parser.add_argument('-v', '--verbose', action='store_true')
    parser.add_argument('-q', '--quiet', action='store_true')
    parser.add_argument('-x', '--untar', action='store_true')
    parser.add_argument('--filter-tar', action='store_true', dest='filter_tar', help=argparse.SUPPRESS)
    parser.add_argument('-m', '--save-mtime', action='store_true')

    args = parser.parse_args()

    if args.verbose:
        level = 'DEBUG'
    elif args.quiet:
        level = 'ERROR'
    else:
        level = 'INFO'

    lpi.init_log(level=level)

    if args.untar:
        args.decompress = True

    if args.filter_tar:
        try:
            os.makedirs(args.to)
        except Exception:
            pass

        try:
            sys_tar = system_tar()

            os.chdir(args.to)
            os.execv(sys_tar, [sys_tar, '-x'])
        except NoTarFound:
            logging.info('no system tar found')

            with contextlib.closing(tarfile.open(fileobj=sys.stdin.buffer, mode='r|')) as f:
                # stupid idiots...
                f.chown = fake_chown
                f.extractall(args.to)

        return

    if args.list_codecs:
        print('\n'.join(lpc.list_all_codecs()))

        return

    if not args.threads:
        args.threads = multiprocessing.cpu_count()

    logging.debug('will use %s CPUs', args.threads)

    procs = []

    def fopen(path, mode):
        if 'r' in mode:
            if path:
                return open(path, mode)

            return sys.stdin.buffer

        if 'w' in mode:
            if args.untar:
                proc = subprocess.Popen([sys.executable, '--filter-tar', '-t', path], shell=False, stdin=subprocess.PIPE)

                procs.append(proc)

                return proc.stdin

            if path:
                return open(path, mode)

            return sys.stdout.buffer

    try:
        if args.compress:
            lpc.compress(args.fr, args.to, codec=args.codec, fopen=fopen, threads=args.threads)

            if args.save_mtime:
                set_mtime(args.fr, args.to)
        elif args.decompress:
            lpc.decompress(args.fr, args.to, codec=args.codec, fopen=fopen, threads=args.threads)

            if args.save_mtime:
                set_mtime(args.fr, args.to)
        else:
            raise Exception('no mode selected')
    finally:
        for p in procs:
            p.stdin.close()

            rc = p.wait()

            if rc:
                raise Exception('shit happen: %s' % rc)


if __name__ == '__main__':
    if '-v' in sys.argv:
        main()
    else:
        try:
            main()
        except Exception as e:
            logging.error('%s', e)
            sys.exit(1)
