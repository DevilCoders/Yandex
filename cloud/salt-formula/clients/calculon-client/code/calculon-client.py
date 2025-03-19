import requests
import subprocess as sp
import json
import yaml
import os
import sys
import hashlib
import datetime
import time
import socket
import logging
import iops
import argparse


LOG = logging.getLogger(__name__)

def make_request_to_calculon(method, handler, entity, server_list, data=None):
  response = None
  for server in server_list:
    url = 'http://{}/calculon/{}/{}'.format(server, handler, entity)
    try:
      if data:
        response = requests.request(method,url,json=data)
      else:
        response = requests.request(method, url)
    except Exception as error:
      LOG.error("Can't make request because of: {}".format(error))
    if response and method == "GET":
      answer = {}
      try:
        answer = json.loads(response.text)
      except Exception as error:
        LOG.error("Can't get data because of {}".format(error))
      if answer:
        return response.status_code, answer
    if response:
      if response.status_code == 200:
        LOG.info("Data successfully processed (method {})".format(method))
      else:
        LOG.info("Data wasn't {}ed with status code: {}".format(method, response.status_code))
        raise requests.HTTPError
    else:
      LOG.error("No response. URL: {}".format(url))
      raise requests.ConnectionError

def make_request_to_lbs(method, handler, data=None):
  url = 'http://[::1]:7807/v1/{}'.format(handler)
  response = None
  try:
    if data:
      response = requests.request(method, url, data=data)
    else:
      response = requests.request(method, url)
  except Exception as error:
    LOG.error("Can't send data to LBS api because of: {}".format(error))
  if response:
    if response.status_code == 200:
      LOG.info("Data: {} successfully sent".format(data))
    return response


def get_delta(update_date):
  now = datetime.datetime.now()
  update = datetime.datetime.strptime(update_date, "%d-%m-%Y")
  delta = now - update
  return delta.days

def send_data(data, server_list):
  response = None
  data['date'] = get_delta('01-01-1970')
  data['time'] = int(time.time())
  try:
    response = make_request_to_calculon('POST', 'receiver', '', server_list, json.dumps(data))
  except Exception as error:
    LOG.error("Can't send data because of: {}".format(error))
  if response:
    if response[0] == 200:
      LOG.info("Data: {} successuflly POSTed to {}".format(data, server_list))
    else:
      LOG.info("Data: {} spoilt on the floor with status code: {}".format(data, response[0]))
  return

def get_disk_data(disk):
  """
  :param disk, str e.g. /dev/sda
  :returns dict: e.g {'sda': '9w38POJLJ009090'}
  """
  disk_name = None
  serial = None
  rotational = None
  cmdline = 'lsblk -o NAME,SERIAL,ROTA {} --json'.format(disk)
  raw_data = None
  try:
    raw_data = sp.Popen(cmdline.split(),stdout=sp.PIPE).stdout.read()
  except Exception as error:
    LOG.error("Can't get dat because of: {}".format(error))
  if raw_data:
    disk_data = {}
    try:
      disk_data = json.loads(raw_data)
    except Exception as error:
      LOG.error("Can't decode data because of: {}".format(error))
    if disk_data:
      if disk_data['blockdevices']:
        for el in disk_data['blockdevices']:
          for key, value in el.iteritems():
            if key == 'name':
              disk_name = value
            if key == 'serial' and value:
              serial = value
              rotational = el['rota']
            if serial and disk_name:
              return {disk_name: {'serial': serial, 'rotational': rotational}}

def get_lbs_pools():
  handler = 'pools'
  response = None
  try:
    response = make_request_to_lbs('GET', handler)
  except Exception as error:
    LOG.error("Can't get pools because of: {}".format(error))
  if response:
    data = None
    try:
      data = json.loads(response.text)
    except Exception as error:
      LOG.info("Can't parse data, malformed json: {}".format(error))
    if data:
      if 'names' in data:
        return data['names']
    else:
      return False
  else:
    LOG.error("No response from LBS api")


