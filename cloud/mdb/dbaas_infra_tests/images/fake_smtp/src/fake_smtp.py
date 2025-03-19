#!/usr/bin/env python
"""
Fake SMTP server.
"""

import collections
import json
import logging

from aiohttp import web
from aiosmtpd.controller import Controller

INBOX = collections.defaultdict(list)


class SMTPHandler:
    async def handle_DATA(self, server, session, envelope):
        rcpt_tos = envelope.rcpt_tos
        logging.info('Got message to %s', rcpt_tos)
        for element in rcpt_tos:
            INBOX[element].append(envelope.content.decode('utf-8'))
        return '250 OK'


async def handler(_):
    logging.info('Returning inbox: %s', json.dumps(INBOX))
    return web.Response(text=json.dumps(INBOX), content_type='application/json')


def _main():
    logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)s:\t%(message)s')
    smtp_handler = SMTPHandler()
    controller = Controller(smtp_handler, hostname='0.0.0.0', port=5000)
    controller.start()
    app = web.Application()
    app.router.add_get('/', handler)
    web.run_app(app, port=6000)


if __name__ == '__main__':
    _main()
