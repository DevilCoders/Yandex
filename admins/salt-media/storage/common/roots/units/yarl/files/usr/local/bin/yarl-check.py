import json
import time
import sys

ts = time.time()
data = sys.stdin.readlines()
if len(data) == 0:
    print "2; zero output"
    sys.exit(2)
else:
     try:
         timestamp_counters = json.loads(data[0])["self"]["lastSyncCountersNs"]/1000000000
         timestamp_qoutas = json.loads(data[0])["self"]["lastSyncQuotasNs"]/1000000000
         if (ts - timestamp_counters) > 60 or (ts - timestamp_qoutas) > 60:
             print "2;sync last then 60s"
             sys.exit(2)
     except Exception:
         print "2;can not parse json"
         sys.exit(2)
print "0;Ok"

