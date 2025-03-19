#!/usr/bin/env python
# -*- coding: utf-8 -*-

import datetime
import json
import logging
import os
import random
import requests
import time

from collections import defaultdict


class BotApi(object):

    def __init__(self, token):
        self.base_url = "https://api.telegram.org/bot%s/" % token
        self.offset = 0

    def __call(self, method, params):
        r = requests.get("%s%s" % (self.base_url, method), params)
        r.raise_for_status()
        j = r.json()
        if j.get("ok") is not True:
            raise Exception("bad response json: %s" % json.dumps(j))
        return j["result"]

    def get_updates(self):
        result = self.__call("getUpdates", [("offset", self.offset)])
        messages = []
        for x in result:
            update_id = x.get("update_id")
            if update_id >= self.offset:
                self.offset = update_id + 1
            messages.append(x.get("message"))
        return messages

    def send_message(self, chat_id, text):
        self.__call("sendMessage", [("chat_id", chat_id), ("text", text)])

    def send_sticker(self, chat_id, file_id):
        self.__call("sendSticker", [("chat_id", chat_id), ("sticker", file_id)])


NIGHTTIME_STICKERS = ["CAADBAADFQMAAlGMzwGuHBwgmFOPVAI"]
GRIM_STICKERS = ["CAADBAADOgMAAlGMzwEWaCzFWso3zQI"]
DEFAULT_STICKERS = [
    "CAADAgADGgADb4UTA0yuEegFIFmKAg",
    "CAADAgADDQADb4UTA_cY10t67pd4Ag",
]
ASK_FOR_DETAILS_STICKERS = [
    "CAADAgADDgADb4UTA6lgQxN6JU1dAg"
]
ALREADY_STICKERS = [
    "CAADAgADEAADb4UTA4DIqJ7mXPYkAg",
    "CAADAgADEgADb4UTAxbiPFJWeqBfAg",
]
GTFO_STICKERS = [
    "CAADAgADHRQAAk9tKgAB1lShEPa57rAC",
]
ARCADIA_STICKERS = [
    "CAADAgADFQADPycFBS6gmQL_x6COAg"
]
WELCOME_STICKERS = [
    "CAADBQADbwMAAukKyAOvzr7ZArpddAI"
]
WTF_STICKERS = [
    "CAADAgADBAADijc4AAFx0NNqDnJm4QI",
    "CAADAgADLQADijc4AAGBowxjAqAlGwI",
    "CAADAgADEAADijc4AAESVXqKiwYE2wI",
    "CAADAgADEgADijc4AAF00GirhpifXQI",
    "CAADAgADFAADijc4AAGtl5dISqHmiAI",
    "CAADAgADGwADijc4AAEdwByBSe9kgQI",
    "CAADAgADHQADijc4AAEw0RBgpCTPAAEC",
]


def get_sticker(stickers):
    return random.choice(stickers)


class ChatContext(object):

    def __init__(self):
        self.is_dutytime = False
        self.user2message_infos = defaultdict(list)


DUTY_BEGINS_MESSAGE = "Дежурная смена начинается."
DUTY_ENDS_MESSAGE = "Дежурная смена закончилась. В случае факапа пишите, пожалуйста, слово \"факап\" в теле сообщения, чтобы привлечь внимание дежурных."
DUTY_OFFLINE_MESSAGE = "Дежурная смена возобновится утром."


def matches(text, alternatives):
    for a in alternatives:
        if text.find(a.decode("utf8")) >= 0:
            return True

    return False


class NBSExpertBot(object):

    def __init__(self, bot_api):
        self.bot_api = bot_api
        self.chats = defaultdict(ChatContext)

    def __on_message(self, message, chat_id, chat_context):
        text = message.get("text", "").lower()

        if matches(text, ["факап"]):
            self.bot_api.send_sticker(chat_id, get_sticker(ALREADY_STICKERS))
            # TODO: call the person who's on duty right now
            self.bot_api.send_message(chat_id, "@qkrorlqr @vskipin")
            return

        if matches(text, ["спасибо"]):
            self.bot_api.send_sticker(chat_id, get_sticker(WELCOME_STICKERS))
            return

        # TODO: use arcadia/ml/neocortex to try to find an appropriate answer in chat history

        if chat_context.is_dutytime:
            return

        from_id = message["from"]["id"]
        message_infos = chat_context.user2message_infos[from_id]
        ts = message["date"]
        message_infos.append(ts)
        i = 0
        while ts - message_infos[i] > 1200:
            i += 1
        if i > 0:
            del message_infos[0:i]

        if len(message_infos) > 10:
            self.bot_api.send_sticker(chat_id, get_sticker(GTFO_STICKERS))
            return

        self.bot_api.send_message(chat_id, DUTY_OFFLINE_MESSAGE)

        if matches(text, ["bb.yandex-team", "bitbucket", "битбакет", "github", "гитхаб"]):
            self.bot_api.send_sticker(chat_id, get_sticker(ARCADIA_STICKERS))
            return

        if matches(text, ["=)", ":)", ")))", "(:"]):
            self.bot_api.send_sticker(chat_id, get_sticker(GRIM_STICKERS))
            return

        now = datetime.datetime.now()
        if now.hour < 10:
            self.bot_api.send_sticker(chat_id, get_sticker(NIGHTTIME_STICKERS))
            return

        if len(text) < 50:
            self.bot_api.send_sticker(chat_id, get_sticker(ASK_FOR_DETAILS_STICKERS))
            return

        self.bot_api.send_sticker(chat_id, get_sticker(DEFAULT_STICKERS))

    def iter(self):
        messages = self.bot_api.get_updates()
        for message in messages:
            try:
                chat_id = message["chat"]["id"]
                self.__on_message(message, chat_id, self.chats[chat_id])
            except Exception as e:
                logging.error("error while processing message: %s, e: %s" % (json.dumps(message), e))

        for chat_id, chat_context in self.chats.iteritems():
            now = datetime.datetime.now()
            is_dutytime = now.hour > 10 and now.hour < 20
            text = None
            if chat_context.is_dutytime and not is_dutytime:
                text = DUTY_ENDS_MESSAGE
            elif not chat_context.is_dutytime and is_dutytime:
                text = DUTY_BEGINS_MESSAGE

            logging.warn(text)
            # if text is not None:
            #     self.bot_api.send_message(chat_id, text)

            chat_context.is_dutytime = is_dutytime


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    bot = NBSExpertBot(BotApi(os.environ["NBS_EXPERT_BOT_TOKEN"]))
    while True:
        bot.iter()
        time.sleep(1)
