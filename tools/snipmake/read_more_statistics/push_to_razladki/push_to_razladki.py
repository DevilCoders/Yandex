import argparse
import subprocess
import datetime
import time
import urllib2
from datetime import timedelta, date

import yt.wrapper as yt

PATH = "//user_sessions/pub/sample_by_uid_1p/search/daily/{}-{}-{}/clean"

def daterange(start_date, end_date):
    for n in range(int ((end_date - start_date).days)):
        yield start_date + timedelta(n)

def make_point(args, datestring):
    Y = datestring[:4]
    M = datestring[4:6]
    D = datestring[6:]
    check_table = PATH.format(Y, M, D)
    while not yt.exists(check_table):
        time.sleep(1000)

    errors = []
    for i in range(10):
        try:
            result = subprocess.check_output([args.path, "-m", datestring, "-M", datestring, "--fast"])
            break
        except Exception as e:
            errors.append(e.message)
    else:
        raise Exception("Errors:\n" + "\n".join(errors))

    errors = []
    for line in result.rstrip().split("\n"):
        date, name, value = line.split("\t")
        print date, name, value
        ts = time.mktime(datetime.datetime.strptime(date, "%Y%m%d").timetuple())
        for i in range(10):
            try:
                urllib2.urlopen("http://launcher.razladki.yandex-team.ru/save_new_data/snippets_read_more?{}={}&ts={}".format(name, value, str(ts)))
                break
            except Exception as e:
                errors.append(e.message)
        else:
            raise Exception("Errors:\n" + "\n".join(errors))

def parse_cfg():
    parser = argparse.ArgumentParser(
        description=__doc__,
    )
    parser.add_argument(
        "-t", "--timestamp",
        type=float,
        help="float timestamp",
    )
    parser.add_argument(
        "-p", "--path",
        help="read more statistics calculator path",
    )
    parser.add_argument(
        "-d", "--date",
        help="format: YYYYMMDD",
    )
    parser.add_argument(
        "-sd", "--startdate",
        help="format: YYYYMMDD",
    )
    parser.add_argument(
        "-ed", "--enddate",
        help="format: YYYYMMDD",
    )
    return parser.parse_args()

def main():
    args = parse_cfg()
    if args.startdate and args.enddate:
        startdate = datetime.datetime.strptime(args.startdate, "%Y%m%d")
        enddate = datetime.datetime.strptime(args.enddate, "%Y%m%d")
        for date in daterange(startdate, enddate):
            make_point(args, date.strftime("%Y%m%d"))
    else:
        if args.timestamp:
            datestring = datetime.datetime.fromtimestamp(args.timestamp * 0.001).strftime("%Y%m%d")
        if args.date:
            datestring = args.date
        make_point(args, datestring)


if __name__ == "__main__":
    main()

