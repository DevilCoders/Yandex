#!/usr/bin/env python3

import multiprocessing

from yc_monitoring import JugglerPassiveCheck

LA1_COEF = 1.0
LA15_COEF = 0.8 # more strick with long time load

def read_first_line_from_file(file_to_read: str):
    try:
        with open(file_to_read, 'r') as f:
            return f.readline().strip()
    except OSError as ex:
        raise Exception("Fail to read file '{}'!".format(file_to_read, ex))

def get_load_average():
    LA_FILE = "/proc/loadavg"

    la_data = read_first_line_from_file(LA_FILE)
    if not la_data:
        raise Exception("Unexpected data in '{}' file!".format(LA_FILE))
    la_data = la_data.split()
    if len(la_data) < 3:
        raise Exception("Unexpected data in '{}' file! Expect at least 3 fields. Data:\n{}".format(LA_FILE, la_data))

    return float(la_data[0]), float(la_data[1]), float(la_data[2])

def main():
    check = JugglerPassiveCheck("load_average")
    try:
        la1, la5, la15 = get_load_average()
        ncpus = multiprocessing.cpu_count()

        if la15/ncpus > LA15_COEF:
            check.crit("Host under huge load for a long time! LA15={}, ncpus={}".format(la15, ncpus))
        elif la1 - la5 > ncpus:
            check.crit("Host load increased dramatically! LA1={},LA5={}, ncpus={}".format(la1, la5, ncpus))
        elif la1/ncpus > LA1_COEF:
            check.warn("Host under huge load! LA1={}, ncpus={}".format(la1, ncpus))

    except NotImplementedError as ex:
        check.crit("Can't obtain number of cpus: ({}): {}".format(ex.__class__.__name__, ex))
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == "__main__":
    main()
