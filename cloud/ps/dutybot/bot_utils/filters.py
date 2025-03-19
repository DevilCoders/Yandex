from telegram.ext import BaseFilter, CommandHandler

from bot_utils.feedback import Feedback


class TicketsResolver(BaseFilter):
    name = "CustomFilter.ticket"

    def filter(self, message):
        try:
            return message.text.startswith('1') and len(message.text) == 15 and message.text.isdigit()
        except AttributeError:
            pass
            """This exception happens when
            message from channel or from another
            bot was parsed"""


class FeedbackFilter(BaseFilter):
    name = "CustomFilter.feedback"

    def filter(self, message):
        try:
            if not str(message.chat_id).startswith("-"):
                feedback = Feedback.get_last_feedback(message.chat_id)
                return len(message.text.strip()) and feedback is not None
            return False
        except AttributeError:
            pass
            """This exception happens when
            message from channel or from another
            bot was parsed"""


class ForwardProtectedHandler(CommandHandler):
    def __init__(self, command, callback, pass_args=False):
        super(ForwardProtectedHandler, self).__init__(command=command, callback=callback, pass_args=pass_args)

    def check_update(self, update):
        if update.message or update.edited_message:
            message = update.message or update.edited_message
            if not message.text:
                message.text = message.caption

            # User account may be hided, so we have only one way to
            # identify if message was forwarded or not: check forward_date
            # in GORE-106 we decided to ignore forwarded messages
            if message.forward_date:
                return False

            if message.text and message.text.startswith('/') and len(message.text) > 1:
                first_word = message.text_html.split(None, 1)[0]
                if len(first_word) > 1 and first_word.startswith('/'):
                    command = first_word[1:].split('@')
                    command.append(message.bot.username)  # in case the command was sent without a username

                    if not (command[0].lower() in self.command and command[1].lower() == message.bot.username.lower()):
                        return False

                    if self.filters is None:
                        return True
                    elif isinstance(self.filters, list):
                        return any(func(message) for func in self.filters)

                    return self.filters(message)
