# coding: utf-8
"""
uwsgi entry point
"""
import argparse
from cloud.mdb.dbm.internal.run import APP


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', type=int, default=5000)
    args = parser.parse_args()

    APP.run(port=args.port, debug=True)


if __name__ == '__main__':
    main()
