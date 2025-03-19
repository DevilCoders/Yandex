#!/usr/bin/env python2
#
# Provides: walle_cpu
#

import sys
import json
import subprocess


def die(exit_num, exit_str):
    print "PASSIVE-CHECK:walle_cpu;%d;%s" % (exit_num, exit_str)
    sys.exit(0)


def read_content(file):
    with open(file, "rt") as src:
        return src.read()

def check_board_name():
    # Skip H8DGU board ( https://st.yandex-team.ru/RND-390 )
    cmd = "sudo dmidecode -s baseboard-product-name"
    output = subprocess.check_output(cmd.split())
    output = output.strip()
    if output in ['H8DGU', 'Z8NR-D12', 'MZ91-FS0-00']:
        return True
    else:
        return False

def offline_cores_detect():
    cpu_sys_path = "/sys/devices/system/cpu"
    cores = read_content(cpu_sys_path + '/offline')
    cores = cores.strip()
    if cores != "":
        if check_board_name():
            return (1, "offline cores: {}; H8DGU borad platform".format(cores))
        else:
            return (2, "offline cores: " + cores)
    else:
        return (0, "Ok")


def main():
    result = {'reason': ['Ok']}
    status = 0
    (res, msg) = offline_cores_detect()
    if res != 0:
        result = {"reason": [msg]}
        status = res

    die(status, json.dumps({"result": result}))


if __name__ == '__main__':
    main()
