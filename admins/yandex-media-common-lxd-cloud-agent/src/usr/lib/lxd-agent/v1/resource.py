from subprocess import run, PIPE
import re
import os
import psutil
import socket
import lvm
import logging

def search():

    cmd = ['lsblk -d -o name,rota | grep -v NAME']
    result = run(cmd, shell=True, stdout=PIPE)

    output = result.stdout.decode('utf-8')
    output = re.sub( '\s+', ' ', output )
    output = output.strip()

    results = output.split(' ')
    disks = {}

    for element in results:
        if element == '1':
            disks['hdd'] = 1
        elif element == '0':
            disks['ssd'] = 1

    if 'ssd' in disks and 'hdd' not in disks:
        disk_type = 'ssd'
    elif 'hdd' in disks and 'ssd' not in disks:
        disk_type = 'hdd'
    else:
        disk_type = 'mixed'

    try:
        vgSize = lvm.vgOpen('lxd','r').getSize()
    except lvm.LibLVMError as error:
        vgSize = 0

    result = {'hostname': socket.gethostname(),
              'cpu': os.cpu_count(),
              'memory': psutil.virtual_memory().total,
              'diskType': disk_type,
              'vgSize' : vgSize }

    return result
