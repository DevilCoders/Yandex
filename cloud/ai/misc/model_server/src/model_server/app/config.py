import argparse
import sys

parser = argparse.ArgumentParser()
parser.add_argument('-p', '--port', dest='rest_api_port', default=80, help='REST API port')
context = parser.parse_args(sys.argv[1:])

MODEL_CLASS_MAP = {
    'badoo': {
        0: 'AdultAndChild',
        1: 'Erotic',
        2: 'Female',
        3: 'Group',
        4: 'Male',
        5: 'Other',
        6: 'Trash',
        7: 'Weapon'
    }
}

rest_api_port = int(context.rest_api_port)
