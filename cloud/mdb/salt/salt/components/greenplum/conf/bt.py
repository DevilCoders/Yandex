{% from "components/greenplum/map.jinja" import gpdbvars with context %}
{% set bin_path="%s-%s/bin"|format(gpdbvars.gphome, gpdbvars.gpmajver) %}
#!/usr/bin/python

import subprocess
import sys
import time
import signal
import os

global p
p = None

def sighandler(signum, frame):
    global p
    if signum in (signal.SIGINT, signal.SIGQUIT):
        p.wait()
    sys.exit(0)

def preexec_function():
    os.setpgrp()

def parse_args(argv):
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("-p", help="pid of the process", action="store", dest="pid")
    parser.add_option("-i", help="[ %default ] Number of backtraces to take)", default=1, action="store", dest="iterations")
    parser.add_option("-s", help="[ %default ] Time to sleep between backtraces)", default=1.0, action="store", dest="sleep")
    (options, arguments) = parser.parse_args(argv)
    if (len(arguments)>1):
        raise Exception("Tool takes no arguments.")
    if options.pid is None:
        raise Exception("Pid of the process to take backtraces is mandatory.")
    return options

if __name__ == '__main__':
    options = parse_args(sys.argv)
    if os.access('/proc/%s/status' % options.pid, os.R_OK):
        pid = int(options.pid)
    else:
        print('No pid %s. Exiting.' % options.pid)
        sys.exit(0)
    try:
        sleep = float(options.sleep)
        iterations = int(options.iterations)
    except Exception:
        print('Could not parse command-line options. Exiting.')
        sys.exit(1)

    signal.signal(signal.SIGINT, sighandler)
    signal.signal(signal.SIGQUIT, sighandler)
    count = 0
    while count < iterations:
        print(time.ctime())
        cmd = 'gdb --nx -batch -x /usr/local/yandex/gdb_bt_cmd {{ bin_path }}/postgres %d' % pid
        res = None
        p = subprocess.Popen(cmd, shell=True, stdin=open('/dev/null', 'r'), stdout=sys.stdout, stderr=open('/dev/null', 'w'), preexec_fn=preexec_function)
        p.wait()
        print('\n')
        count += 1
        time.sleep(sleep)
