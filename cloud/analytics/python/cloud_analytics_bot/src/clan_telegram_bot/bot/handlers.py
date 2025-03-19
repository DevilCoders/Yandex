# -*- coding: utf-8 -*-
from io import BytesIO, StringIO
import logging.config
import os
import re
import sys
import gzip
from sqlalchemy.orm import composite
import telegram
import pandas as pd
from telegram.error import BadRequest
from telegram.ext import Updater
from clan_telegram_bot.db.CLANDbAdapter import TimeAllocation
from datetime import datetime, timedelta
from clan_telegram_bot.ui.UI import Keyboards, InlineKeyboards
from clan_tools.utils.time import datetime2utcms, utcms2datetime, utcms2tzdatetime
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.TrackerAdapter import TrackerAdapter
import re
logger = logging.getLogger(__name__)


class CLANUpdater(Updater):
    def __init__(self, bot_conf, bot_db_adapter):
        super(CLANUpdater, self).__init__(token=bot_conf['TOKEN'])
        self.bot_db_adapter = bot_db_adapter

    def start_polling(self, *args):
        start_msg = "CLAN bot started."
        # subscribed_users = self.bot_db_adapter.get_subscribed_users()
        # for user in subscribed_users:
        #     try:
        #         self.bot.send_message(chat_id=user.id, text=start_msg)
        #     except BadRequest as e:
        #         logger.error(e)
        #     logger.info('Start message was sent to user {}(id={})'.format(user.name, user.id))
        super(CLANUpdater, self).start_polling(*args)

    def signal_term_handler(self, signum, frame):
        kill_msg = 'CLAN telegram bot was stopped!'
        logger.info(kill_msg)
        # subscribed_users = self.bot_db_adapter.get_subscribed_users()
        # for user in subscribed_users:
        #     self.bot.send_message(chat_id=user.id, text=kill_msg)
        #     logger.info('Kill message was sent to user {}(id={})'.format(user.name, user.id))
        # self.bot_db_adapter.Session.remove()
        sys.exit(0)

    def signal_handler(self, signum, frame):
        self.is_idle = False
        if self.running:
            self.stop()
            self.signal_term_handler(signum, frame)
        else:
            self.logger.warning('Exiting immediately!')
            os._exit(1)


