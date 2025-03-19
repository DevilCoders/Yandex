import logging
from typing import Optional

from telegram import InlineKeyboardMarkup

from messengers.abstract import AbstractMessenger
from models import Reply, IncomingCallbackQuery, CallbackQueryReply
from mssngr.botplatform.client.src import Bot


class YandexMessenger(AbstractMessenger):
    def __init__(self, bot: Bot):
        self._bot = bot

    def send_message(self, chat_id: str, reply: Reply, reply_to_message_id: Optional[str] = None):
        # andgein: reply markups doesn't work in Yandex Messenger even if we send them in requested format
        # (https://bp.mssngr.yandex.net/docs/api/bot/types/#inlinekeyboardmarkup)
        # so we just ignore `reply.inline_keyboard_markup` now
        try:
            self._bot.send_message(
                reply.text,
                chat_id=chat_id,
                disable_web_page_preview=reply.disable_web_page_preview,
                reply_markup=None,
                reply_to_message_id=int(reply_to_message_id) if reply_to_message_id else None,
            )
        except Exception as e:
            logging.warning(f"Can't send message to the chat {chat_id}: {e}", exc_info=e)

    def notify_admins(self, reply: Reply):
        # TODO (andgein): not implemented
        pass

    def is_user_a_member_of_chat(self, chat_id: str, user_id: int, ttl: int) -> bool:
        return True  # Yandex.Messenger doesn't support this feature

    def answer_callback_query(self, incoming: IncomingCallbackQuery, reply: CallbackQueryReply):
        raise NotImplementedError("Callback queries are not implemented for Yandex Messenger")

    def edit_message(self, chat_id: str, message_id: str, new_reply: Reply):
        # Message editing is not implemented for Yandex Messenger
        pass

    def edit_message_reply_markup(
        self, chat_id: str, message_id: str, new_inline_keyboard_markup: Optional[InlineKeyboardMarkup] = None
    ):
        raise NotImplementedError("Reply markups are not implemented in Yandex Messenger")
