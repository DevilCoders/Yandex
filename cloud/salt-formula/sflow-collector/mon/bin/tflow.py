#!/usr/bin/env python3

import json
import subprocess

_proc_status = '/dev/null'
_scale = {'kB': 1024.0, 'mB': 1024.0*1024.0,
          'KB': 1024.0, 'MB': 1024.0*1024.0}

def _VmB(VmKey):
  '''Private.
  '''
  global _proc_status, _scale
   # get pseudo file  /proc/<pid>/status
  try:
      t = open(_proc_status)
      v = t.read()
      t.close()
  except:
      return 2, {"reason": "can't read proc status"}
   # get VmKey line e.g. 'VmRSS:  9999  kB\n ...'
  i = v.index(VmKey)
  v = v[i:].split(None, 3)  # whitespace
  if len(v) < 3:
      return 0.0  # invalid format?
   # convert Vm value to bytes
  return float(v[1]) * _scale[v[2]]


def memory(since=0.0):
  '''Return memory usage in bytes.
  '''
  return _VmB('VmSize:') - since


def run_check():
  status = 0
  message = ["OK"]
  command = ["/bin/systemctl", "is-active", "tflow"]
  try:
    process = subprocess.Popen(command, stdout=subprocess.PIPE)
    # print(process.communicate())
    data =  process.communicate()[0].decode().strip()
    # print(data)
    if(data == 'inactive'):
      return 2, {"reason": "service inactive"}
  except Exception as e:
    return 2, {"reason": "Can't check whether tflow is active: {}".format(e)}

  command = ["/usr/bin/pgrep", "tflow"]
  try:
    process = subprocess.Popen(command, stdout=subprocess.PIPE)
    pid =  process.communicate()[0].decode().strip()
    if(pid == ''):
      return 2, {"reason": "Can't find tflow PID"}
    else:
      global _proc_status
      _proc_status = "/proc/{}/status".format(pid)
      mem = memory()
      if(mem>1000000000):
        status = 1
        message.append("mem: {}".format(mem))
      else:
        message.append("mem: {}".format(mem))

  except Exception as e:
    return 2, {"reason": "Can't get tflow process information: {}".format(e)}

  return status, message


def main():
    status, message = run_check()
    print("{};{}".format(status, message))


if __name__ == "__main__":
    main()
