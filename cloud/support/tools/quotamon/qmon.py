#!/usr/bin/env python3\
# v0.4

import json
import subprocess
import requests
import re
import time
from datetime import datetime
from os import listdir, getcwd
from os.path import isfile, join
import os

script_path = os.path.dirname(__file__)
config_path = script_path+'/conf/'
log_path = script_path+'/logs/'
log_filename = log_path+'%s.txt' % datetime.now().strftime('%Y-%m-%d')


class ConvertValueError(Exception):
    pass


def log_writer(filename, level, module, message):
    try:
        with open(filename, 'a') as file:
            file.write('[%s] [%s] [%s] %s\n' % (
                datetime.fromtimestamp(time.time()), level, module, message))
    except FileNotFoundError as e:  # If logs folder doesn't exist
        os.mkdir(os.path.dirname(filename), mode=0o777)
        log_writer(filename, 'WARN', 'log_writer', e)
        log_writer(filename, 'INFO', 'log_writer',
                   'Creating folder %s' % os.path.dirname(filename))
        with open(filename, 'a') as file:
            file.write('[%s] [%s] [%s] %s\n' % (
                datetime.fromtimestamp(time.time()), level, module, message))

CONVERTABLE_VALUES = (
    'compute.hddDisks.size', 'compute.hddFilesystems.size', 'compute.instanceMemory.size', 'compute.snapshots.size', 'compute.ssdDisks.size', 'compute.ssdNonReplicatedDisks.size',
    'network-hdd-total-disk-size', 'network-ssd-total-disk-size',
    'memory', 'total-snapshot-size',
    'mdb.hdd.size', 'mdb.ssd.size', 'mdb.memory.size',
    'storage.volume.size',
    'serverless.memory.size',
    'managed-kubernetes.memory.size', 'managed-kubernetes.disk.size',
)
SKIP_QUOTAS = (
    'compute.placementGroups.count'
)

# this test repeats the condition of sender
def test(quotas):
    for quota in quotas:
        if 'cloud' in quota:
            print('Облако %s, квота %s: Использовано %s из %s (%s)' % (
                quota['cloud'], quota['name'], quota['used'], quota['limit'], quota['percent']))
        elif 'url' in quota:
            print('Увеличить квоты можно по ссылке - %s' % quota['url'])


def main():
    conffiles = [f for f in listdir(config_path) if isfile(join(config_path, f))] # Создаём список всех файлов конфигов в директории config_path
    for config in conffiles:  # Читаем конфиги по одному и заполняем необходимые параметры
        with open(config_path + config, 'r') as file:
            log_writer(log_filename, "INFO", "main", "read conffile %s" % config)
            cur_conf = file.readlines()
            cur_conf = [[line.rstrip()] for line in cur_conf]
        email_list = cur_conf[1][0].split(',')
        log_writer(log_filename, "INFO", "main",
                   "Email_list = [%s]" % email_list)
        services = cur_conf[3][0].split(',')
        log_writer(log_filename, "INFO", "main", "Services = [%s]" % services)
        clouds = cur_conf[5][0].split((','))
        log_writer(log_filename, "INFO", "main", "Clouds = [%s]" % clouds)
        threshold = [float(i) for i in cur_conf[7]]
        threshold = float(threshold[0])
        log_writer(log_filename, "INFO", "main",
                   "Threshold = [%s]" % threshold)

        quotalist = {'quotas': []}

        for cloud in clouds: # go through all the clouds from the config
            cnt = 0 # count the number of quotas for the current cloud, so that after the last one we can display a link to raising quotas
            url = 'https://console.cloud.yandex.ru/cloud/%s?section=quotas' % cloud # generate url for raising quotas
            for service in services: # go through all the services from the config
                output = subprocess.check_output([script_path+'/quotactl-l2', service, cloud, '--raw']).decode() # run quotactl-l2 with parameters from config and read output
                # output = ansi_escape.sub('', outputB.decode()) # Convert output from binary to string and remove ANSI escape color sequences
                quotas_list = [ # parse prettytable to list
                    [item.strip() for item in line.split('|') if item] # counting the number of columns in a row
                    for line in output.strip().split('\n')
                    if '+-' not in line  # skip row with symbols +-
                ]
                for line in quotas_list:
                    cnt += 1
                    name, limit, usage = line
                    if name in SKIP_QUOTAS:
                        continue
                    else:
                        # Skip first row with zero values or row with headings
                        if limit not in ('0', 'Limit'):
                            if usage == '0':  # if usage == 0 then not need to calculate percent. percent == 0
                                percent = 0
                            else:
                                percent = float(usage) / float(limit)
                        else:
                            continue
                    if name in CONVERTABLE_VALUES:
                        limit = float(limit) / (1024 ** 3)
                        usage = float(usage) / (1024 ** 3)
                    if percent > threshold:
                        quotalist['quotas'].append({'cloud': cloud,
                                                    'name': name,
                                                    'limit': limit,
                                                    'used': usage,
                                                    'percent': str('%.2f %%' % (percent*100))})
            if cnt >= len(quotas_list) and len(quotalist['quotas'])!=0:
                quotalist['quotas'].append({'url': url})
        # test(quotalist['quotas']) # testing sender format
        if not quotalist['quotas']:
            log_writer(log_filename,'INFO','main','nothing to send. Quotalist empty')
        else:
            quotalist = json.dumps(quotalist)  # convert to json format
            for email in email_list:
                send_to_mail(email, quotalist) #commit for test
                #log_writer(log_filename,'INFO','send_to_mail','(dry-run) sending to mail %s' %email) #uncommit for test


def send_to_mail(email, data):
    log_writer(log_filename, 'INFO', 'send_to_mail',
        'Sending notification to %s' % email)
    url = 'https://sender.yandex-team.ru/api/0/yandex.cloud/transactional/G0F342G4-YET1/send'
    params = {'async': 'true',
              'to_email': email,
              'args': data}
    req = requests.post(url=url,
                        params=params,
                        auth=('9cb9a39b5d4d4bc182691e92f577c33b', ''))
    log_writer(log_filename, 'INFO', 'send_to_mail',
        'Send complete with code %i' % req.status_code)


if __name__ == '__main__':
    log_writer(log_filename, 'INFO', 'main', 'SCRIPT STARTED')
    main()
    log_writer(log_filename, 'INFO', 'main', 'SCRIPT COMPLETED')