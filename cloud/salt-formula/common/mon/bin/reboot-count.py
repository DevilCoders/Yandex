#!/usr/bin/python
import subprocess
import datetime
try:
    last_output = subprocess.check_output('last').split('\n')
except Exception:
    print("PASSIVE-CHECK:reboot-count;1;Something went wrong")

count_today = 0
count_yes = 0
for string in last_output:
    if 'boot' in string and int(string.split()[6]) == datetime.datetime.today().day:
        count_today += 1
    if 'boot' in string and int(string.split()[6]) == datetime.datetime.today().day - 1:
        count_yes += 1


if count_today + count_yes:
    print("PASSIVE-CHECK:reboot-count;1;Server was rebooted {0} times today, yesterday {1} times".format(str(count_today), str(count_yes)))
else:
    print("PASSIVE-CHECK:reboot-count;0;Ok 0 boots today and yesterday")
