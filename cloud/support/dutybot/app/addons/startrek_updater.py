import logging
from datetime import time

from startrek_client.exceptions import NotFound
from telegram.ext import CommandHandler
from telegram.ext.dispatcher import run_async

from app.database.cache import get_cache
from app.utils.classes import User


class StartrekUpdater:
    def __init__(self, updater, dispatcher, st_client, job_worker):
        self.updater = updater
        self.dispatcher = dispatcher
        self.st_client = st_client
        self.job_worker = job_worker
        self.dispatcher.add_handler(CommandHandler('startrek', self.command_startrek))
        self.job_worker.run_daily(self.job_startrek, time(10, 10, 0))
        self.job_worker.run_daily(self.job_startrek, time(22, 30, 0))

    def get_current_support(self):
        # Return list with all supports on duty
        # Oncall-supports ID
        self.supports_list = ['apereshein', 'zpaul', 'akimrx', 'vr-owl']
        logging.info("[startrek_updater.get_current_support] StartrekUpdater is started")
        duty = get_cache(team='support').get('main')
        logging.info(f"[startrek_updater.get_current_support] Support on duty is {duty}")
        self.supports_list.remove(duty)
        return self.supports_list

    def tickets_handler(self, supports_list):
        filter = {'queue': 'CLOUDSUPPORT',
                  'assignee': supports_list,
                  'status': ('In progress', 'Open')
                 }
        result = [(issue.key, issue.assignee.id) for issue in self.st_client.issues.find(filter=filter)]
        counter = 0
        for ticket, login in result:
            try:
                issue = self.st_client.issues[ticket]
                issue.update(assignee={"unset": login})
                counter += 1
            except NotFound:
                pass
        return counter

    @run_async
    def command_startrek(self, bot, update):
        user = User(chat_id=update.message.chat_id,
                    login_tg=update.message.chat.username)
        if user.bot_admin is True:
            supports_list = self.get_current_support()
            start = update.message.reply_text(f"🕑 Убираю исполнителей {', '.join(x for x in supports_list)} из тикетов...")
            handler_result = self.tickets_handler(supports_list)
            bot.edit_message_text(chat_id=user.chat_id,message_id=start.message_id,text=f"✅ Готово! Очистил {handler_result} тикетов")

    @run_async
    def job_startrek(self, bot, update):
        logging.info(f"[startrek_updater.job_startrek] Started")
        supports_list = self.get_current_support()
        handler_result = self.tickets_handler(supports_list)
        logging.info(f"[startrek_updater.job_startrek] Finished, updated {handler_result} tickets")
