import json

import sys


def get_tz_by_string_key(timezone):
    hours = {
        "MSK": 3,
        "UTC": 0
    }[timezone.upper()]

    return hours * 3600


def script(inputs, *_, **__):
    yql_result, labels, timezone = inputs[0], inputs[1], inputs[2]

    with open(yql_result, 'r') as f:
        yql_result = json.loads(f.read())[0]

    with open(labels, 'r') as f:
        labels = json.loads(f.read())

    with open(timezone, 'r') as f:
        timezone = f.read()

    result = {
        "metrics": [{
            "labels": labels,
            "ts": yql_result["now"],
            "value": yql_result["delta"] + get_tz_by_string_key(timezone)
        }]
    }
    sys.stdout.write(json.dumps(result))
