#!/usr/bin/env python3

import json
import subprocess


def run_check():
  status = 0
  message = "ok"
  command = ["/usr/bin/gobgp", "n", "--json"]
  try:
    process = subprocess.Popen(command, stdout=subprocess.PIPE)
    # print(process.communicate()[0])
    data = json.loads(process.communicate()[0].decode())
    # print(data)
    addr = data[0]['state']['neighbor-address']
    state = data[0]['state']['session-state']
    if((addr != '10.255.254.1') or (state != 'established')):
      status = 2
      message = {"reason": "neighbor: {}, state: {}".format(addr, state)}

    return status, message
  except Exception as e:
    return 1, {"reason": "Can't get status from gobgp process: {}".format(e)}


def main():
    status, message = run_check()
    print("{};{}".format(status, message))


if __name__ == "__main__":
    main()
