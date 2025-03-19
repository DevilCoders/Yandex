import re

from cloud.iam.bot.telebot.commands import Command


#
# /chats
# /chats new <chat_id>
# /chats delete <chat_id>
#
class ChatsCommand(Command):
    def __init__(self, bot, db):
        super().__init__(bot)

        self._db = db
        self._help_message = """
 • `/chats` — list registered chats
 • `/chats new <chat_id>` — register a new chat
 • `/chats delete <chat_id>` — unregister a chat
"""

    def process(self, message):
        result = re.fullmatch(r'/chats\s*(new\s+([0-9-]+)|delete\s+([0-9-]+))?', message.text)
        if not result:
            return self._bot.send_message(chat_id=message.chat.id,
                                          reply_to_message_id=message.id,
                                          parse_mode='MarkdownV2',
                                          text='Invalid `/chats` command format:\n' + self._help_message)

        create_chat_id = result.group(2)
        delete_chat_id = result.group(3)

        if create_chat_id:
            return self._create_chat(message, create_chat_id)
        elif delete_chat_id:
            return self._delete_chat(message, delete_chat_id)
        else:
            return self._list_chats(message)

    def _list_chats(self, message):
        chats = self._db.list_chats()

        self._bot.send_message(chat_id=message.chat.id,
                               reply_to_message_id=message.id,
                               text=str(chats))

    def _create_chat(self, message, chat_id):
        try:
            self._db.create_chat(int(chat_id))
        except ValueError:
            return self._bot.send_message(chat_id=message.chat.id,
                                          reply_to_message_id=message.id,
                                          parse_mode='MarkdownV2',
                                          text='Invalid `/chats new <chat_id>` command: `chat_id` is not an integer')

        self._bot.send_message(chat_id=message.chat.id,
                                       reply_to_message_id=message.id,
                                       parse_mode='MarkdownV2',
                                       text='Chat `' + str(chat_id) + '` has been registered')

    def _delete_chat(self, message, chat_id):
        try:
            self._db.delete_chat(int(chat_id))
        except ValueError:
            return self._bot.send_message(chat_id=message.chat.id,
                                          reply_to_message_id=message.id,
                                          parse_mode='MarkdownV2',
                                          text='Invalid `/chats delete <chat_id>` command: `chat_id` is not an integer')

        self._bot.send_message(chat_id=message.chat.id,
                                       reply_to_message_id=message.id,
                                       parse_mode='MarkdownV2',
                                       text='Chat `' + str(chat_id) + '` has been unregistered')