class LBSPool(object):
  def __init__(self, name, pool_type):
    self.name =  name
    self.pool_type = pool_type
    self.data = {}

  def create(self):
    existing_pools = get_lbs_pools()
    if existing_pools:
      if self.name in existing_pools:
        LOG.info("{} exists".format(self.name))
        return self
    else:
      pool_meta = {"name": self.name, "type": self.pool_type, "enabled": True, "default": True }
      self.data.update(pool_meta)
      response = None
      handler = 'pool'
      try:
        response = make_request_to_lbs("POST", handler, json.dumps(pool_meta))
      except Exception as error:
        LOG.error("Can't create pool {} because of {}".format(self.name, error))
      if response:
        if response.status_code == 200:
          LOG.info("Pool {} successfully created".format(self.name))
        else:
          LOG.error("LBS respond with {} status code".format(response.status_code))
      else:
        LOG.error("LBS api unavailable")
    return self

  def get_devices(self):
    response = None
    try:
      handler = 'pool/{}/physdev'.format(self.name)
      response = make_request_to_lbs("GET", handler)
    except Exception as error:
      LOG.error("Can't get list of {} devices because of {}".format(self.name, error))
    if response:
      data = None
      try:
        data = json.loads(response.text)
      except Exception as error:
        LOG.error("Can't parse pool data, malformed json {}".format(error))
      if data:
        if 'physdevices' in data:
          if data['physdevices']:
            self.data['physdevices'] = data['physdevices']
          else:
            self.data['physdevices'] = {}
            LOG.info("Empty pool")
        else:
          LOG.error("No data in response from LBS")
    else:
      LOG.error("No response from LBS")
    return self

  def remove_device(self, devices):
    pass

  def add_devices(self, devices):
    """
    :param devices: list of tuples, el[0] = disk name, el[1] = iops data
    """
    for device in devices:
      device_name,device_metadata = device
      self.data[device_name] = device_metadata
      response = None
      try:
        handler = 'pool/{}/physdev'.format(self.name)
        data = {'name': device_name}
        data.update(device_metadata)
        response = make_request_to_lbs("POST", handler, json.dumps(data))
      except Exception as error:
        LOG.error("No response from LBS api, can't add {} to {} because of {}".format(device_name, self.name, error))
      if response:
        if response.status_code == 200:
          LOG.info("{} successfully added to {}".format(device_name, self.name))
        else:
          LOG.error("Failed to add {} to {} with status {}".format(device_name, self.name, response.status_code))
      else:
        LOG.error("No response from LBS api")
    return self