class Handlers:
    EVENTS_JOB_FREQ = 60.

    def __init__(self, bot_db_adapter, developers_ids, tracker_adapter:TrackerAdapter):
        self._bot_db_adapter = bot_db_adapter
        self._developers_ids = developers_ids
        self._tracker_adapter = tracker_adapter


    def subscribe_events_callback(self, bot, job):
        current_time = datetime.now()
        if current_time.hour == 19 and current_time.minute == 0 and (current_time.weekday() not in (5, 6)):
            try:
                subscribed_users = self._bot_db_adapter.get_subscribed_users()
                for user in subscribed_users:
                    logger.info('User {} handling'.format(user.name))
                    message = "Please, check yours time allocation for today."

                    bot.send_message(chat_id=user.id, text=message)

            except Exception as e:
                logger.error(e)
                for id in self._developers_ids:
                    bot.send_message(chat_id=id, text=str(e))


   

    @staticmethod
    def not_subscribed(cons):
        cons.update.message.reply_text(text='Sorry you have no access.')



    @staticmethod
    def show_main_menu(cons):
        cons.update.message.reply_text(text="Main menu. Please, choose an option ",
                                       reply_markup=Keyboards.MAIN)

    @staticmethod
    def add_project_input(cons):
        cons.update.message.reply_text(text='Input project name', reply_markup=Keyboards.BACK_TO_MAIN)

    @staticmethod
    def add_project_name(cons):
        project_name = str(cons.update.message.text)
        cons.db_adapter.add_project(project_name=project_name)
        cons.update.message.reply_text(text=f'Project {project_name} is added')
  

    
    def get_project_input(self, cons):
        cons.update.message.reply_text(text='Searching for your issues in tracker. It will take a few seconds.')

        projects = cons.db_adapter.get_projects()
        user_id = cons.update.effective_user.id
        user_map = {321067378: {'user':'bakuteev', 'queue': 'CLOUDANA'},
                    111206594: {'user':'artkaz', 'queue': 'CLOUDANA'},
                    116558929: {'user':'lunin-dv', 'queue': 'CLOUDANA'},
                    141793967: {'user':'danilapavlov', 'queue': 'CLOUDANA'},
                    304864316: {'user':'elena-nenova', 'queue': 'CLOUDANA'},
                    314163063: {'user':'soin08', 'queue': 'CLOUDDWH'},
                    359144162: {'user': 'daniilkhar', 'queue': 'CLOUDANA'}
        }
        # date_from = (datetime.now() -timedelta(days=30)).date().isoformat()
        issues = self._tracker_adapter.get_user_issues(user_map[user_id]['user'], 
                    queue=user_map[user_id]['queue'], date_from=None)
        project_names = [f'[{issue.key}]({",".join(issue.components)}) {issue.summary}'  for issue in issues]+[project.name for project in projects]
        cons.update.message.reply_text(text="Select project",
                                       reply_markup=Keyboards.plain_keyboard(project_names))
    
    @staticmethod
    def input_hours(cons):
        project_str = str(cons.update.message.text)
        project_name = None
        components = None
        try:
            project_name = re.match("^\[(.*)\]", project_str).group(1)
            components = re.search("\((.*)\)", project_str).group(1)
            if components == '':
                components = None
        except AttributeError:
            pass
        if (project_name is None) and (components is None):
            project_name = project_str
        user_id = cons.update.effective_user.id
        cons.db_adapter.update_selected_project(user_id=user_id, project_name=project_name, project_components=components)
        cons.update.message.reply_text(text=f'How many hours did you work on "{project_name}"?',
                                       reply_markup=Keyboards.HOURS)

    @staticmethod
    def commit_hours(cons):
        hours = str(cons.update.message.text)
        current_user = cons.db_adapter.get_user(eff_user=cons.update.effective_user)
        cons.db_adapter.add_time_allocation(user_id=current_user.id, 
                                           project_name=current_user.selected_project,
                                           components=current_user.selected_project_components,
                                           hours=hours)
                                    
    @staticmethod
    def get_stats(cons):
        hours = str(cons.update.message.text)
        time_allocation = cons.db_adapter.get_time_allocation()
        users = cons.db_adapter.get_users_df()
        yt_adapter = YTAdapter()


        ta_schema = [
            {"name": "id", "type": "int64"},
            {"name": "time", "type": "double"},
            {"name": "user_id", "type": "int64"},
            {"name": "project_name", "type": "string"},
            {"name": "components", "type": "string", "required": False},
            {"name": "hours", "type": "double"},
        ]
        yt_adapter.save_result('//home/cloud_analytics/time_allocation/hours', 
                                df=time_allocation,
                                schema=ta_schema,
                                append=False
                                )


    
        yt_adapter.save_result('//home/cloud_analytics/time_allocation/users', 
                                df=users[['id', 'state', 'first_name', 'last_name']],
                                schema={'id': 'int64', 
                                        'state': 'string', 
                                        'first_name': 'string', 
                                        'last_name':'string' 
                                        # 'name':'string', 
                                        # 'username': 'string',
                                        # 'selected_project':'string',
                                        # 'is_subscribed':'int64'
                                        },
                                append=False
                                )

        time_allocation.to_csv('current_time_allocation.csv')
        # csv_buffer = BytesIO()
        # output_file = time_allocation.to_csv(csv_buffer)
        # b_buf = BytesIO()
        # with gzip.open(b_buf, 'wb') as f:
        #     f.write(time_allocation.to_csv().encode())
        # b_buf.seek(0)

        cons.bot.send_document(chat_id=cons.update.effective_user.id, 
            document=open('current_time_allocation.csv', 'rb'))

 


