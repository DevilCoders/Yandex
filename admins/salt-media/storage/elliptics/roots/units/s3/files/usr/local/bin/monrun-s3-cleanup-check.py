#!/usr/bin/env python3

import json
import subprocess as sp
import argparse


def get_log_path(config_path='/etc/s3-goose/cleanup.json'):
    #with open(config_path) as f:
    #    config = json.load(f)
    #return config['logging']['default']['file_path'].split('//')[1]
    return "/var/log/s3/goose-cleanup/scheduler.log"


def count_deleted(path, time_ago):
    cmd = 'timetail -t gofetcher -n {} {}'.format(time_ago, path) + ''' | grep -Po 'creating cleanup target entity' | awk '{s+=1} END {print s}' '''
    return int(sp.check_output(cmd, shell=True).strip() or 0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-n', '--time-ago', type=int, default=1800)
    parser.add_argument('-c', '--crit', type=int, default=1000)
    parser.add_argument('-w', '--warn', type=int, default=0)
    args = parser.parse_args()
    try:
        path = get_log_path()
        deleted = count_deleted(path, time_ago=args.time_ago)
        if deleted <= args.crit:
            status = 2
        elif deleted <= args.warn:
            status = 1
        else:
            status = 0
        msg = '{};deleted {} in last {} seconds'.format(status, deleted, args.time_ago)
    except Exception as e:
        msg = '2;Check failed to run: {}'.format(e.message)
        raise

    print(msg)
