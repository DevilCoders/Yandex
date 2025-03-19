import logging
from functools import wraps
from typing import Literal, Callable, Optional

import aiohttp.web
import requests

import mssngr.botplatform.client.src
from bot_utils.config import Config
from bot_utils.users import User
from bots.abstract import AbstractDutyBot
from messengers.yandex import YandexMessenger
from models import IncomingChat, IncomingMessage
from templaters.yandex import YandexMessengerTemplater


class YandexMessengerDutyBot(AbstractDutyBot):
    templater = YandexMessengerTemplater()

    def __init__(self, config: Config, environment: Literal["prod"]):
        super().__init__(config)

        # Currently, we support only prod environment for Yandex Messenger :(
        if environment not in ("prod",):
            raise ValueError(f"Unknown environment for Yandex.Messenger: {environment}")
        self._environment = environment

        self.token = config.yandex_messenger_prod

        self.bot = mssngr.botplatform.client.src.Bot(
            mssngr.botplatform.client.src.PROD_HOST, token=self.token, is_team=True
        )
        self.messenger = YandexMessenger(self.bot)

        # Add handlers
        self._add_handler(r"/duty(\s|$)", self.command_duty)
        self._add_handler(r"/inc(\s|$)", self.command_create_incident)
        self._add_handler(r"/help(\s|$)", self.command_help)
        self._add_handler(r"1\d{14}", self.command_resolve_support_ticket)
        self._add_handler(r"/dt(\s|$)", self.command_get_duty_ticket)

    def run(self):
        logging.info("Start yandex messenger bot for environment %s", self._environment)
        if self._config.use_webhook:
            logging.info("Setting webhook at %s", self._config.yandex_messenger_webhook_url)
            self._set_webhook(self._config.yandex_messenger_webhook_url)
            # TODO: set online_status (see https://bp.mssngr.yandex.net/docs/api/team/methods/#izmenenie-statusa-onlainovosti-bota)
            self._start_webhook_server(host="0.0.0.0", port=8081, url_path="yandex")
        else:
            self._remove_webhook()
            self.bot.polling()

    def _set_webhook(self, webhook_url: Optional[str]):
        # See https://bp.mssngr.yandex.net/docs/api/team/methods/#obnovlenie-dannykh-nastroek-bota
        logging.info(f"Setting webhook url to {webhook_url}")
        request_url = f"{mssngr.botplatform.client.src.PROD_HOST}/team/update/"
        r = requests.post(
            request_url,
            headers={"Authorization": f"OAuthTeam {self.token}", "Content-Type": "application/json"},
            json={"webhook_url": webhook_url},
        )
        r.raise_for_status()

        logging.info(f"Bot settings received from BotPlatform Team API: {r.json()}")

    def _remove_webhook(self):
        self._set_webhook(None)

    def _start_webhook_server(self, host: str, port: int, url_path: str):
        async def healthcheck(_):
            return aiohttp.web.Response(body="ok")

        async def handler(request):
            try:
                data = await request.json()
                self.bot.serve_update(mssngr.botplatform.client.src.Update.from_dict(data))
                return aiohttp.web.Response()
            except Exception as e:
                logging.error(e, exc_info=e)
                return aiohttp.web.Response(status=400, text=str(e))

        app = aiohttp.web.Application()
        app.router.add_get("/", healthcheck)
        app.router.add_post("/" + url_path, handler)

        aiohttp.web.run_app(app, host=host, port=port)

    def _convert_to_incoming_message(self, message: mssngr.botplatform.client.src.Message) -> IncomingMessage:
        caller = message.from_user.login if message.from_user.login else "dutybot"
        chat_title = message.chat.title if message.chat.title else caller
        chat = IncomingChat.yandex(message.chat.id, message.chat.type != "group", chat_title)

        # Extract args like python-telegram-bot does it
        args = message.text.split()
        if args and args[0].startswith("/"):
            args = args[1:]

        return IncomingMessage(
            chat,
            User(login_staff=message.from_user.login),
            str(message.id),
            message.text,
            message.text,
            args,
            self._convert_to_incoming_message(message.reply_to) if message.reply_to else None,
        )

    def _add_handler(
        self,
        regexp: str,
        handler: Callable[[IncomingMessage, ], None],
    ):
        @wraps(handler)
        def f(message: mssngr.botplatform.client.src.Message):
            # Ignore forwarded messages
            if message.forwarded_messaged:
                return

            message = self._convert_to_incoming_message(message)
            handler(message)

        self.bot.message_handler(regexp=regexp)(f)
