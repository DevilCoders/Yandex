import hashlib
import telebot

from telebot import TeleBot
from telebot.custom_filters import SimpleCustomFilter

from cloud.iam.bot.telebot.commands import ChatsCommand, StartCommand, AccessLogCommand, StartrekCommand


class RoleFilter(SimpleCustomFilter):
    def __init__(self, bot, role):
        self._bot = bot
        self._role = role

    def check(self, message):
        if message.authenticated_user.has_role(self._role):
            return True

        self._bot._forbidden(message)

        return False


class IsAdmin(RoleFilter):
    key = 'is_admin'

    def __init__(self, bot):
        super().__init__(bot, 'admin')


class IsUser(RoleFilter):
    key = 'is_user'

    def __init__(self, bot):
        super().__init__(bot, 'user')


class KnownChat(SimpleCustomFilter):
    key = 'known_chat'

    def __init__(self, bot):
        self._bot = bot

    def check(self, message):
        # Allow private chats
        if message.chat.type == 'private':
            return True

        # Skip leave chat updates
        if message.new_chat_member and message.new_chat_member.user.id == self._bot._bot_id and message.new_chat_member.status == 'left':
            return False

        # Allow messages in registered chats
        if self._bot._db.find_chat(message.chat.id):
            return True

        self._bot._forbidden(message)

        if message.chat.type != 'private':
            self._bot._bot.leave_chat(message.chat.id)

        return False


class ExceptionHandler:
    def handle(self, e):
        print(e)


class Bot:
    def __init__(self, token, config_dir, config, authenticator, db, jinja, paste_client, startrek_client):
        bot = TeleBot(token=token, exception_handler=ExceptionHandler())

        self._bot = bot
        self._token = token
        self._config = config
        self._authenticator = authenticator
        self._db = db

        bot.add_custom_filter(KnownChat(self))
        bot.add_custom_filter(IsAdmin(self))
        bot.add_custom_filter(IsUser(self))

        start_command = StartCommand(bot)
        chats_command = ChatsCommand(bot, db)
        access_log_command = AccessLogCommand(bot, config_dir, config['access_log_database'], jinja, paste_client)
        startrek_command = StartrekCommand(bot, jinja, startrek_client)

        @bot.middleware_handler(update_types=['message'])
        def authenticate(bot, message):
            message.authenticated_user = authenticator.authenticate(message.from_user.username)

        @bot.message_handler(commands=['help', 'start'], known_chat=True)
        def message_handler_start(message):
            start_command.process(message)

        @bot.message_handler(commands=['chats'], known_chat=True, is_admin=True)
        def message_handler_chats(message):
            chats_command.process(message)

        @bot.message_handler(commands=['access-log'], known_chat=True, is_user=True)
        def message_handler_access_log(message):
            access_log_command.process(message)

        @bot.message_handler(commands=['st', 'startrek'], known_chat=True, is_user=True)
        def message_handler_startrek(message):
            startrek_command.process(message)

    def set_webhook(self):
        token_hash = hashlib.sha256(self._token.encode('utf-8')).hexdigest()

        self._bot.remove_webhook()
        self._bot.set_webhook(url='https://{}:{}/{}/'.format(self._config['telegram']['webhook']['host'],
                                                             self._config['telegram']['webhook']['port'],
                                                             token_hash))

        return token_hash

    def process_request(self, body):
        update = telebot.types.Update.de_json(body)
        self._bot.process_new_updates([update])

    def _forbidden(self, message):
        self._bot.send_sticker(chat_id=message.chat.id,
                               sticker='CAACAgIAAxkBAAMsYdGgHPoBR_5OcxTEfSTAi5mUp54AAmsAA8GcYAyWtN-bkygmMyME',
                               reply_to_message_id=message.id if type(message) == telebot.types.Message else None)
