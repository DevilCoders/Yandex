# This module should be included if you need any third-party package provided by gencfg installer
# Installation script puts all packages into virtualenv venv folder
import sys, os
import subprocess
import time
import fcntl


def prompt(question):
    while True:
        try:
            i = raw_input("%s (type 'yes' or 'no'): " % question)
        except KeyboardInterrupt:
            return False
        if i.lower() in ('yes', 'y'):
            return True
        elif i.lower() in ('no', 'n'):
            return False


def run_command(args, raise_failed=True, timeout=None, sleep_timeout=0.1, cwd=None, close_fds=False, stdin=None):
    """
        Wrapper on subprocess.Popen

        :return (int, str, str): return triplet of (returncoce, stdout, stderr)
    """

    if timeout is not None:
        sleep_timeout = min(timeout / 10, sleep_timeout)

    try:
        if stdin is None:
            p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=cwd, close_fds=close_fds)
        else:
            p = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=cwd,
                                 close_fds=close_fds)
    except Exception, e:
        if raise_failed:
            raise Exception("subpocess.Popen for <<%s>> failed: %s" % (args, e))
        else:
            return 1, 'Got unknown error %s' % e, ''

    try:
        if stdin is not None:
            p.stdin.write(stdin)  # Is it correct ?
            p.stdin.close()

        if timeout is None:
            out, err = p.communicate()
            if p.returncode != 0 and raise_failed:
                raise Exception("Command <<%s>> returned %d\nStdout:%s\nStderr:%s" % (args, p.returncode, out, err))
        else:
            out, err = '', ''
            wait_till = time.time() + timeout

            fcntl.fcntl(p.stdout, fcntl.F_SETFL, fcntl.fcntl(p.stdout, fcntl.F_GETFL) | os.O_NONBLOCK)
            fcntl.fcntl(p.stderr, fcntl.F_SETFL, fcntl.fcntl(p.stderr, fcntl.F_GETFL) | os.O_NONBLOCK)

            while time.time() < wait_till:
                p.poll()
                try:
                    while True:
                        r = os.read(p.stdout.fileno(), 1024)
                        out += r
                        if len(r) == 0:
                            break
                except OSError:
                    pass
                try:
                    while True:
                        r = os.read(p.stderr.fileno(), 1024)
                        err += r
                        if len(r) == 0:
                            break
                except OSError:
                    pass
                if p.returncode == 0:
                    return p.returncode, out, err
                if p.returncode is not None:
                    if raise_failed:
                        raise Exception("Command <<%s>> returned %d\nStdout:%s\nStderr:%s" % (args, p.returncode, out, err))
                    else:
                        return p.returncode, out, err
                time.sleep(sleep_timeout)

            if raise_failed:
                raise Exception("Command <<%s>> timed out (%f seconds)" % (args, timeout))
    finally:
        if p.returncode is None:
            p.kill()
            p.wait()

    return p.returncode, out, err


def update_venv(install_sh):
    if os.isatty(sys.stdout.fileno()):
        reply = prompt("Your venv needs to be updated. Update?")
        if not reply:
            raise Exception("Venv was not updated, can not proceed ...")

    print "Updating venv..."
    status, out, err = run_command([install_sh], raise_failed=False)
    if status != 0:
        raise Exception("Install command <%s> failed with status <%s> stdout <%s> and stderr <%s>" % (install_sh, status, out, err))

    print "Updating of venv finished"


# check venv version
if 1:
    mydir = os.path.dirname(__file__)
    repo_resources = os.path.join(mydir, 'sandbox.resources')
    installed_resources = os.path.join(mydir, 'sandbox.resources.installed')

    # check for existance of some installed files (optional)
    venv_dir = os.path.abspath(os.path.join(mydir, 'venv'))
    install_sh = os.path.abspath(os.path.join(mydir, "install.sh"))
    if not os.path.exists(venv_dir):
        update_venv(install_sh)

    # check for existance of installed resources
    if not os.path.exists(installed_resources):
        update_venv(install_sh)

    # old installed resources
    if open(repo_resources).read() != open(installed_resources).read():
        update_venv(install_sh)


# add path to venv to our sys.path
import gencfg0
