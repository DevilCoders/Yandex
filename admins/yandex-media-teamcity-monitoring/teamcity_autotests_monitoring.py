#!/usr/bin/env python3
"""
A flask app that recieves a json reponse from teamcity autotests.
It lights up golem triger if some tests have failed
"""

import json
import logging
from logging.handlers import RotatingFileHandler
import socket
import requests
from flask import Flask, request

APP = Flask(__name__)
FAIL_THRESHOLD = 4 # number of failed tests
JUGGLER_API = "http://juggler-push.search.yandex.net/api/1/batch"
LOG_FILE = '/var/log/yandex/teamcity-autotests-monitoring/log.log'
LOG250MB = 1024 * 1025 * 250

HANDLER = RotatingFileHandler(LOG_FILE, maxBytes=LOG250MB, backupCount=1)
HANDLER.setLevel(logging.INFO)
FORMATTER = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
HANDLER.setFormatter(FORMATTER)
LOGGER = logging.getLogger('app')
LOGGER.setLevel(logging.DEBUG)
LOGGER.addHandler(HANDLER)

def get_failed(build_status):
    """
    Get failed test percent
        :param build_status: build status string sent by teamcity, str
<<<<<<< HEAD
        :return: percent of failed test, str
=======
        :retrun: number of failed test, str
>>>>>>> change_threashold
    """

    LOGGER.info("Build_status %s", build_status)
    build_status = build_status.split(';')
    build_status = build_status[0].split(',')
    failed = build_status[0]
    try:
        index = failed.index("(")
        failures = failed[:index].split()[-1]
    # sometime teamcity sends string without "("
    except: # pylint: disable=broad-except, bare-except
        failures = failed.split()[-1]
    return failures

def send_juggler_event(description, status):
    """
    Create juggler check
        :param description: juggler check description for golem, str
        :param status: juggler status. OR or CRIT, str
    """

    juggler_check = [{
        "host": socket.getfqdn(),
        "service": "music-teamcity-autotests",
        "description": description,
        "status": status
    }]
    LOGGER.info("Juggler msg: %s", juggler_check)
    try:
        res = requests.post(JUGGLER_API,
                            json=juggler_check)
        LOGGER.info("Juggler response %s", res.text)

    except Exception as ex: # pylint: disable=broad-except
        LOGGER.info("Juggler Excepton %s", ex)
        raise ex


def url_shortener(url):
    """
    Shorten a url
        :param url: a url of a teamcity task, str
        :return: a short url or full url on error, str
    """

    try:
        nda = 'https://nda.ya.ru/--?url='
        res = requests.get(nda + url, timeout=0.5)
        short_url = res.text
    except Exception as ex: # pylint: disable=broad-except
        # Return full url if nda.ya.ru failed
        LOGGER.error("%s raise an error %s. Using full lenth ur", nda, ex)
        short_url = url
    return short_url

@APP.route('/', methods=['POST'])
def check():
    """
    Main check function
    """
    msg = "Ok"
    level = "Ok"
    code = 200
    data = json.loads(request.data.decode('utf-8'))
    if data['build']['buildResult'] == "failure":
    # send event to juggler
        failed = get_failed(data['build']["buildStatus"])
        if failed > FAIL_THRESHOLD:
            msg = "Number of failed tests {} > {}  ".format(failed,
                                                            FAIL_THRESHOLD)
        url = url_shortener(data['build']['buildStatusUrl'])
        msg += "Link: {}".format(url)
        level = "CRIT"

    try:
        send_juggler_event(msg, level)
    except Exception as ex: # pylint: disable=broad-except
        LOGGER.error(ex)
        code = 500
    return "", code

application = APP # pylint: disable=invalid-name
