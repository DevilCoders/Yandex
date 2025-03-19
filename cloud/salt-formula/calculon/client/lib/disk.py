import requests
import json
import subprocess as sp
import socket
import calculon_config
import os
import iops


# def read_data_from_calculon():
#   """Asks calculon about disk to compare local and remote data"""
#   url = "http://{}/api/v1/iops/hosts/{}".format(calculon_config.public_endpoint, socket.getfqdn())
#   try:
#     response = requests.get(url, timeout=2)
#   except requests.ConnectionError:
#     return {}
#   if response:
#     if response.status_code == 200:
#       try:
#         data = json.loads(response.text)
#       except json.JSONDecoder:
#         return {}
#       if data:
#         return data.get('data')
#     else:
#       return {}


class Disk(object):
  """
  Loads iops data from disk and compare with calculon
  if it knows something and use rejection method to understand itself able to be
  added under LBS else adds the record into calculon.
  Table structure:
    Model String - str, disk model, lsblk
    Serial String - str, disk serial number (mandatory for disk change), lsblk
    Rotational UInt8 - int (0 or 1), disk rotational factor ssd/hdd, lsblk
    HCTL String - str, md5 hash from platform description /proc/cpuinfo, /sys/class/dmi/id/boardname,
    SCHED String - str, io scheduler, useful for experiments, lsblk
    iopsRandRead UInt32 - int, iops Random Read, calculated or loaded from /opt/fio/metadata/
    iopsRandWrite UInt32 - int, iops Random Write, gotten same way
    iopsSeqRead UInt32 - int, iops Sequental Read, gotten same way
    iopsSeqWrite UInt32 - int, iops Sequental Write, gotten same way
    Hostname String - str, socket.getfqdn()
    Rancid String DEFAULT '' - str, '' or task id in st/ITDC in case of rejection
  """

  def __init__(self, name):
    self.name = name
    self.Model = ''
    self.Serial = ''
    self.Rotational = None
    self.HCTL = ''
    self.SCHED = ''
    self.iopsRandRead = 0
    self.iopsRandWrite = 0
    self.iopsSeqRead = 0
    self.iopsSeqWrite = 0
    self.Hostname = socket.getfqdn()
    self.Rancid = ''
    self.LBS_eligible = ''

  def get_disk_metadata(self):
    """Reads lsblk -o ROTA,SCHED,MODEL,SERIAL"""
    cmd = '/bin/lsblk -o NAME,ROTA,HCTL,SCHED,MODEL,SERIAL,PARTLABEL /dev/{} --json'.format(self.name)
    result = None
    raw_dev_data = None
    try:
      result = sp.Popen(cmd.split(), stdout=sp.PIPE).stdout.read()
    except sp.CalledProcessError as error:
      print("Can't get list of block devices because of: {}".format(error))
    if result:
      try:
        raw_dev_data = json.loads(result)
      except ValueError as error:  # json.loads in python2 raises ValueError
        print("Malformed json: {}".format(error))
    if raw_dev_data:
      dev_data = raw_dev_data.get('blockdevices')
      for dev in dev_data:
        for key in calculon_config.schema['disks']:
          if key.lower() in dev.keys():
            self.__dict__[key] = dev[key.lower()]
          if key == 'Rotational':
            self.Rotational = int(dev['rota'])
        for child in dev['children']:
          if 'children' not in child and 'GRUB' not in dev['partlabel']:
             self.LBS_eligible = child['name']
    return self

  def calculate_iops(self):
    try:
      iops.get_iops(calculon_config.disk_test_duration, calculon_config.fio_tmp, [self.LBS_eligible])
    except RuntimeError as error:
      print("Can't run benchmarks: {}".format(error))

  def get_iops(self):
    """Reads /opt/benchmarks/results/iops/<disk>.json and get all iops info, get iops from calculon
    If there is no data, simply adds it, if there is data different with
    data loaded from file, compares in addition time stamp from calculon response
    and file modification time, newest value is correct and should be synchronized.
    If there is no data at all, raise Exception, it should be configuration/deployment problem.
    """
    result = None
    probable_path = os.path.join(calculon_config.benchmark_results, "iops/{}.json".format(self.LBS_eligible))
    if not os.path.exists(probable_path):
      self.calculate_iops()
    with open(probable_path) as data_source:
      try:
        result = json.loads(data_source.read())
      except ValueError as error:  # json.loads in python2 raises ValueError
        print("Can't get IOPS, malformed json: {}".format(error))
    if result:
      for key, value in result.iteritems():
        self.__dict__[key] = value
    else:
      print("Trying to read calculon")
    return self

