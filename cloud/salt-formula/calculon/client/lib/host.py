#!/usr/bin/env python -o
import json
import requests
import socket
import hashlib
import os
import calculon_config
import subprocess as sp


# def read_data_from_calculon(hostname):
#   url = "http://{}/api/v1/ycu/hosts/{}".format(calculon_config.public_endpoint, hostname)
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


class Host(object):
  """Load data about from calculon if it exists and decide if it could be added
  to the cloud based on statistic rejection method
  Table structure:
    Hostname String - str, socket.getfqdn()
    HWID String - str, hardware model hash
    YCU String - str, Yandex Compute Unit calculated on physical core
    VCPU_YCU String - str, Yandex Compute Unit calculated on virtual core
    Rancid String DEFAULT '' - str, startrek task on failed predeploy
  """
  def __init__(self, fqdn):
    self.Hostname = fqdn
    self.YCU = '0.0'
    self.VCPU_YCU = '0.0'
    self.HWID = ''
    self.Rancid = ''

  def calculate_ycu(self):
    if os.path.exists(os.path.abspath(calculon_config.ycu_bench_path)):
      os.chdir(calculon_config.ycu_bench_path)
    ycu_cmdline = './calculate'
    vcpu_cmdline = './calculate-vcpu'
    ycu = None
    vcpu = None
    try:
      ycu = sp.Popen(ycu_cmdline.split(), stdout=sp.PIPE, stderr=sp.PIPE).communicate()
    except sp.CalledProcessError as error:
      print(error)
    try:
      vcpu = sp.Popen(vcpu_cmdline.split(), stdout=sp.PIPE, stderr=sp.PIPE).communicate()
    except sp.CalledProcessError as error:
      print(error)
    if ycu:
      if not ycu[1]:
        self.YCU = ycu[0].replace('YCU', '').strip()
    if vcpu:
      if not vcpu[1]:
        self.VCPU_YCU = vcpu[0].replace('YCU', '').strip()

    dump_path = os.path.join(calculon_config.benchmark_results, 'cpu.json')
    with open(dump_path, 'w') as dump:
      dump.write(json.dumps({'YCU': self.YCU, 'VCPU_YCU': self.VCPU_YCU}))
    return self

  def get_compute_unit(self):
    """
    Load ycu from /opt/benchmarks/results/cpu.json, if there is neither file nor data in calculon raises Exception
    If data in calculon newer, dumps ycu to file, if values in calculon outdated,
    simply update it and returns valid structure.
    :returns: dict with YCU, VCPU_YCU values
    """
    last_local_changes = None
    last_calculon_changes = None
    local_data = None
    ycu_dump = os.path.join(calculon_config.benchmark_results, 'cpu.json')
    if os.path.exists(ycu_dump):
      last_local_changes = int(os.path.getmtime(ycu_dump))
    else:
      self.calculate_ycu()
    with open(ycu_dump) as dump:
      try:
        local_data = json.loads(dump.read())
      except ValueError as error:  # json.loads in python2 raises ValueError
        print("Can't load values from json: {}".format(error))

    calculon_data = None
    # try:
    #   calculon_data = read_data_from_calculon(self.Hostname)
    # except requests.ConnectionError as error:
    #   print "can't get data from calculon: {}".format(error)
    # if calculon_data:
    #   last_calculon_changes = int(calculon_data['Time'])
    #
    # if calculon_data and local_data:
    #   if last_calculon_changes > last_local_changes:
    #     local_data.update(calculon_data)
    #     with open(ycu_dump, 'w') as dump:
    #       dump.write(json.dumps(local_data))
    #     self.YCU = local_data['YCU']
    #     self.VCPU_YCU = local_data['VCPU_YCU']
    #
    #   elif last_calculon_changes <= last_local_changes:
    #     calculon_data.update(local_data)
    #     self.YCU = calculon_data['YCU']
    #     self.VCPU_YCU = calculon_data['VCPU_YCU']

    if local_data and not calculon_data:
      self.YCU = local_data['YCU']
      self.VCPU_YCU = local_data['VCPU_YCU']

    elif calculon_data and not local_data:
      self.YCU = calculon_data['YCU']  # pylint: disable=E1136
      self.VCPU_YCU = calculon_data['VCPU_YCU']  # pylint: disable=E1136

    return self

  def get_hwid(self):
    """Calculate hwid using hash of platform defining factors, such as
    CPU Model
    CPU frequency
    Board Name
    Should be reproducible on same hardware config
    """
    with open('/sys/class/dmi/id/board_name') as board_name:
      motherboard = board_name.read().strip()
    with open('/proc/cpuinfo') as cpu_info:
      data = cpu_info.readlines()
      cpu_name = ''.join(set(el.split(':')[1].strip() for el in data if 'model name' in el))
      frequency = cpu_name.split()[-1]
    self.HWID = hashlib.md5("{}.{}.{}".format(motherboard, cpu_name, frequency)).hexdigest()
    return self


if __name__ == '__main__':
  h = Host(socket.getfqdn())
  print(h.__dict__)

  h.get_hwid().get_compute_unit()
  print(h.__dict__)
