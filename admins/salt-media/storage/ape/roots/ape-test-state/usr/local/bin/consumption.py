#!/usr/bin/env python
import requests
import json
warning_percent = float('10.0')
fail_percent = float('30.0')
fail = '2;'
warning = '1;'
curl = "http://localhost:8080/v1/status"
try:
    r = requests.get(curl,timeout=10)
except:
    print "1;problem with /v1/status"
    exit(0)
consumption = json.loads(r.text)['consumption']
if 'clusters' in consumption:
    for cluster in consumption['clusters']:
        if float(consumption['clusters'][cluster]['cpu'].rstrip('%')) > fail_percent:
            fail = fail + ' ' + consumption['clusters'][cluster]['cpu'] + ' cpu used on ' + cluster + ';'
        elif float(consumption['clusters'][cluster]['cpu'].rstrip('%')) > warning_percent:
            warning = warning + ' ' + consumption['clusters'][cluster]['cpu'] + ' cpu used on ' + cluster + ';'
        if float(consumption['clusters'][cluster]['memory'].rstrip('%')) > fail_percent:
            fail = fail + ' ' + consumption['clusters'][cluster]['memory'] + ' memory used on ' + cluster + ';'
        elif float(consumption['clusters'][cluster]['memory'].rstrip('%')) > warning_percent:
            warning = warning + ' ' + consumption['clusters'][cluster]['memory'] + ' memory used on ' + cluster + ';'
if float(consumption['cpu'].rstrip('%')) > fail_percent:
    fail = fail + ' ' + consumption['cpu'] + ' cpu used on all clusters;'
elif float(consumption['cpu'].rstrip('%')) > warning_percent:
    warning = warning + ' ' + consumption['cpu'] + ' cpu used on all clusters;'
if float(consumption['memory'].rstrip('%')) > fail_percent:
    fail = fail + ' ' + consumption['memory'] + ' memory used on all clusters;'
elif float(consumption['memory'].rstrip('%')) > warning_percent:
    warning = warning + ' ' + consumption['memory'] + ' memory used on all clusters;'

if fail == '2;' and warning == '1;':
    print '0;Ok'
elif fail == '2;':
    print warning
else:
    print fail

