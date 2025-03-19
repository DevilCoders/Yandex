#!/usr/bin/env python
import host
import disk
import calculon_config
import datetime
import time
import requests
import socket
import json
import logging
import os
import subprocess as sp

LOG = logging.getLogger(__name__)
logging.basicConfig(filename=calculon_config.client_log, level=logging.DEBUG, format='%(asctime)s %(message)s')

if calculon_config.host_rejection or calculon_config.disk_rejection:
  import reject


def get_delta():
  now = datetime.datetime.now()
  update = datetime.datetime.strptime('01-01-1970', "%d-%m-%Y")
  delta = now - update
  return delta.days


def make_data_to_upload(table, data):
  """
  :param table, str, table in clickhouse
  :param data, dict, data to send
  :returns list:

  """
  res = []
  for el in calculon_config.schema[table]:
    res.append(data[el])
  return res


def read_data_from_calculon():
  pass


def save_data(table, data):
  #TODO clear cache of temporary files once 3 hours.
  safe_path = "{}_{}_{}".format(calculon_config.tmp_dir, table, int(time.time()))
  try:
    with open(safe_path, 'w') as dump:
      dump.write(json.dumps(data))
  except IOError as error:
    LOG.error("Can't save data to file {}, because of :{}\nPrint to log: {}".format(safe_path, error, json.dumps(data)))
  return


def send_data_to_calculon(table, data):
  url = 'http://{}/api/receiver/upload/{}/'.format(calculon_config.collector_endpoint, table)
  response = None
  print(url)
  print(make_data_to_upload(table, data))
  try:
    response = requests.post(url=url, json=json.dumps({'data': make_data_to_upload(table, data)}))
  except requests.ConnectionError as error:
    LOG.error("Can't send data to calculon: {}".format(error))
  if response:
    return response



def get_disks():
  """Looking for disks.
  :returns list of disks"""
  cmd = '/bin/lsblk -o NAME,TYPE --json'
  raw_disk_data = None
  result = None
  disks = []
  try:
    result = sp.Popen(cmd.split(),stdout=sp.PIPE).stdout.read()
  except sp.CalledProcessError as error:
    print("Can't get list of block devices: {}".format(error))
  if result:
    try:
      raw_disk_data = json.loads(result)
    except ValueError as error:  # json.loads in python2 raises ValueError
      print("Malformed json: {}".format(error))
    if raw_disk_data:
      data = raw_disk_data.get('blockdevices')
      for dev in data:
        if dev['type'] == 'disk':
          disks.append(dev['name'])
  return disks


def add_host():
  pass


def add_disk():
  pass


def remove_disk():
  pass


def main():
  host_data = host.Host(socket.getfqdn()).get_compute_unit().get_hwid()
  clickhouse_special = {'Time': int(time.time()), 'Date': get_delta()}
  host_data.__dict__.update(clickhouse_special)
  response = None
  try:
    response = send_data_to_calculon('calculon', host_data.__dict__)
  except Exception as error:
    LOG.error("Can't send data to calculon: {}".format(error))
    save_data('calculon', host_data.__dict__)
    LOG.warning("Trying to save data on disk")
  if response:
    if response.status_code == 200:
      LOG.info("Data sent!")
    else:
      LOG.warning("Data possibly stored on filesystem, run client again to retry")

  disks = get_disks()
  for d in disks:
    possibly_good = disk.Disk(d)
    possibly_good.get_disk_metadata().get_iops()
    possibly_good.__dict__.update(clickhouse_special)
    # if calculon_config.disk_rejection:
    #   rancid = reject.reject(d)
    #   if rancid:
    #     possibly_good.Rancid(datetime.datetime.now())
    #     os.unlink(os.path.join('/opt/benchmarks/results/iops/', '{}.json'.format(d)))
    send_data_to_calculon('disks', possibly_good.__dict__)

if __name__ == "__main__":
  main()
