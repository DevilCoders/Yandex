""" Check Mongo Replica Set lag and close local host from world
    when lag is big enough """
import logging
import argparse
import re
import os
from iptables_except_my_group.utils import setup_logger, run_cmd,\
    operate_iptables, M_CRIT, M_OK, M_WARN, monrun


MONGO_CHECK_RS = "/usr/bin/mongodb-check-rs"
MONGO_PORT = 27018
LOG = "/var/log/yandex/react-mongo-lag.log"
STOP_FLAG = "/var/tmp/react_mongo_lag.stop"
logger = logging.getLogger(__name__)


def setup_argparser():
    """ Configure argument parser and get cmd args """
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--sudo', action='store_true', default=False,
                        help='Use sudo for checking the lag')
    parser.add_argument('-d', '--debug', action='store_true', default=False,
                        help='Set log level to DEBUG')
    parser.add_argument('-min', '--min-lag', action='store', default=120,
                        type=int, help='Lag value to trigger WARNING')
    parser.add_argument('-max', '--max-lag', action='store', default=300,
                        type=int, help='Lag value to trigger CRITICAL')
    parser.add_argument('--stop', action='store_true', default=False,
                        help='Create stop flag ({}), do not do anything'.format(STOP_FLAG))
    parser.add_argument('-p', '--perm', action='store',
                        choices=['open', 'close'], default=False,
                        help='Open or close replica and set STOP flag')
    return parser.parse_args()


def get_lag_value(max_lag, use_sudo=False):
    """ Parse MONGO_CHECK_RS output, ignore status, just get lag value """
    if use_sudo:
        cmd = ["sudo", MONGO_CHECK_RS]
    else:
        cmd = [MONGO_CHECK_RS]

    logging.debug('Getting lag value with command: %s', cmd)
    check_rs_lag_output = run_cmd(cmd, ["mongodb", str(max_lag)]).rstrip()
    (code, status, message) = check_rs_lag_output.split(";", maxsplit=2)
    logging.debug("Code: %s. Status: %s. Message: %s.", code, status, message)

    # No replica set treat as zero lag
    if message == "Replica set is not configured":
        return 0
    # Ignore code and status, just extract "[Ll]ag=\d+ from message"
    lag_value_re = re.search(r"[Ll]ag=(\-?\d+)", message)
    logging.debug('Parsed result: %s', lag_value_re)

    try:
        lag_value = int(lag_value_re.group(1))
    except AttributeError:
        raise RuntimeError('Cannot parse lag value from output {}'.format(
            check_rs_lag_output))
    return lag_value


def operate_lag(lag_value, high_thresh, low_thresh):
    """ Do iptables actions based on lag value """
    if lag_value > high_thresh:
        operate_iptables('close', MONGO_PORT, remove_backup_suffix=True)
        monrun(M_CRIT, 'Lag (%s) too high (%s), closed %s port' % (
            lag_value, high_thresh, MONGO_PORT))
    elif lag_value > low_thresh:
        monrun(M_WARN, 'mongodb is lagging (%s), be careful' % lag_value)
    elif lag_value < low_thresh:
        operate_iptables('open', MONGO_PORT, remove_backup_suffix=True)
        monrun(M_OK, 'Lag is ok (%s<%s)' % (lag_value, low_thresh))
    monrun(M_CRIT, 'Unexpected result: current lag %s, '
                   'high_thresh: %s, low_thresh: %s' % (lag_value, high_thresh,
                                                        low_thresh))


def manage_stop_flag(stop=False, perm=False):
    """
    If perm != False (may be open/close) - open/close replica
    If stop|perm == True - create stop flag, else - just check if present
    If stop flag present - do not do anything, just CRIT to monitoring
    """
    if perm:
        operate_iptables(perm, MONGO_PORT, remove_backup_suffix=True)
        logging.debug('%s action processed', perm)
    if stop or perm:
        with open(STOP_FLAG, 'a'):
            os.utime(STOP_FLAG, None)
    if os.path.isfile(STOP_FLAG):
        monrun(M_CRIT, 'Stop flag present: {}. Skipping execution'.format(STOP_FLAG))


def main():
    """
    Close mongo port with iptables if Replica lag greater than high_thresh
    and open when Replica lag lower than low_thresh
    """
    args = setup_argparser()
    setup_logger(default_debug=args.debug)
    lag_value = None

    manage_stop_flag(args.stop, args.perm)
    try:
        lag_value = get_lag_value(args.max_lag)
    except RuntimeError as error:
        monrun(M_CRIT, error)
    except Exception as error:
        monrun(M_CRIT, 'Unexpected error while getting lag value: %s' % error)

    try:
        operate_lag(lag_value, args.max_lag, args.min_lag)
    except RuntimeError as error:
        monrun(M_CRIT, error)
    except Exception as error:
        monrun(M_CRIT, 'Unexpected error while operating lag: %s' % error)


if __name__ == "__main__":
    main()
