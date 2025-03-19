import subprocess as sp
import logging

log = logging.getLogger(__name__)


def _get_rota():
  """Run lsblk and check if there are ssd or/and hdd drives
  :return info: dict, {'ssd': False, 'hdd': False}
  """
  info = {'ssd': False, 'hdd': False}
  cmdline = '/bin/lsblk -o ROTA'
  data = None
  try:
    data = sp.Popen(cmdline.split(),stdout=sp.PIPE).stdout.readlines()
  except RuntimeError as error:
    log.warning("Can't run lsblk because of: {}".format(error))
  if data:
    info['ssd'] = True if set(el for el in data if (el.strip().isdigit() and int(el) != 1)) else False
    info['hdd'] = True if set(el for el in data if (el.strip().isdigit() and int(el) != 0)) else False
  return info

def main():
  grains = {'pools': {}}
  try:
    grains['pools'] = _get_rota()
  except RuntimeError as error:
    log.error("Can't get data about disks: {}".format(error))
  return grains

if __name__ == '__main__':
  logging.basicConfig(level=logging.DEBUG)
  main()

