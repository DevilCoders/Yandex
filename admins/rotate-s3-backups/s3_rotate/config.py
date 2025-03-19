"""
Config loader
"""

import os
import logging
import logging.handlers
import argparse

import yaml
from dotmap import DotMap

def load_config(args):
    """Configure logging and load rotate configs"""
    confdir = '/etc/rotate-s3-backups'
    name = "main.yaml"
    if not os.path.exists(confdir):
        if os.path.exists("./conf"):
            confdir = "./conf"
            name = "main-default.yaml"
    with open(os.path.join(confdir, name)) as main_conf:
        conf = DotMap(yaml.load(main_conf))

    conf = set_defaults_in_config(args, conf)
    conf.args = args

    fmt = "%(asctime)s %(levelname)-6s %(funcName)14s: %(message)s"
    handler = logging.handlers.WatchedFileHandler(conf.logfile)
    handler.setFormatter(logging.Formatter(fmt))
    logging.getLogger('kazoo.client').setLevel(logging.INFO)

    log = logging.getLogger()
    log.setLevel(logging.DEBUG)
    log.addHandler(handler)
    log = logging.getLogger(__name__)

    confdir = os.path.join(confdir, "buckets")
    for bucket_yaml in os.listdir(confdir):
        bucket_conf_file = os.path.join(confdir, bucket_yaml)
        if os.path.isfile(bucket_conf_file):
            log.info("Load config %s", bucket_conf_file)
            bucket = bucket_yaml.rsplit(".", 1)[0]
            try:
                with open(bucket_conf_file) as bucket_conf:
                    conf.buckets[bucket] = DotMap(yaml.load(bucket_conf))
            except:  # pylint: disable=bare-except
                log.exception("Failed to load %s", bucket_conf_file)
                os.sys.exit(1)

    # dont move to set_defaults_in_config
    if not conf.buckets[conf.current_bucket].s3cfg:
        conf.buckets[conf.current_bucket].s3cfg = os.path.expanduser("~/.s3cfg")

    return conf

def set_defaults_in_config(args, conf):
    """Fix config"""

    conf.logfile = conf.logfile or '/var/log/rotate-s3-backups.log'

    conf.current_bucket = args.bucket
    conf.debug = args.debug

    conf.zk.group = conf.zk.group or "media-stable-zk"
    conf.zk.basepath = conf.zk.basepath or "/media/rotate-s3-backups"
    conf.zk.safe_period = conf.zk.safe_period or (2 * 60 * 60) # 2 h
    conf.zk.timeout = conf.zk.timeout or 300 # seconds

    if not conf.last_ts_name:
        conf.last_ts_name = "last_timestamp"
        conf.last_ts_path = os.path.join(
            conf.zk.basepath, args.bucket, conf.last_ts_name
        )

    conf.monitoring.rotate = conf.monitoring.rotate or 3 # save last 3 backups stats
    conf.monitoring.daily_only = conf.monitoring.daily_only or 0

    if args.check:
        if not conf.monitoring.logfile:
            path_arr = conf.logfile.split("/")
            name_arr = path_arr[-1].rsplit(".", 1)
            name_arr[-1] = name_arr[-1] or "log"

            while len(name_arr) < 2:
                name_arr.insert(0, "log")
            path_arr[-1] = "{}-monitoring.{}".format(*name_arr)
            conf.monitoring.logfile = "/".join(path_arr) # don't use os.path.join here
        # while check backups log to another file
        conf.logfile = conf.monitoring.logfile

        conf.check_for_database = args.check
        # check inteval 600 sec, relevant check period 500 sec
        conf.zk.safe_period = 500
        conf.last_ts_path = "{}-monitoring-{}".format(conf.last_ts_path, args.check)
        conf.zk_last_status = os.path.join(
            conf.zk.basepath, args.bucket, "last_status-{}".format(args.check)
        )

    return conf

def parse_args():
    """Parse command arguments"""
    parser = argparse.ArgumentParser(description='Rotate s3 backups')
    parser.add_argument(
        '--check', metavar='DATABASE', help="Run backup check instead rotation"
    )
    parser.add_argument(
        '-b', '--bucket', required=True, help="Run operation for specified bucket"
    )
    parser.add_argument('--debug', action='store_true', help="Print debug messages")
    parser.add_argument('--dry-run', action='store_true', help="Dry run, don't do any action")
    return parser.parse_args()
