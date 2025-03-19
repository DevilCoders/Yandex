from dataclasses import dataclass
from typing import Literal, List, Optional

from telegram import ParseMode, InlineKeyboardMarkup

from bot_utils.users import User


@dataclass
class IncomingChat:
    messenger: Literal["telegram", "yandex", "slack"]
    chat_id: str
    is_personal: bool
    title: str

    @classmethod
    def telegram(cls, chat_id: int, title: str):
        is_personal = chat_id > 0
        return cls("telegram", str(chat_id), is_personal, title)

    @classmethod
    def yandex(cls, chat_id: str, is_personal: bool, title: str):
        return cls("yandex", chat_id, is_personal, title)

    @classmethod
    def slack(cls, chat_id: str, is_personal: bool, title: str):
        return cls("slack", chat_id, is_personal, title)


@dataclass
class IncomingRequest:
    chat: IncomingChat
    user: User


@dataclass
class IncomingMessage(IncomingRequest):
    message_id: str
    text: str
    text_markdown: str
    args: List[str]
    reply_to_message: Optional["IncomingMessage"] = None


@dataclass
class IncomingCallbackQuery(IncomingRequest):
    query_id: str
    message: IncomingMessage
    data: str


@dataclass
class Reply:
    text: str

    format: Optional[ParseMode] = ParseMode.MARKDOWN  # None means TEXT
    quote_message: bool = True  # For Telegram and Yandex: quote original message, for Slack: start thread
    disable_web_page_preview: bool = True  # Telegram only
    inline_keyboard_markup: Optional[InlineKeyboardMarkup] = None  # Telegram only


@dataclass
class CallbackQueryReply:
    text: str

    show_alert: bool = False
