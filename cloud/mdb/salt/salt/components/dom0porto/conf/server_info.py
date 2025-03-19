import json

def run():
    include = ['points_info', ]
    grains = {}
    for name, value in __grains__.items():
        if name in include:
            grains.update({name: value})
    return str(json.dumps(grains, indent=4))
