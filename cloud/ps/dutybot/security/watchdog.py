import logging
from functools import wraps

from bot_utils.chats import Chat
from bot_utils.users import User
from models import Reply


def check_telegram_caller_permission(func):
    @wraps(func)
    def watchdog(obj, bot, update, *args, **kwargs):
        if not update.message and not update.callback_query:
            logging.warning(f"Unknown type of the update: {update}")
            return

        message = update.message if update.message is not None else update.callback_query.message
        if not message.chat_id:
            logging.info(f"{func.__qualname__} called by dutybot itself")
            return func(obj, bot, update, *args, **kwargs)

        if str(message.chat_id).startswith("-"):
            chat = Chat("telegram", chat_id=message.chat_id, chat_name=message.chat.username)
            if not chat.allowed:
                # TODO (azzpg7) Need to convert message before calling the decorator
                message = obj._convert_to_incoming_message(message)
                obj.messenger.reply(
                    message,
                    Reply("Бот в чате не активирован. Нажмите /start, чтобы активировать бота."),
                )
                logging.warning(f"Chat {chat.id} {chat.name} tried to use {func.__qualname__}")
                return

        user = User(chat_id=message.from_user.id, login_tg=message.from_user.username)
        if not user.allowed:
            # TODO (azzpg7) Need to convert message before calling the decorator
            message = obj._convert_to_incoming_message(message)
            obj.messenger.reply(
                message,
                Reply("У тебя нет прав для запуска бота."),
            )
            logging.warning(f"User {user.chat_id} {user.login_tg} tried to use {func.__qualname__}")
            return

        return func(obj, bot, update, *args, **kwargs)

    return watchdog


def check_telegram_admin_privileges(func):
    @wraps(func)
    def watchdog(obj, bot, update, *args, **kwargs):
        if update.message.chat.type != "private":
            logging.info("Chat's is not supported")
            return
        user = User(chat_id=update.message.chat_id, login_tg=update.message.chat.username)
        if user.bot_admin:
            logging.info(f"admin {update.message.chat.username} used {func.__qualname__} command")
            return func(obj, bot, update, *args, **kwargs)
        logging.warning(f"User {user.chat_id} {user.login_tg} tried to use admin command {func.__qualname__}")

    return watchdog


def check_telegram_cloud_department(func):
    # TODO (azzpg7) Add understandable replies
    @wraps(func)
    def watchdog(obj, bot, update, *args, **kwargs):
        if update.message.chat.type != "private":
            logging.info("Chat's is not supported")
            return
        user = User(chat_id=update.message.chat_id, login_tg=update.message.chat.username)
        if user.cloud:
            return func(obj, bot, update, *args, **kwargs)
        if user.allowed:
            update.message.reply_text(
                "Извините, эта функция доступна только сотрудникам Облака.",
                "Если у вас есть вопросы, обратитесь к https://abc.yandex-team.ru/services/cloud-oncall",
            )
        else:
            update.message.reply_text("У тебя нет доступа к этой команде.")
        logging.warning(f"User {user.chat_id} {user.login_tg} tried to use cloud command {func.__qualname__}")

    return watchdog
