#!/usr/bin/env python3
import requests
import json
from flask import Flask


def nginx_stub_status():
    r = requests.get('http://127.0.0.1:7777');
    return r.text.split("\n")



app = Flask(__name__)
app.debug = True

@app.route("/")
def main():
    data = {
        'active_connections': 0,
        'conn_count.accepts': 0,
        'conn_count.handled': 0,
        'conn_count.requests': 0,
        'conn_status.reading': 0,
        'conn_status.writing': 0,
        'conn_status.waiting': 0,
    }
    marker1 = False
    for line in nginx_stub_status():
        if 'Active connections:' in line:
            data['active_connections'] = int(line.split('Active connections:')[1])
            continue
        elif 'server accepts handled requests' in line:
            marker1 = True
            continue
        elif marker1:
            marker1 = False
            values = line.split()
            data['conn_count.accepts'] = int(values[0])
            data['conn_count.handled'] = int(values[1])
            data['conn_count.requests'] = int(values[2])
            continue
        elif 'Reading:' in line and 'Writing:' in line and 'Waiting:' in line:
            data['conn_status.reading'] = int(line.split()[1])
            data['conn_status.writing'] = int(line.split()[3])
            data['conn_status.waiting'] = int(line.split()[5])
            continue
    result = {'sensors':[]}
    sensors = result['sensors']
    for key, val in iter(data.items()):
        sensors.append({
            "labels": {
                "sensor": key
            },
            "value": val
        })
    return json.dumps(result, indent=2)


app.run(host='::', port=7888, threaded=True)