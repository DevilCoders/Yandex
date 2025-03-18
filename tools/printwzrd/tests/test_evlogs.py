#!/usr/bin/env python3
'''
This is the test to test that some event logs are present in the wizard HTTP response
for &debug=eventlogs&format=json.

This is a quick-and-dirty test, which is not good enough for production use as is.
Prerequisites:
* ya make in /web/daemons/wizard
* ya make in /search/wizard/data/wizard

The test starts the wizard as a daemon using relative paths,
it should be run as ./test_evlog.py.
Then it makes a HTTP request to this localhost wizard instance
and validates that there is a section 'eventlog' with non-empty events in the response.
'''

import atexit
import datetime
import json
import os
import subprocess
import sys
import urllib.request
import urllib.parse
from time import sleep

ROOT = '../../..'
WIZARD_BIN = '%s/web/daemons/wizard/wizard' % ROOT
WIZARD_DATA = '%s/search/wizard/data'% ROOT
WIZARD_CFG = '%s/tools/printwzrd/configs/printwzrd.cfg' % ROOT
WIZARD_PORT = 47001

TEST_ROOT = './test-evlog-results'
WIZARD_EVLOG = '%s/evlog' % TEST_ROOT
WIZARD_STDOUT = '%s/wizard.stdout' % TEST_ROOT
WIZARD_STDERR = '%s/wizard.stderr' % TEST_ROOT
JSON_RESPONSE = '%s/response.txt' % TEST_ROOT

TEXT = 'реферат про мікроклімат і його вплив на людину'

TEXT = urllib.parse.quote(TEXT, safe='')

os.makedirs(TEST_ROOT, exist_ok = True)

print('Starting wizard...')
cmd = [WIZARD_BIN, '--evlog', WIZARD_EVLOG,
                   '--data', WIZARD_DATA,
                   '--config', WIZARD_CFG,
                   '--port', '%d' % WIZARD_PORT,
      ]
print(' '.join(cmd))
print('    stdout: %s\n    stderr: %s' % (WIZARD_STDOUT, WIZARD_STDERR))
Wizard = subprocess.Popen(cmd, stdout = open(WIZARD_STDOUT, 'w'), stderr = open(WIZARD_STDERR, 'w'))
def stopWizard():
    print('Stopping the wizard process...', end = ' ')
    sys.stdout.flush()
    try:
        Wizard.wait(10)
        print('ok')
    except:
        Wizard.kill()
        print('the wizard was killed after timeout')

atexit.register(stopWizard)

cgi_request = 'text=%s&debug=eventlogs&format=json' % TEXT

attempt = 0
NATTEMPTS = 600

print ('[%s] Waiting for the wizard (timeout is %d seconds)...' % (datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), NATTEMPTS))
while attempt < NATTEMPTS: # wait 10 minutes
    try:
        request = urllib.request.urlopen("http://127.0.0.1:%s/wizard?%s" % (WIZARD_PORT, cgi_request))
        response = request.read()
        break
    except:
        sleep (1)
        continue # wizard is not ready yet
else:
    print('Error: test failed. The wizard did not respond in %d seconds' % NATTEMPTS)
    sys.exit (-1)

response = response.decode('utf-8')

print('HTTP request done, cgi: %s' % cgi_request)

def perror(x):
    print('Error: test failed. %s' % x)
    print('Examine the response here: %s' % JSON_RESPONSE)

try:
    json_ans = json.loads(response, encoding = 'utf-8')
except Exception:
    with open(JSON_RESPONSE, 'w') as f:
        f.write(response)
    perror('The response is not a valid json.')
    raise

with open(JSON_RESPONSE, 'w') as f:
    json.dump(json_ans, f, ensure_ascii = False, indent = 2)

if json_ans["eventlog"][0]["EventBody"]["Fields"]["Request"] != cgi_request:
    perror('The response contains an invalid cgi request, should be: «%s»' % cgi_request)
    sys.exit(1)

for i,event in enumerate(json_ans["eventlog"]):
    if "EventBody" not in event:
        perror('Event %d has no body!' % i)
        sys.exit(2)
    body = event["EventBody"]
    for field in ("Fields", "Type"):
        if field not in body:
            perror('Body of the event %d has no field «%s»!' % (i, field))
            sys.exit(3)
    if "Timestamp" not in event:
        perror('Event %d has no timestamp!' % i)
        sys.exit(4)

print(Wizard.terminate())
print('Success: all checks passed. You may now want to delete %s.' % TEST_ROOT)

