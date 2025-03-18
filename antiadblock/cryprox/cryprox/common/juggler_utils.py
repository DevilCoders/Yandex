import logging
import os
import socket
from json import dumps

from tornado.httpclient import AsyncHTTPClient, HTTPError
from tornado.gen import coroutine


JUGGLER_CLIENT_ADDRESS = 'http://localhost:31579/events'
LOCAL_HOSTNAME = socket.getfqdn()


@coroutine
def send_juggler_event(description, service, status):
    json = {"source": "antiadb.cryprox", "events": [
        {"description": description,
         "host": LOCAL_HOSTNAME,
         "instance": "",
         "service": service,
         "status": status,
         }]}
    if os.getenv("ENV_TYPE") in ('staging', 'production'):
        try:
            client = AsyncHTTPClient(force_instance=True)
            yield client.fetch(JUGGLER_CLIENT_ADDRESS, method='POST', body=dumps(json))
        except (HTTPError, socket.error):
            logging.exception("Error trying to send juggler alert")
    else:
        logging.info("JUGGLER-IMITATING LOG MESSAGE: {}".format(dumps(json)))
