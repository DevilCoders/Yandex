from cloud.iam.bot.telebot.commands import Command


class StartCommand(Command):
    def __init__(self, bot):
        super().__init__(bot)

    def process(self, message):
        self._bot.send_message(message.chat.id, 'Welcome')
