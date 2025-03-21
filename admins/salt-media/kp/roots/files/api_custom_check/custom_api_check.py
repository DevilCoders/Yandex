#!/usr/bin/python2.7

import requests
import socket
import sys
import StringIO
import subprocess
import re

me=socket.gethostname()

cmd="find /etc/ -wholename \"*kino-*-*/app.conf\" -print"

try:
  conf = subprocess.check_output(cmd, shell=True).strip()
except:
  print "1;Failed to find app config directory"
  sys.exit(0)

cmd="grep extra.server.port " + conf + " | awk -F= \'{print $2}\'"

# get app service port
try:
  port = subprocess.check_output(cmd, shell=True)
except:
  print "1;Failed to find app port"
  sys.exit(0)

url = "http://"+me+":"+port+"/monrun/healthcheck/all"
url = url.replace(" ", "").replace("\n", "")

# get status
try:
  status=requests.get(url)
  if status.status_code != 200:
    print "2;API returned "+status.status_code.__str__()
    sys.exit(0)
except:
  print "2;Failed to get application status"
  sys.exit(0)

data=StringIO.StringIO(requests.get(url).content)

if sys.argv.__len__() > 1:
    check = sys.argv[1]
else:
  print "1;unknown check"
  sys.exit(0)

all_st= {}
for st in data:
  if re.match("^[0-9];[\S]+", st):
    t = st.split(";")
    if int(t[0]) > 0:
      # if existing key status lower than current, rewrite status
      if t[1] in all_st.keys():
        if int(all_st[t[1]][0]) < int(t[0]):
          all_st.update({t[1] : [t[0], t[2]]})
      else:
        all_st.update({t[1] : [t[0], t[2]]})

if check in all_st.keys():
  print all_st[check][0]+";"+all_st[check][1]
  sys.exit(0)
else:
  print "0;OK"
