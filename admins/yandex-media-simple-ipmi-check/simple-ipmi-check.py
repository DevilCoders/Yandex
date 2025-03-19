#!/usr/bin/python

from __future__ import print_function

import commands
import json
import logging as log
import socket
import time
import urllib2
from functools import wraps
from logging.handlers import RotatingFileHandler
from ssl import SSLError

logFile = '/var/log/ipmi_monitoring/ipmi_monitoring.log'
apiUrl = 'https://bot.yandex-team.ru/api/osinfo.php?fqdn=%(fqdn)s&output=instance_number'
ipmiUrl = 'https://bot.yandex-team.ru/api/m3/ipmi.php'
ipmiUrlParams = '?inv=%d&action=%s'
sleepTime = 10 * 60
fqdn = socket.getfqdn()

handler = RotatingFileHandler(logFile, maxBytes=50*1024*1024, backupCount=2)
handler.setFormatter(log.Formatter('%(asctime)s\t%(levelname)s\t%(message)s'))
handler.setLevel(log.DEBUG)
log.getLogger().addHandler(handler)
log.getLogger().setLevel(log.DEBUG)


def retry(ExceptionToCheck, tries=3, delay=3, backoff=2, logger=None, debug=False):
    """Retry calling the decorated function using an exponential backoff.

    http://www.saltycrane.com/blog/2009/11/trying-out-retry-decorator-python/
    original from: http://wiki.python.org/moin/PythonDecoratorLibrary#Retry

    :param ExceptionToCheck: the exception to check. may be a tuple of
        exceptions to check
    :type ExceptionToCheck: Exception or tuple
    :param tries: number of times to try (not retry) before giving up
    :type tries: int
    :param delay: initial delay between retries in seconds
    :type delay: int
    :param backoff: backoff multiplier e.g. value of 2 will double the delay
        each retry
    :type backoff: int
    :param logger: logger to use. If None, print
    :type logger: log.Logger instance
    """
    def deco_retry(f):

        @wraps(f)
        def f_retry(*args, **kwargs):
            mtries, mdelay = tries, delay
            while mtries > 1:
                try:
                    return f(*args, **kwargs)
                except ExceptionToCheck as e:
                    msg = "%s, Retrying in %d seconds...; %s(*%s, **%s)" % \
                            (str(e), mdelay, f.__name__, str(args), str(kwargs))
                    if logger:
                        logger.warning(msg)
                    elif debug:
                        print(msg)
                    time.sleep(mdelay)
                    mtries -= 1
                    mdelay *= backoff
            return f(*args, **kwargs)

        return f_retry  # true decorator

    return deco_retry


@retry((urllib2.URLError, socket.timeout, SSLError), tries=3, delay=3, backoff=2, logger=log)
def get_url(url):
    log.debug("Get: %s" % url)
    return urllib2.urlopen(url, timeout=3).read()


def die(code, mes):
    print("%d;%s" % (code, mes))
    exit(0)


def getIpmiStatus(id):
    url = ipmiUrl + ipmiUrlParams % (id, 'check')
    log.info("Check bot api about my IPMI statistic: %s" % url)
    result = get_url(url)
    if not result:
        result = 'empty'
    return True if 'empty' in result else False


def resetIpmiStatus(myID):
    url = ipmiUrl + ipmiUrlParams % (myID, 'remove')
    log.info("Reset my IPMI statistic using bot api: %s" % url)
    get_url(url)
    log.info("Sleep %d" % sleepTime)
    time.sleep(sleepTime)
    if getIpmiStatus(myID):
        log.info("I'm OK!")
        die(0, 'OK')


def runIpmitool(cmd):
    log.debug("Runing: \"%s\"" % cmd)
    code, stdout = commands.getstatusoutput(cmd)
    if code:
        log.error("Failed: \"%s\"!" % stdout)
    return code


def resetBmc(myID):
    log.warning("Try to reset BMC")
    service('start')
    err = runIpmitool('ipmitool mc reset cold')
    service('stop')
    if not err:
        resetIpmiStatus(myID)


def service(action):
    cmd = 'service openipmi %s 2>&1' % action
    log.debug("Runing: \"%s\"" % cmd)
    code, stdout = commands.getstatusoutput(cmd)
    if code:
        mes = "Can't %s openipmi: %s" % (action, stdout.replace('\n', ' \\n '))
        log.critical(mes)
        die(2, mes)


def getMyServer(fqdn):
    try:
        return int(json.loads(get_url(apiUrl % {"fqdn": fqdn}))["os"][0]["instance_number"])
    except Exception as exc:
        mes = "Bot know nothing about %s. Is it virtual server?" % fqdn
        log.info(mes + " Exception = %s", repr(exc))
        die(0, mes)


def main():
    log.info('Start check')
    log.debug('My FQDN: %s' % fqdn)
    log.debug("Finding my bot id")
    myID = getMyServer(fqdn)
    log.debug("My bot ID: %d" % myID)
    if getIpmiStatus(myID):
        log.info("I'm OK!")
        die(0, 'OK')
    resetBmc(myID)
    mes = "IPMI doesn't work =("
    log.critical(mes)
    die(2, mes)


if __name__ == '__main__':
    main()
