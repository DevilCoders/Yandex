#!/usr/bin/env python

import json
import sys
if sys.version_info[0] < 3:
    import httplib
else:
    import http.client as httplib

from string import Template

def get_endpoints(endpoint_set_id, dc, add_port=True):
    data = Template('{"cluster_name": "$dc", "endpoint_set_id": "$endpoint_set_id", "client_name": "mds-admin"}').substitute(dc=dc, endpoint_set_id=endpoint_set_id)
    conn = httplib.HTTPConnection("sd.yandex.net", 8080)
    conn.request("POST", "/resolve_endpoints/json", data)
    r = conn.getresponse()
    res = json.loads(r.read())
    endpoints = []
    for enp in res.get('endpoint_set',{}).get('endpoints',{}):
        endpoint = []
        endpoint.append(enp.get('fqdn',""))
        if add_port:
            endpoint.append(str(enp.get('port',"")))
        endpoints.append(":".join(endpoint))
    endpoints.sort()
    conn.close()
    return endpoints
