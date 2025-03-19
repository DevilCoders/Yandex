#!/usr/bin/env python

import requests
import subprocess
import json
import logging
import os

from time import sleep

LOG_PATH = "/var/log/walle"
CMS_URL = "http://walle-cms.mds.yandex.net/host/"
LOG_LEVEL = logging.INFO 

class Command(object):
    @staticmethod
    def down():
        command = ["iptruler","all","down"]
        logging.debug('Executing "' + " ".join(command) + '"')
        subprocess.check_call(command)
        sleep(60)

    @staticmethod
    def up():
        command = ["iptruler","all", "up"]
        logging.debug('Executing "' + " ".join(command) + '"')
        subprocess.check_call(command)

    @staticmethod
    def reboot():
        Command.down()

    @staticmethod
    def redeploy():
        Command.down()

    def __getattr__(self, name):
        logging.debug(name + " not found in class Command")
        def success():
            logging.debug("default = success")
            return True
        return success 

command = Command()


def process(action):
    try:
        logging.debug("Doing " + action)
        getattr(command,action)()
        return True
    except AttributeError as e:
        logging.error("Action {} failed".format(action))
        return False

def delete(job):
    r = requests.delete(CMS_URL+'delete/'+job)
    logging.debug(r.text)


def fail(job):
    r = requests.get(CMS_URL+'fail/'+job)
    logging.debug(r.text)


def main():
    try:
        os.makedirs(LOG_PATH)
    except OSError:
        if not os.path.isdir(LOG_PATH):
            raise

    logging.basicConfig(level = LOG_LEVEL, filename=LOG_PATH + '/cms_agent.log', format='%(asctime)s %(process)d %(levelname)s: %(message)s')
    hostname = os.uname()[1]
    logging.debug("Hostname: " + hostname)
    r = requests.get(CMS_URL+hostname)
    try:
        data = json.loads(r.text)
        logging.debug("Response from /host/: " + r.text)
        if "action" in data.keys():
            action = data["action"]
            job_id = data["id"]
            logging.info("Action: " + action)
            result = process(action)
            if result:
                logging.info("Successfully done job {}, action {}".format(job_id, action))
                delete(job_id)
            else:
                logging.error("Failed job {}, action {}".format(job_id, action))
                fail(job_id)
        else:
            logging.debug("Nothing to do")
            exit(0)
    except ValueError as e:
        logging.error("Unable to parse response: " + r.text)
        exit(1)
    exit(0)

if __name__ == "__main__":
    main()
