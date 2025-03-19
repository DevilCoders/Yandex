#!/usr/bin/python
import sys
import re
import yaml
import argparse
import os
# Variables
## Argument Parser
CHECKS_DIR = "/etc/nginx-kp-graphite-confs"
CHECKS = os.listdir(CHECKS_DIR)

# Functions
def read_config(cfile):
        with open(cfile,'r') as f:
                config = yaml.load(f)
        return config

def read_log(log):
        with open(log,'r') as log_file:
                return log_file.readlines()

def operate_log(lines,line_regexp):
        for line in lines:
                result = line_regexp.search(line)
                for host in HOSTS.keys():
                        if host == "all" or any(result.group('HTTP_HOST') in item for item in HOSTS[host]["hosts"]):
                                try:
                                        HOSTS[host]["rpm"][result.group('STATUS')] = HOSTS[host]["rpm"].setdefault(result.group('STATUS'), 0) + 1
                                except:
                                        pass

def convert_rpm_to_rps(time_delta):
        for host in HOSTS.keys():
                for code in HOSTS[host]["rpm"].keys():
                        HOSTS[host]["rps"][code] = float(HOSTS[host]["rpm"][code]) / time_delta

def printer(host):
        rpx = HOSTS[host]["type"]
        for code in HOSTS[host][rpx].keys():
                print("%s.%s %s" % (host,code,HOSTS[host][rpx][code]))

# Fire!
if __name__ == "__main__":
        # Read config
        for check in CHECKS:
                check_path = "/".join([CHECKS_DIR,check])
                config = read_config(check_path)
                # Bind variables
                HOSTS = config["HOSTS"]
                TIME_DELTA = config["TIME_DELTA"]
                LOG = config["LOG"]
                LINE_REGEXP = re.compile(config["LINE_REGEXP"])

                # Create counters
                for host in HOSTS.keys():
                        HOSTS[host]["rpm"] = {}
                        HOSTS[host]["rps"] = {}

                # Make operations
                operate_log(read_log(LOG),LINE_REGEXP)
                convert_rpm_to_rps(TIME_DELTA)

                # Print result
                for host in HOSTS.keys():
                        printer(host)
