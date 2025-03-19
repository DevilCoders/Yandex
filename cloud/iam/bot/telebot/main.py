import argparse
import logging
import jinja2
import os
import pathlib
import pprint
import ssl
import telebot
import yaml

from aiohttp import web
from pathlib import Path
from tvmauth import BlackboxTvmId
from tvm2 import TVM2

from cloud.iam.bot.telebot.bot import Bot
from cloud.iam.bot.telebot.clients import Staff, Paste, Startrek
from cloud.iam.bot.telebot.authenticator import Authenticator
from cloud.iam.bot.telebot.db import Db


def main():
    logging.basicConfig(level=logging.INFO)

    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', help='Configuration file path', default='config.yaml')
    parser.add_argument('--no-webhook', action='store_true', help='dont set webhook')

    args = parser.parse_args()

    with open(args.config, 'r') as f:
        config = yaml.safe_load(f)

    config_dir = pathlib.Path(args.config).parent.resolve()

    logging.info('Configuration' + os.linesep + pprint.pformat(config))

    oauth_token = Path(config_dir, config['yandex_team_authentication']['token_file']).read_text()

    # https://abc.yandex-team.ru/services/staffapi/resources/?view=consuming&layout=table&supplier=14&type=47&show-resource=4006336
    # https://a.yandex-team.ru/svn/trunk/arcadia/library/python/tvm2
    tvm = TVM2(
        client_id=str(config['tvm']['client_id']),
        secret=Path(config_dir, config['tvm']['secret_file']).read_text(),
        blackbox_client=BlackboxTvmId.Prod,
        destinations=(Staff.application_id,)
    )

    staff = Staff(config['staff'], tvm)
    startrek = Startrek(config['startrek'], oauth_token)
    paste = Paste(config['paste'])
    authenticator = Authenticator(staff, config['roles'])
    db = Db(config['database'])

    jinja = jinja2.Environment(loader=jinja2.PackageLoader('cloud.iam.bot.telebot'),
                               autoescape=jinja2.select_autoescape(['html']))

    telebot.apihelper.ENABLE_MIDDLEWARE = True
    telebot.logger.setLevel(logging.DEBUG)

    token = Path(config_dir, config['telegram']['token_file']).read_text()
    bot = Bot(token, config_dir, config, authenticator, db, jinja, paste, startrek)

    if not args.no_webhook:
        webhook_path = bot.set_webhook()

    async def handle(request):
        h = request.match_info.get('webhook_path')
        if h == webhook_path:
            request_body_dict = await request.json()
            bot.process_request(request_body_dict)
            return web.Response()
        else:
            logging.warn('Unknown token hash ' + h)
            return web.Response(status=403)

    async def healthcheck_handle(request):
        return web.Response()

    app = web.Application()
    app.router.add_post('/{webhook_path}/', handle)
    app.router.add_get('/healthcheck', healthcheck_handle)

    ssl_context = None
    if 'ssl' in config['telegram']['webhook']:
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
        ssl_context.load_cert_chain(config['telegram']['webhook']['listen']['ssl']['certificate_file'],
                                    config['telegram']['webhook']['listen']['ssl']['private_key_file'])

    web.run_app(
        app,
        host=config['telegram']['webhook']['listen']['host'],
        port=config['telegram']['webhook']['listen']['port'],
        ssl_context=ssl_context)


if __name__ == "__main__":
    main()
