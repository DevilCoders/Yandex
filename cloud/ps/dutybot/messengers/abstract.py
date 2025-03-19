import abc
from typing import Optional

from telegram import InlineKeyboardMarkup

from models import IncomingMessage, Reply, IncomingCallbackQuery, CallbackQueryReply


class AbstractMessenger:
    def reply(self, incoming: IncomingMessage, reply: Reply):
        reply_to_message_id: Optional[str] = None
        if reply.quote_message and not incoming.chat.is_personal:
            reply_to_message_id = incoming.message_id

        self.send_message(incoming.chat.chat_id, reply, reply_to_message_id)

    @abc.abstractmethod
    def notify_admins(self, reply: Reply):
        pass

    @abc.abstractmethod
    def send_message(self, chat_id: str, reply: Reply, reply_to_message_id: Optional[str] = None):
        pass

    @abc.abstractmethod
    def is_user_a_member_of_chat(self, chat_id: str, user_id: str, ttl: int) -> bool:
        pass

    @abc.abstractmethod
    def answer_callback_query(self, incoming: IncomingCallbackQuery, reply: CallbackQueryReply):
        pass

    @abc.abstractmethod
    def edit_message(self, chat_id: str, message_id: str, new_reply: Reply):
        pass

    @abc.abstractmethod
    def edit_message_reply_markup(
        self, chat_id: str, message_id: str, new_inline_keyboard_markup: Optional[InlineKeyboardMarkup] = None
    ):
        pass
