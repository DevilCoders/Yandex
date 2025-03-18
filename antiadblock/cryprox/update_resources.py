#!/usr/bin/python -u
import os
import urllib2
import json
import subprocess


HOST_SKYNET_PATH = '/Berkanavt/supervisor/base/active'
SKY_BIN_PATH = '/skynet/tools/sky'

CONTAINER_SKYNET_PATH = '/skynet'
CONTAINER_SKY_PATH = '/usr/local/bin/sky'

BYPASS_UIDS_BASE_PATH = os.getenv('BYPASS_UIDS_FILE_PATH', '/tmp_uids')
CURRENT_RESOURCES_PATH = BYPASS_UIDS_BASE_PATH
CURRENT_RESOURCES_STATE_PATH = os.path.join(BYPASS_UIDS_BASE_PATH, 'bypass_uids_state')
NEW_RESOURCES_PATH = os.path.join(BYPASS_UIDS_BASE_PATH, 'tmp')

# configure host sky
# https://wiki.yandex-team.ru/skynet/drafts/edinyjj-xostovyjj-skynet/
if not os.path.exists(CONTAINER_SKYNET_PATH):
    os.symlink(HOST_SKYNET_PATH, CONTAINER_SKYNET_PATH)
if not os.path.exists(CONTAINER_SKY_PATH):
    os.symlink(SKY_BIN_PATH, CONTAINER_SKY_PATH)


def update_resource(resource_type, skynet_id):
    old = os.path.join(CURRENT_RESOURCES_PATH, resource_type + '.old')
    if os.path.exists(old):
        os.remove(old)
    subprocess.call([CONTAINER_SKY_PATH, 'get', '-t 300',  '-d', NEW_RESOURCES_PATH, skynet_id])
    new = os.path.join(NEW_RESOURCES_PATH, resource_type)
    cur = os.path.join(CURRENT_RESOURCES_PATH, resource_type)
    old = os.path.join(CURRENT_RESOURCES_PATH, resource_type + '.old')
    if os.path.exists(cur):
        os.rename(cur, old)
    os.rename(new, cur)


resource_types = ['ANTIADBLOCK_BYPASS_UIDS_DESKTOP', 'ANTIADBLOCK_BYPASS_UIDS_MOBILE', 'ANTIADBLOCK_CRYPROX_INTERNAL_EXPERIMENT_CONFIG']
sandbox_api = 'https://sandbox.yandex-team.ru/api/v1.0/resource?limit=1&type={type}&state=READY&owner=ANTIADBLOCK'

current_resources_state = dict()
new_resources_state = dict()
state_changed = False
if os.path.exists(CURRENT_RESOURCES_STATE_PATH):
    with open(CURRENT_RESOURCES_STATE_PATH, 'r') as f:
        try:
            current_resources_state = json.load(f)
        except Exception:
            pass  # in case someone touched content of the file

for rt in resource_types:
    response = urllib2.urlopen(sandbox_api.format(type=rt))
    resource_description = json.loads(response.read())
    rid = resource_description['items'][0]['id']
    skynet_id = resource_description['items'][0]['skynet_id']
    new_resources_state[rt] = rid
    if current_resources_state.get(rt) != rid:
        update_resource(rt, skynet_id)
        state_changed = True

if state_changed:
    with open(CURRENT_RESOURCES_STATE_PATH, 'w') as f:
        json.dump(new_resources_state, f)
