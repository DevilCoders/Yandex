import logging
import os
from time import sleep

from startrek_client import Startrek
from startrek_client.exceptions import Forbidden, NotFound
from telegram.ext import (CommandHandler, ConversationHandler, Filters,
                          MessageHandler)
from telegram.ext.dispatcher import run_async

from app.utils.classes import User
from app.utils.config import Config


class Tasks:
    def __init__(self, updater, dispatcher):
        self.updater = updater
        self.dispatcher = dispatcher
        self.messages = {}
        self.st_client = Startrek(useragent=Config.useragent,
                                  base_url=Config.tracker_endpoint,
                                  token=Config.ya_tracker)
        self.conversation = ConversationHandler(
            entry_points=[MessageHandler(filters=Filters.forwarded,
                                         callback=self.forwards_handler,
                                         pass_user_data=True)],

            states={
                'step-1-basics': [MessageHandler(filters=Filters.forwarded,
                                                 callback=self.forwards_handler,
                                                 pass_user_data=True),
                                  CommandHandler('ticket', self.option_queue_name_handler,
                                                 pass_user_data=True),
                                  CommandHandler('comment', self.input_ticket_name_handler,
                                                 pass_args=True)],

                'step-2-ticket': [MessageHandler(filters=Filters.text,
                                                 callback=self.option_ticket_name_handler,
                                                 pass_user_data=True)],

                'step-2-comment': [MessageHandler(filters=Filters.text,
                                                  callback=self.add_comment)],

                'step-3-ticket': [MessageHandler(filters=Filters.text,
                                                 callback=self.create_ticket,
                                                 pass_user_data=True)]
            },

            fallbacks=[CommandHandler('cancel', self.cancel_command)])
        self.dispatcher.add_handler(self.conversation)

    @run_async
    def cancel_command(self, bot, update):
        text = "Отмена! Отмена! Отмена!"
        bot.send_message(chat_id=update.message.chat_id,
                         text=text)
        for message in self.messages[update.message.from_user["username"]]:
            if message['type'] == 'photo':
                os.remove(message['text'])
        return ConversationHandler.END

    @run_async
    def forwards_handler(self, bot, update, user_data):
        # Disable in chats
        if str(update.message.chat_id).startswith('-'):
            return ConversationHandler.END

        # Init the man who forwarded messages, messages author and messages
        # text for furthers methods
        user = User(chat_id=update.message.chat_id,
                    login_tg=update.message.from_user["username"])
        if user.allowed is False or user.get_user is False:
            return ConversationHandler.END

        forwarder = update.message.from_user['username']
        author = 'Аноним' if update.message.forward_from is None else update.message.forward_from['username']
        text = update.message.text
        date = update.message.forward_date.strftime("%H:%M, %d %B")
        # Add data from forwarded messages to user_data
        if forwarder in self.messages:
            if update.message.photo:
                pic = update.message.photo[-1]
                pic = pic.file_id
                bot.get_file(pic).download(f"{pic}.jpg")
                self.messages[forwarder].append({
                    'from': forwarder,
                    'author': author,
                    'text': pic,
                    'date': date,
                    'type': 'photo'
                })
            else:
                self.messages[forwarder].append({
                    'from': forwarder,
                    'author': author,
                    'text': text,
                    'date': date,
                    'type': 'text'
                })
        else:
            self.messages[forwarder] = []
            if update.message.photo:
                pic = update.message.photo[-1]
                pic = pic.file_id
                self.messages[forwarder].append({
                    'from': forwarder,
                    'author': author,
                    'text': pic,
                    'date': date,
                    'type': 'photo'
                })
            else:
                self.messages[forwarder].append({
                    'from': forwarder,
                    'author': author,
                    'text': text,
                    'date': date,
                    'type': 'text'
                })
        return 'step-1-basics'

    @run_async
    def option_queue_name_handler(self, bot, update, user_data):
        logging.info('[forward_tickets.new-ticket] Messages fetched, creating ticket')
        text = 'Введи название очереди, в которой хочешь создать тикет'
        bot.send_message(chat_id=update.message.chat_id, text=text)
        return 'step-2-ticket'

    @run_async
    def option_ticket_name_handler(self, bot, update, user_data):
        username = update.message.chat.username
        user_data[f'{username}-queue'] = str(update.message.text).upper()
        bot.send_message(chat_id=update.message.chat_id,
                         text='Введи название тикета')
        return 'step-3-ticket'

    @run_async
    def input_ticket_name_handler(self, bot, update, args):
        if args:
            self.messages['mention'] = args
        logging.info('[forward_tickets.add_comment] Messages fetched, adding comment to ticket')
        text = 'Введи ключ тикета, в котором нужно добавить комментарий\n' \
               'Например: CLOUD-1234'
        bot.send_message(chat_id=update.message.chat_id,
                         text=text)
        return 'step-2-comment'

    @run_async
    def add_comment(self, bot, update):
        logging.info(f"[forward_tickets.add_comment] {self.messages}")
        username = update.message.chat.username
        user = User(chat_id=update.message.chat_id,
                    login_tg=username)
        # Generating formatted text for ticket.
        text = f'Комментарий добавил staff:{user.login_staff}\n'
        attach_list = []
        for message in self.messages[username]:
            if message['text'] is '':
                logging.info('[forward_tickets.add_comment] Message is empty')
            else:
                if message['type'] == 'text':
                    text += '%%{author} [{date}]:\n{text}%%\n\n'.format(date=message['date'],
                                                                    author=message['author'],
                                                                    text=message['text'])
                elif message['type'] == 'photo':
                    logging.info("[forward_tickets.add_comment] Adding photo")
                    bot.get_file(message['text']).download(f"{message['text']}.jpg")
                    picril = self.st_client.issues[str(update.message.text).upper()].attachments.create(f"{message['text']}.jpg")
                    logging.info(f"[forward_tickets.add_comment] {picril.id}")
                    text += '%%{author} [{date}]:%%\n((https://st.yandex-team.ru/{ticket}/attachments/{attach}? 250x0:https://st.yandex-team.ru/{ticket}/attachments/{attach}))\n\n'.format(
                        author=message['author'], date=message['date'], ticket=str(update.message.text).upper(), attach=picril.id)
                    os.remove(f"{message['text']}.jpg")

        try:
            ticket = str(update.message.text).upper()
            update.message.reply_text(f'Пишу комментарий к тикету {ticket}')
            issue = self.st_client.issues[f'{ticket}']
            if self.messages.get('mention'):
                comment = issue.comments.create(text=text, summonees=self.messages['mention'])
            else:
                comment = issue.comments.create(text=text)
            sleep(1)
            update.message.reply_text(f'Готово! [Комментарий добавлен](https://st.yandex-team.ru/{ticket})',
                                      parse_mode='Markdown')
        except (Forbidden, NotFound) as e:
            update.message.reply_text(f"Такого тикета не существует, либо "
                                      f"у робота нет доступа к очереди. Свяжись с @hexyo")
            logging.info(f"[forward_tickets.merge_data] started by {username} failed with {e}")

        # Clearing 'cache' and exiting conversation
        self.messages = {}
        return ConversationHandler.END

    def get_ticket(self, filter, summary, step):
        if step >= 5:
            return False
        try:
            sleep(2)
            ticket_id = self.st_client.issues.find(filter=filter)
            ticket_id = [issue.key for issue in ticket_id if self.st_client.issues[issue.key].summary == summary]
            return ticket_id[0]
        except (IndexError, NotFound):
            logging.info(f"[forward_tickets.get_ticket] retry {step}: Can't found")
            step += 1
            return self.get_ticket(filter, summary, step)

    def create_ticket(self, bot, update, user_data):
        logging.info(f"[forward_tickets.create_ticket] All data was fetched {user_data}")
        logging.info(f"[forward_tickets.create_ticket] {self.messages}")

        username = update.message.chat.username
        summary = update.message.text

        # Generating formatted text for ticket.
        text = '%%'
        for message in self.messages[username]:
            if message['text'] is '':
                logging.info('[forward_tickets.create_ticket] Message is empty')
                pass
            else:
                text += '{author} [{date}]:\n{text}\n\n'.format(date=message['date'],
                                                                author=message['author'],
                                                                text=message['text'])
        text += '%%'

        # Trying to create ticket
        try:
            user = User(chat_id=update.message.chat_id,
                        login_tg=username)
            update.message.reply_text(f"Создаю тикет...\n")
            update.message.reply_text("Обычно это занимает несколько секунд, подожди пожалуйста :)")
            self.st_client.issues.create(
                queue=user_data[f'{username}-queue'],
                summary=summary,
                description=text,
                author=user.login_staff
            )
            filter = {'queue': user_data[f'{username}-queue'],
                      'author': user.login_staff,
                      'resolution': 'empty()'}

            # This sleep is Startrek time to create ticket
            sleep(2)
            ticket = self.get_ticket(filter, summary, 0)
            if ticket is False:
                logging.info('[forward_tickets.create_ticket] Startrek api didnt response for few steps. Maybe its down')
                update.message.reply_text("Я попытался создать тикет, но не могу найти его.\n"
                                          " Вероятно API Трекера прилегло отдохнуть."
                                          " \nНапиши @hexyo, если есть вопросы.")
            else:
                update.message.reply_text(f"Готово! Тикет [{ticket}]"
                                          f"(https://st.yandex-team.ru/{ticket}) создан",
                                          parse_mode='Markdown')
                logging.info(f"[forward_tickets.create_ticket] user {user.login_staff} created {ticket}")

        except (Forbidden, NotFound) as e:
            update.message.reply_text(f"Упс, такой очереди не существует, либо "
                                      f"у робота нет к ней доступа. Свяжись с @hexyo")
            logging.info(f"[forward_tickets.create_ticket] by {username} failed with {e}")

        # Clearing 'cache' and exiting conversation
        self.messages = {}
        return ConversationHandler.END
