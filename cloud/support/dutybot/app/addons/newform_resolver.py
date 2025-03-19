import logging

from telegram.ext import BaseFilter, MessageHandler
from telegram.ext.dispatcher import run_async

from app.utils.classes import User


# Custom filter for new support form ticket id's
class CustomFilter(BaseFilter):
    class TicketsResolver(BaseFilter):
        name = "CustomFilter.ticket"

        def filter(self, message):
            try:
                return bool(message.text.startswith('1') and len(message.text) == 15)
            except AttributeError:
                """This exception happens when
                message from channel or from another
                bot was parsed"""

    ticket = TicketsResolver()


class TicketsResolver:
    def __init__(self, updater, dispatcher, st_client):
        self.updater = updater
        self.dispatcher = dispatcher
        self.st_client = st_client
        self.dispatcher.add_handler(MessageHandler(filters=CustomFilter.ticket, callback=self.command_whois))

    def search_ticket(self, hash):
        query = f'Queue: "Поддержка Облака" and "Ticked ID": "{hash}"'
        result = [issue.key for issue in self.st_client.issues.find(query=query)]
        if result == []:
            return None
        else:
            return result

    @run_async
    def command_whois(self, bot, update):
        if str(update.message.chat.id).startswith('-') is False:
            user = User(chat_id=update.message.chat_id,
                        login_tg=update.message.chat.username)
            logging.info(f"[newform_resolver.command_whois] user {user.login_staff} resolving {update.message.text}")
            if user.get_user is True:
                ticket = self.search_ticket(update.message.text)
                logging.info(f"[newform_resolver.command_whois] found {ticket}")
                if ticket is None:
                    update.message.reply_text("❌ Не нашёл тикет с таким ID")
                else:
                    update.message.reply_text(f"🔍 По этому ID я нашел тикет <a href='https://st.yandex-team.ru/{ticket[0]}'>{ticket[0]}</a>",
                                              parse_mode="HTML",
                                              disable_web_page_preview=True)