class Host(object):
  def __init__(self, hostname, deleted):
    self.hostname = hostname
    self.deleted = deleted
    self.data = {}

  def get_hwid(self):
    LOG.info("Getting host's hwid...")
    with open('/sys/class/dmi/id/board_name') as board_name:
      motherboard = board_name.read().strip()
    with open('/proc/cpuinfo') as cpu_info:
      data = cpu_info.readlines()
      cpu_name = ''.join(set(el.split(':')[1].strip() for el in data if 'model name' in el))
      frequency = cpu_name.split()[-1]
    self.data['hwid'] = hashlib.md5("{}.{}.{}".format(motherboard,cpu_name,frequency)).hexdigest()
    return self

  def get_data(self, server_list):
    response = None
    self.deleted = None
    try:
      response = make_request_to_calculon('GET', 'hosts', self.data['hostname'], server_list)
    except Exception as error:
      LOG.info("No data about {}: {}".format(self.data['hostname'], error))
    if response:
      data = None
      if response[0] == 200:
        data = response[1]
      else:
        LOG.error("Bad response from db: {}".format(response[0]))
      if data:
        if data['deleted']:
          self.data['deleted'] = data['deleted']
        if self.data['deleted'] != 0:
          LOG.info('Host {} is out of use'.format(self.data['hostname']))
          return
        else:
          LOG.info("Host {} is active in db".format(self.data['hostname']))
      else:
        LOG.info("Empty data...")
      return data
    else:
      LOG.error("No response from db")

  def get_disks(self):
    result = None
    raw_dev_data = None
    cmd = 'lsblk -o NAME,SERIAL,ROTA --json'
    try:
      result = sp.Popen(cmd.split(),stdout=sp.PIPE).stdout.read()
    except Exception as error:
      LOG.error("Can't get list of blok devices because of: {}".format(error))
    if result:
      try:
        raw_dev_data = json.loads(result)
      except Exception as error:
        LOG.error( "Malformed json: {}".format(error))
    if raw_dev_data['blockdevices']:
      for dev in raw_dev_data['blockdevices']:
        serial = dev['serial']
        rotational = dev['rota']
        for prop_name, data in dev.iteritems():
          if prop_name == 'children':
            for el in data:
              if 'children' in el:
                continue
              else:
                self.data.setdefault('disks', {})
                disk = str(el['name'])
                self.data['disks'][disk] = {}
                self.data['disks'][disk]['serial'] = str(serial)
                self.data['disks'][disk]['rotational'] = rotational
    if not self.data['disks']:
      sys.exit(1),"No free partititons"
    return self

  def calculate_ycu(self, bench_dir):
    LOG.info("Got benchmark directory: {}".format(bench_dir))
    if os.path.exists(os.path.abspath(bench_dir)):
      os.chdir(bench_dir)
    ycu_cmdline = './calculate'
    vcpu_cmdline = './calculate-vcpu'
    ycu = None
    vcpu = None
    try:
      LOG.info("Calculating YCU started...")
      ycu = sp.Popen(ycu_cmdline.split(),stdout=sp.PIPE, stderr=sp.PIPE).communicate()
      LOG.info("Calculating YCU finished")
    except Exception as error:
      LOG.error("Can't run cmd: {} in {}, because of {}".format(ycu_cmdline, bench_dir, error))
    try:
      LOG.info("Calculating vcpu YCU started...")
      vcpu = sp.Popen(vcpu_cmdline.split(), stdout=sp.PIPE, stderr=sp.PIPE).communicate()
      LOG.info("Calculating vcpu YCU finished")
    except Exception as error:
      LOG.error("Can't run cmd: {} in {}, because of {}".format(vcpu_cmdline, bench_dir, error))
    if ycu:
      if not ycu[1]:
        self.data['YCU'] = ycu[0].replace('YCU', '').strip()
      else:
        logging.error("Cmd '{}' failed because of: {}".format(ycu_cmdline, ycu[1].strip()))
    if vcpu:
      if not vcpu[1]:
        self.data['VCPU'] = vcpu[0].replace('YCU', '').strip()
      else:
        logging.error("Cmd '{}' failed because of: {}".format(vcpu_cmdline, vcpu[1].strip()))
    return self

  def get_ycu(self, bench_dir, server_list):
    if self.data['hwid']:
      hwid = self.data['hwid']
      response = None
      try:
        response = make_request_to_calculon('GET', 'hwid', hwid, server_list)
      except Exception as error:
        LOG.error("Can't get YCU, because of: {}".format(error))
        LOG.info("Calculating YCU...")
        self.calculate_ycu(bench_dir)
      if response:
        if response[0] == 200:
          self.data['YCU'] = response[1]['avg_ycu']
          self.data['VCPU'] = response[1]['avg_vcpu_ycu']
    return self

  def get_iops(self, duration, dumps_dir, disks):
    LOG.info('Checking iops in db...')
    response = None

    LOG.info("Calculating iops locally")
    try:
      self.data['iops'] = iops.get_iops(duration, dumps_dir, disks)  # pylint: disable=E1128
    except Exception:
      raise
    if response:
      if response[0] == 200:  # pylint: disable=E1136
        self.data['iops'] = response[1]  # pylint: disable=E1136
    return self

  def change_disk(self, disk, action, duration=None, dump_dir=None):
    disk_meta = get_disk_data(disk)
    serial = disk_meta['serial']
    rotational = disk_meta['rotational']
    disk_name = disk.split('/')[-1]
    lbs_part = ''.join(filter(lambda x: disk_name in x, self.data['disks'].keys()))
    pool = 'hdd' if rotational == 1 else 'ssd'
    if serial in self.data['disks'][lbs_part].values():
      if action == 'remove':
        data = {'status': 3}
        response = None
        try:
          handler = 'pool/{}/physdev/{}'.format(pool, disk_name)
          response = make_request_to_lbs('PATCH', handler, json.dumps(data))
        except Exception as error:
          LOG.error("Can't send data to LBS because of: {}".format(error))
        if response.status_code == 200:
          LOG.info("Data successfully sent")
        else:
          LOG.error("Didn't send data to LBS")
        self.data['disks'].pop(lbs_part)
        self.data['iops'].pop(lbs_part)
        return self
    if action == 'add':
      self.data['disks'].update(disk_meta)
      disk_iops = iops.get_iops(duration,dump_dir, [lbs_part])  # pylint: disable=E1128
      self.data['iops'].update(disk_iops)
      response = None
      try:
        handler = 'pool/{}/physdev'.format(pool)
        response = make_request_to_lbs('GET', handler)
      except Exception as error:
        LOG.error("Can't get data from LBS because of: {}".format(error))
      if response:
        lbs_pool = None
        try:
          lbs_pool = json.loads(response.text)
        except Exception as error:
          LOG.error("Malformed json from LBS: {}".format(error))
        if lbs_pool:
          broken_partition = ''.join([dev['name'] for dev in lbs_pool if dev['status'] == 3])
          response = None
          try:
            handler = 'pool/{}/physdev/{}'.format(pool, broken_partition)
            data = {'name': disk_name}
            data.update(data['iops'][disk_name])
            response = make_request_to_lbs('PUT', handler, data=json.dumps(data))
          except Exception as error:
            LOG.error("Can't update data in LBS because of: {}".format(error))
          if response.status_code == 200:
            LOG.info("Data successfully sent to LBS")
          return self


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('-r', '--remove-disk', nargs=1, dest='remove_disk', action='store', type=str, help='Device for remove')
  parser.add_argument('-a', '--add-disk', nargs=1, dest='add_disk', action='store', type=str, help='Device to add')
  parser.add_argument('-i', '--install', dest='install', action='store_true', help='Skip deleted record')
  parser.add_argument('-d', '--demount', dest='demount', action='store_true', help='Mark host as deleted')
  arg = parser.parse_args()
  settings = None
  cfg_path = '/etc/calculon-client/calculon-client.yaml'
  if os.path.exists(os.path.abspath(cfg_path)):
    with open(cfg_path) as cfg:
      try:
        settings = yaml.load(cfg)
      except Exception as error:
        print("Can't parse config because of: {}".format(error))
  else:
    print("No config file in /etc/calculon-client/calculon-client.yaml")
    sys.exit(1)
  if settings:
    logging.basicConfig(filename=settings['log_file'], level=logging.DEBUG, format='%(asctime)s %(message)s')
    LOG.info("Initialized with settings: {}".format(settings))
  host = Host(socket.getfqdn(), 0)
  host.data['hostname'] = socket.getfqdn()
  host.data['deleted'] = host.deleted
  data = None
  try:
    data = host.get_data(settings['server_list'])
  except Exception as error:
    LOG.info("No data in db: {}".format(error))
    arg.install = True
  if data:
    if 'deleted' in data:
      if data['deleted']:
        host.data['deleted'] = data['deleted']
  if host.data['deleted'] !=0 :
    if arg.install:
      try:
        host.get_hwid()
      except Exception as error:
        LOG.error("Can't calculate hwid because of: {}".format(error))
      try:
        host.get_disks()
      except Exception as error:
        LOG.error("Can't get disks data because of: {}".format(error))
      try:
        host.get_ycu(settings['benchmark_directory'], settings['server_list'])
      except Exception as error:
        LOG.error("Can't calculate YCU because of: {}".format(error))
      try:
        host.get_iops(settings['test_duration'], settings['fio_artifacts'], host.data['disks'].keys())
      except Exception as error:
        LOG.error("Can't get host's iops because of: {}".format(error))
      host.data['deleted'] = 0
      send_data(host.data, settings['server_list'])
      pools = []
      ssd = [disk_name for disk_name, disk_data in host.data['disks'].iteritems() if disk_data['rotational'] == '0']
      hdd = [disk_name for disk_name, disk_data in host.data['disks'].iteritems() if disk_data['rotational'] == '1']
      if ssd:
        ssd_pool = LBSPool('ssd', 'ssd')
        ssd_pool.create()
        pools.append(ssd_pool)
      if hdd:
        hdd_pool = LBSPool('hdd', 'hdd')
        hdd_pool.create()
        pools.append(hdd_pool)
      for pool in pools:
        pool.get_devices()
        if not pool.data['physdevices']:
          if pool.pool_type == 'ssd':
            pool.add_devices([dev for dev in host.data['iops'].iteritems() if dev[0] in ssd])
          if pool.pool_type == 'hdd':
            pool.add_devices([dev for dev in host.data['iops'].iteritems() if dev[0] in hdd])
    else:
      LOG.info("Do nothing, host marked as deleted")
      exit(0)
  if host.data['deleted'] == 0:
    if arg.demount:
      response = None
      try:
        response = make_request_to_calculon('DELETE', 'hosts', host.data['hostname'], settings['server_list'])
      except Exception as error:
        LOG.error("Can't mark host deleted: {}".format(error))
      if response:
        LOG.info("Host successfully marked as deleted")
        exit(0)
    else:
      if arg.add_disk:
        #TODO don't forget about right pool
        #TODO PUT -d '{"name": "loop3", "iopsRandRead": 200, "iopsRandWrite": 200}' http://[::1]:7807/v1/pool/first/physdev/loop0
        pass
      if arg.remove_disk:
        #TODO find out pool disk present
        #TODO mark as broken  PATCH {"status": 3} http://[::1]:7807/v1/pool/hdd/physdev/<devname>
        pass



    #   if arg.add:
    #     host.change_disk(arg.add, 'add', settings['duration'], settings['fio_artifacts'])
    #   if arg.remove:
    #     host.change_disk(arg.remove, 'remove')
    #   if host.data:
    #       print host.data, settings['server_list']
    #       print host.data.keys()
    #       send_data(host.data, settings['server_list'])
    # else:
    #   print "Host deleted in db"

if __name__ == '__main__':
  main()
  #host = Host(socket.getfqdn())
  #host.get_disks()
  #host.get_hwid()
  #host.get_iops(60, '/tmp/fio/dumps', ['man1-fullerene-front-1.cloud.yandex.net'], host.data['disks'].keys())

  #print host.data

