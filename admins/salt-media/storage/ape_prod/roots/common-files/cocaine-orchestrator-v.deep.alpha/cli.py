#!/usr/bin/env python

import json
import argparse

from tornado import gen
from tornado import ioloop

from cocaine.services import Service

from logdef import *
import const
import defaults


@gen.coroutine
def client(unicorn, argv):
    log = ContextAdapter(logging.getLogger(), {'routine': 'client'})
    if argv.setconf is not None:
        log.info('Configuration update requested.')
        log.info('Reading json-encoded configuration from file %s', argv.setconf)
        with open(argv.setconf, "r") as conffile:
            conf = conffile.read()
            conffile.close()
        log.debug('Making syntax check of configuration using json module')
        try:
            json.loads(conf)
        except:
            msg = 'Configuration syntax check failed!'
            log.critical(msg)
            print('\n\n'+msg+'\n\n')
            log.critical('Trace:\n\n', exc_info=True)
            exit(1)
        log.debug('Configuration syntax check passed successfully.')
        # getting lock
        log.debug('Getting lock: %s', const.UNICORN_SERVER_CONF_LOCK)
        try:
            conflock = yield unicorn.lock(const.UNICORN_SERVER_CONF_LOCK)
            result = yield conflock.rx.get(timeout=const.UNICORN_TIMEOUT_GETLOCK)
        except:
            try:
                yield conflock.tx.close()
                log.critical("Failed to get lock: %s", const.UNICORN_SERVER_CONF_LOCK)
            except:
                log.critical("Service unicorn failed.")
                exit(1)
            exit(1)
        log.debug('Lock acquired: %s', const.UNICORN_SERVER_CONF_LOCK)
        # getting conffile
        try:
            u = yield unicorn.get(const.UNICORN_SERVER_CONF_FILE)
            result = yield u.rx.get(timeout=const.UNICORN_TIMEOUT)
            # configuration already exists
            try:
                u = yield unicorn.put(const.UNICORN_SERVER_CONF_FILE, conf, result[1])
                result = yield u.rx.get(timeout=const.UNICORN_TIMEOUT)
                log.info('Configuration updated.')
                log.debug('Unicorn replied: %s', result)
                yield u.tx.close()
            except:
                log.critical('Can not write new configuration to unicorn! Exiting.\n')
                exit(1)
        except:
            # configuration doesn't exist
            try:
                yield u.tx.close()
                log.debug('Config "%s" not found in unicorn.', const.UNICORN_SERVER_CONF_FILE)
                u = yield unicorn.create(const.UNICORN_SERVER_CONF_FILE, conf)
                result = yield u.rx.get(timeout=const.UNICORN_TIMEOUT)
                yield u.tx.close()
                log.info('Configuration created: %s', argv.setconf)
            except:
                log.critical("Service unicorn failed.")
                exit(1)
        yield conflock.tx.close()
        exit(0)
    if argv.showconf:
        log.debug('Getting config %s'' from unicorn.', const.UNICORN_SERVER_CONF_FILE)
        try:
            u = yield unicorn.get(const.UNICORN_SERVER_CONF_FILE)
            result = yield u.rx.get(timeout=const.UNICORN_TIMEOUT)
            # configuration exists
            log.debug('Unicorn replied: %s', result)
            print json.dumps(json.loads(result[0]), sort_keys=True, indent=4, separators=(',', ': '))
            yield u.tx.close()
        except:
            # configuration doesn't exist
            try:
                yield u.tx.close()
                log.critical('Configuration %s not found in unicorn!', const.UNICORN_SERVER_CONF_FILE)
                exit(1)
            except:
                log.critical("Service unicorn failed.", exc_info=True)
                exit(1)
        exit(0)
    log.critical('Unknown usage case')
    exit(1)


def main():

    def get_srv(service_name):
        return Service(service_name, endpoints=[[argv.host, argv.port]])

    p = argparse.ArgumentParser()
    p.add_argument("--host", type=str, default=defaults.LOCATOR_HOST, help="locator-service host")
    p.add_argument("--port", type=int, default=defaults.LOCATOR_PORT, help="locator-service port")
    p.add_argument("--setconf", metavar='server.json', type=str, help="set new orchestrator configuration from given file")
    p.add_argument("--showconf", action='store_true', help="show current orchestrator configuration stored in unicorn")
    p.add_argument("--debug", action='store_true', help="enable debug mode and print all messages to stderr")
    p.add_argument("--logfile", metavar='filename.log', type=str, default=None, help="path to logfile")
    argv = p.parse_args()
    if argv.debug:
        loglevel = LOGLEVEL_DEBUG
    else:
        loglevel = LOGLEVEL_DEFAULT
    logging.basicConfig(format=LOG_FORMAT, level=loglevel, filename=argv.logfile)
    log = ContextAdapter(logging.getLogger(), {'routine': 'main'})

    log.debug('Using debug logging level.')
    log.debug('Using locator: %s:%d', argv.host, argv.port)

    unicorn = get_srv("unicorn")

    log.debug('Starting ochestrator command-line client.')
    try:
        ioloop.IOLoop.current().run_sync(lambda: [client(unicorn, argv)], timeout=const.UNICORN_TIMEOUT)
    except ioloop.TimeoutError:
        log.critical("ioloop client() timeout")
        exit(1)

# entry point
if __name__ == '__main__':
    main()
