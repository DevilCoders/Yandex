import subprocess
import json
from subprocess import run, PIPE, SubprocessError
import lvm
import time
import logging

def post(payload):

    logging.basicConfig(level = logging.DEBUG, filename = u'/var/log/lxd-media-agent/agent.log')
    logging.debug(payload)
    newSize = payload['size']
    name = payload['fqdn'].replace('.yandex.net','').replace('-','--').replace('.','----')
    sizeInBytes = "{}B".format(newSize)
    containerPath = "/dev/lxd/containers_{}".format(name)

    try:
        lvresize = run(["/sbin/lvresize", "-L", sizeInBytes, containerPath], shell=False, check=True, stdout = PIPE, stderr = PIPE)
        logging.debug(lvresize)
    except SubprocessError as err:
        logging.error(err)

    try:
        resize2fs = run(["/sbin/resize2fs", containerPath], shell=False, check=True, stdout = PIPE, stderr = PIPE)
        logging.debug(resize2fs)
    except SubprocessError as err:
        logging.error(err)

    try:
        tune2fs = run(["/sbin/tune2fs", "-m1", containerPath], shell=False, check=True, stdout = PIPE, stderr = PIPE)
        logging.debug(tune2fs)
    except SubprocessError as err:
        logging.error(err)

    vg = lvm.vgOpen('lxd', 'r')
    lv_list = vg.listLVs()

    size = 0
    for l in lv_list:
        logging.debug("lv: {}".format(l.getName()))
        if l.getName() == "containers_{}".format(name):
            size = l.getSize()

    fqdn = name.replace('----','.').replace('--','-')+'.yandex.net'

    return {'name': fqdn, 'lvSize': size}
