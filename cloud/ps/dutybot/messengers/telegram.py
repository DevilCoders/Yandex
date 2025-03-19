import logging
from functools import lru_cache
from typing import Optional

import telegram
from telegram import InlineKeyboardMarkup

from messengers.abstract import AbstractMessenger
from models import Reply, IncomingCallbackQuery, CallbackQueryReply


class TelegramMessenger(AbstractMessenger):
    def __init__(self, bot: telegram.Bot):
        self._bot = bot

    def notify_admins(self, reply: Reply):
        # TODO: replace with sending to bot admin chats
        andgein_chat_id = "960926"
        self.send_message(andgein_chat_id, reply)

    def send_message(self, chat_id: str, reply: Reply, reply_to_message_id: Optional[str] = None):
        try:
            self._bot.send_message(
                chat_id,
                reply.text,
                parse_mode=reply.format,
                disable_web_page_preview=reply.disable_web_page_preview,
                reply_markup=reply.inline_keyboard_markup,
                reply_to_message_id=reply_to_message_id,
            )
        except Exception as e:
            logging.warning(f"Can't send message to the chat {chat_id}: {e}", exc_info=e)

    @lru_cache(maxsize=64)
    # use additional arg `ttl` to invalidate cache every ttl seconds
    def is_user_a_member_of_chat(self, chat_id: str, user_id: int, ttl: int) -> bool:
        try:
            logging.info(f"Checking if {user_id} is a member of telegram chat {chat_id}")
            chat_member = self._bot.get_chat_member(chat_id, user_id)
            logging.debug(f"Telegram response: {chat_member}")
            if chat_member is None:
                return False
            # See https://core.telegram.org/bots/api#chatmember
            return chat_member.status not in ["left", "kicked"]
        except telegram.error.BadRequest as e:
            if "user not found" in e.message.lower():
                return False
            logging.debug(f"Bad request for {user_id}, {e}")
        except telegram.TelegramError as e:
            logging.debug(f"Can not check {user_id}, {e}")
        return False

    def answer_callback_query(self, incoming: IncomingCallbackQuery, reply: CallbackQueryReply):
        self._bot.answer_callback_query(incoming.query_id, text=reply.text, show_alert=reply.show_alert)

    def edit_message(self, chat_id: str, message_id: str, new_reply: Reply):
        self._bot.edit_message_text(
            new_reply.text,
            chat_id=chat_id,
            message_id=message_id,
            parse_mode=new_reply.format,
            disable_web_page_preview=new_reply.disable_web_page_preview,
            reply_markup=new_reply.inline_keyboard_markup,
        )

    def edit_message_reply_markup(
        self, chat_id: str, message_id: str, new_inline_keyboard_markup: Optional[InlineKeyboardMarkup] = None
    ):
        self._bot.edit_message_reply_markup(chat_id, message_id, reply_markup=new_inline_keyboard_markup)
