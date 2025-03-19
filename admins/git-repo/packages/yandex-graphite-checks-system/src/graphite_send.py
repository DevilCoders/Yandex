#!/usr/bin/env python

import os, sys, socket, re, time, ConfigParser, logging

config = ConfigParser.ConfigParser()

debugPrint = False
if len(sys.argv) > 1 and sys.argv[1] == '-d': debugPrint = True

try:
  config.read(['/etc/graphite-checks.conf', './conf/graphite-checks.conf'])
  checks_dir = config.get('main', 'checks_dir')
  metrics_prefix = config.get('main', 'metrics_prefix')
  port = config.getint('main', 'port')
  logfile = config.get('main', 'log_filename')
except Exception, e:
  print "Error reading config file:"
  print repr(e)
  exit(1)

checks = os.listdir(checks_dir)

try:
  logging.basicConfig(filename=logfile, level=logging.DEBUG, format="[%(asctime)s] %(levelname)s %(message)s")
except Exception, e:
  print "Error creating logfile: " + repr(e)
  exit(1)

logger = logging.getLogger('main')

hostname = socket.gethostname()
try:
  hostname = socket.getfqdn(hostname)
except Exception, e:
  logger.error("Can't get host FQDN: %s" % repr(e))

hostname = hostname.replace(".","_")

if not debugPrint:
  try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('127.0.0.1', port))
  except Exception, e:
    logger.error('Error connecting to graphite client: ' + repr(e))
    exit(0)

for check in checks:
  checkname = re.sub("\.[^\.]+$", "", check)
  try:
    pipe = os.popen(checks_dir + "/" + check)
    timestamp = int(time.time())
  
    for line in pipe.readlines():
      result = "" if metrics_prefix == "" else metrics_prefix + "."
      result += hostname + "." + checkname + "." + line.strip() + " " + str(timestamp) + "\n"
      if debugPrint:
        print result.strip()
      else:
        s.sendall(result)

  except Exception, e:
    logger.error('Error in metric "' + checkname + '": ' + repr(e))
  
if debugPrint: exit(0)

s.close()
