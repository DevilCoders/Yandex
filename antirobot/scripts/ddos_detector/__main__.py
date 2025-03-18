import logging
from time import time
import requests
import os
import socket
import logging.handlers as handlers
import certifi
from infra.yasm.yasmapi import RtGolovanRequest

logging.basicConfig()
host = 'ASEARCH'
CAPTCHA_DURATION = 300
CTYPES = ['stable', 'prestable']
ITS_RUCHKAS = ['antirobot_cbb_panic_mode_for_', 'antirobot_suspicious_ban_for_']
LOG_FILE = "./logs/ddos_detector.log"


class ServiceConfig(object):
    def __init__(self, service, limit):
        self.service : str = service
        self.signal : str = 'unistat_daemon-requests_deee'
        self.limit : int = limit
        self.etag : dict = {}
        self.captcha_till : float = time()

        for ctype in CTYPES:
            for ruchka in ITS_RUCHKAS:
                self.etag[ruchka + ctype] = ''

    @property
    def tag(self):
        return f'itype=antirobot;ctype=prod,prestable;service_type={self.service}'


config = [
    ServiceConfig('market', 5000),
    ServiceConfig('marketblue', 1000),
    ServiceConfig('captcha_gen', 80),
]


def loadNannyToken():
    with open('secrets/NANNY_OAUTH_TOKEN', 'r') as f:
        global NANNY_TOKEN
        NANNY_TOKEN = f.read().strip()


def checkLogPath():
    LOG_PATH = os.path.dirname(LOG_FILE)
    if not os.path.isdir(LOG_PATH):
        os.makedirs(LOG_PATH)


def initLogger():
    checkLogPath()
    fmt = '[%(asctime)s] [%(levelname)s] [{}] [{}] %(message)s'.format(
        os.getpid(), socket.getfqdn())
    datefmt = '%Y-%m-%dT%H:%M:%S'
    standartFormat = logging.Formatter(fmt=fmt, datefmt=datefmt)
    fileLog = handlers.RotatingFileHandler(LOG_FILE, maxBytes=512 * 1024 * 1024, backupCount=10)
    fileLog.setFormatter(standartFormat)
    logging.getLogger().addHandler(fileLog)
    logging.addLevelName(logging.INFO, 'info')
    logging.addLevelName(logging.DEBUG, 'debug')


def enable_its(service, ctype, ruchka):
    its_url = f"https://its.yandex-team.ru/v1/values/antirobot/antirobot/{service}/{ctype}/{ruchka}{service}/"
    etag = ''
    headers = {'Authorization': 'Oauth {}'.format(NANNY_TOKEN), 'Content-Type': 'text/plain'}
    response = requests.post(its_url, timeout=10, verify=certifi.where(), headers=headers, json={'value': service})
    if 'ETag' in response.headers:
        etag = response.headers['ETag']
    return etag, response.content, response.status_code


def disable_its(service, etag, ctype, ruchka):
    its_url = f"https://its.yandex-team.ru/v1/values/antirobot/antirobot/{service}/{ctype}/{ruchka}{service}/"
    headers = {'Authorization': 'Oauth {}'.format(NANNY_TOKEN), 'Content-Type': 'text/plain', 'If-Match': etag}
    response = requests.delete(its_url, timeout=10, verify=certifi.where(), headers=headers)
    return response.content, response.status_code


def rt_request():
    request = {
        host :  {
            c.tag : [c.signal] for c in config
        }
    }
    for point in RtGolovanRequest(request):
        for c in config:
            tag = point.values[host].get(c.tag)
            if not tag:
                continue
            value = tag.get(c.signal)
            if value is None:
                continue
            value = int(value) / 5
            if time() > c.captcha_till and all(v for v in c.etag.values()):
                for ctype in CTYPES:
                    for ruchka in ITS_RUCHKAS:
                        response, status_code = disable_its(c.service, c.etag[ruchka + ctype], ctype, ruchka)
                        logger.info(
                            f'Hide captcha for its: "{c.service}" ({ruchka}{ctype}), status code: {status_code}, response: [{response}]'
                        )
                        c.etag[ruchka + ctype] = ''
            elif value < c.limit:
                logger.info(
                    f"Current rps for {c.service}: {value}, crit: {c.limit}. This is fine!"
                )
            if value >= c.limit and time() > c.captcha_till:
                logger.info(
                    f"Current value: {value} rps in service_type '{c.service}'. DDoS detected (crit value: {c.limit})! Captcha turns on !"
                )
                for ctype in CTYPES:
                    for ruchka in ITS_RUCHKAS:
                        c.etag[ruchka + ctype], response, status_code = enable_its(c.service, ctype, ruchka)
                        logger.info(
                            f'Show captcha its: "{c.service}" ({ruchka}{ctype}), status code: {status_code}, response: [{response}]'
                        )
                        if not c.etag[ruchka + ctype]:
                            break
                else:
                    c.captcha_till = time() + CAPTCHA_DURATION
            if time() < c.captcha_till:
                if value >= c.limit and all(v for v in c.etag.values()):
                    c.captcha_till = time() + CAPTCHA_DURATION

                logger.info(
                    f"Captcha for {c.service} will turn off after {c.captcha_till - time()}s"
                )


def main():
    initLogger()
    loadNannyToken()
    logging.getLogger("requests").setLevel(logging.WARNING)
    logger.info("script was started")
    rt_request()


if __name__ == '__main__':
    logging.getLogger().setLevel(logging.INFO)
    logger = logging.getLogger(__name__)
    main()
