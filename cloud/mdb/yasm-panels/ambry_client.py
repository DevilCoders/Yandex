import json

import requests
from simplejson import JSONDecodeError


def updatePanel(key, panelDict, baseUrl):
    getRes = requests.get(baseUrl + 'get?key=' + key).json()
    if 'error' in getRes:
        if getRes['error_code'] == 'panel_not_found':
            panelId = key
        else:
            raise Exception(getRes['error'])
    else:
        panelId = getRes['result']['_id']

    postDict = {'keys': {'user': None, 'key': panelId},
                'values': {'value': panelDict,
                           'key': key,
                           'type': 'panel'}}
    postRes = requests.post(baseUrl + 'upsert', data=json.dumps(postDict))
    try:
        if 'error' in postRes.json():
            raise Exception(postRes.json()['error'])
    except JSONDecodeError:
        pass
