import sys
import subprocess
import time


def css(data):
    c = 0

    for l in data.split('\n'):
        if l.startswith('< '):
            c += 1

        if l.startswith('> '):
            c += 1

    return c


def ss(p):
    with open(p, 'r') as f:
        return len(f.read())


for cmd in [['./fast_diff'], ['diff', '-a'], ['diff', '-a', '--minimal']]:
    for data in [['3', '4'], ['1', '2']]:
        tt = time.time()
        p = subprocess.Popen(cmd + data, shell=False, stdout=subprocess.PIPE)
        diff, _ = p.communicate()
        tt = int((time.time() - tt) * 1000)

        print 'cmd: ' + ' '.join(cmd + data) + ', change set size: ' + str(css(diff)) + ', time: ' + str(tt) + ' ms, input1: ' + str(ss(data[0])) + ' bytes, input2: ' + str(ss(data[1])) + ' bytes'
