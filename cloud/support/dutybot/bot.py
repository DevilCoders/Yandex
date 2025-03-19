import logging
from datetime import datetime, time, timedelta
from time import sleep
import sys

from startrek_client import Startrek
from telegram.error import BadRequest, Unauthorized
from telegram.ext import CommandHandler, Updater, MessageHandler, Filters
from telegram.ext.dispatcher import run_async

import app.database.chats as chats_orm
import app.database.components as components_orm
import app.database.users as users_orm
from app.addons.forward_tickets import Tasks
from app.addons.newform_resolver import TicketsResolver
from app.addons.startrek_updater import StartrekUpdater
from app.database.logs import add_logs
from app.utils.classes import Chat, Duty, Emojis, Reply, User
from app.utils.config import Config
from app.utils.resps import get_duty_ticket, get_oncall


class DutyBot(Tasks,
              StartrekUpdater,
              TicketsResolver):
    def __init__(self, env):
        # Telegram token choose
        if env == "prod":
            self.token = Config.tg_prod
        elif env == "debug":
            self.token = Config.tg_debug

        # Tracker
        self.st_client = Startrek(
            useragent=Config.useragent,
            base_url=Config.tracker_endpoint,
            token=Config.ya_tracker
        )

        # Telegram updater and dispatcher
        self.updater = Updater(token=self.token, workers=8, use_context=False)
        self.job_worker = self.updater.job_queue
        self.dispatcher = self.updater.dispatcher
        self.queue = self.updater.job_queue

        # Custom modules
        Tasks.__init__(self, updater=self.updater,
                       dispatcher=self.dispatcher)

        StartrekUpdater.__init__(self, updater=self.updater,
                                 dispatcher=self.dispatcher,
                                 st_client=self.st_client,
                                 job_worker=self.job_worker)

        TicketsResolver.__init__(self, updater=self.updater,
                                 dispatcher=self.dispatcher,
                                 st_client=self.st_client)

        # Reply and Duty init
        self.reply = Reply()
        self.today_duty = Duty()
        self.incidents = {}

        # Basic handlers init
        commands_list = [
                         CommandHandler("start", self.command_start),
                         CommandHandler("help", self.command_help),
                         CommandHandler("reset", self.command_reset),
                         CommandHandler("me", self.command_me),
                         CommandHandler(["duty", "вген", "дюти"],
                                        self.command_duty,
                                        pass_args=True),
                         CommandHandler("update", self.command_update),
                         CommandHandler("post", self.command_post),
                         CommandHandler("myduty", self.command_myduty),
                         CommandHandler("notify", self.command_notify),
                         CommandHandler(["dutyticket", "dt"],
                                        self.command_duty_ticket,
                                        pass_args=True),
                         CommandHandler("inc", self.command_incident,
                                        pass_args=True)
                         ]

        # Commands startup
        for command in commands_list:
            self.dispatcher.add_handler(command)

        # Regular jobs list
        jobs_list = {
            self.job_update: '900 0',
            self.job_notify: '900 50',
            self.job_support_business: '300 100',
            self.job_incidents_pm: '120 120',
            self.job_abuse: '300 200'
        }

        # Jobs startup
        for worker, interval in jobs_list.items():
            self.job_worker.run_repeating(worker,
                                          int(interval.split(' ')[0]),
                                          int(interval.split(' ')[1])
                                          )

        # Notify oncall about upcoming duty
        self.job_worker.run_daily(self.job_duty_notify, time(18, 0, 0))

        # Rotation
        job_time = set([x[2] for x in components_orm.get_all_components()])
        for _time in job_time:
            self.job_worker.run_daily(self.job_update,
                                      time(_time, 0, 0))
            self.job_worker.run_daily(self.job_start_duty,
                                      time(_time, 1, 0))

    def start(self):
        logging.info("[bot.start] Heheh! Let's hear those guns!")
        self.updater.start_polling()

    def command_start(self, bot, update):
        # Command for chats
        if str(update.message.chat_id).startswith('-'):
            user = User(chat_id=update.message.from_user.id,
                        login_tg=update.message.from_user.username)
            chat = Chat(chat_id=update.message.chat.id,
                        name=update.message.chat.title)
            if chat.get_chat is False:
                self.new_chat = f"{chat.chat_id},'{chat.chat_name}','{chat.component}','{chat.admin}'"
                logging.info(f"[bot.command_start] New chat: {self.new_chat}")
                update.message.reply_text("Привет! Бот изначально не работает в чатах," \
                                          " в которые его добавили. Это сделано в целях безопасности.\n" \
                                          "Я уже отправил пуш @hexyo, чтобы он активировал бота в этом чате, но если" \
                                          " это нужно сделать как можно скорее, то ты можешь пингануть его сам.")
                bot.send_message(chat_id="126910519",
                                 text=f"{user.login_tg} добавил бота в новый чат.\n{self.new_chat}")
        # Command for users
        else:
            # If user is not exist -> add to database
            user = User(chat_id=update.message.chat_id, login_tg=update.message.chat.username)
            if user.get_user is not True and user.allowed is True:
                user.add_user()
                bot.send_message(chat_id=user.chat_id,
                                 text=f"Алоха, {user.name_staff.split(' ')[0]}!\n\n"
                                      f"{self.reply.help_reply}", parse_mode="HTML")
            elif user.get_user is True:
                update.message.reply_text(f"Мы уже знакомы!\n\n{self.reply.help_reply}", parse_mode="HTML")

    @run_async
    def command_help(self, bot, update,):
        # This command works only with users
        if str(update.message.chat_id).startswith('-') is False:
            user = User(chat_id=update.message.chat_id,
                        login_tg=update.message.chat.username)
            if user.get_user is True:
                update.message.reply_text(self.reply.help_reply, parse_mode="HTML")
            if user.bot_admin is True:
                update.message.reply_text(self.reply.help_reply_admin, parse_mode="HTML")

    def command_reset(self, bot, update):
        # This command works only with users
        if str(update.message.chat_id).startswith('-') is False:
            user = User(chat_id=update.message.chat_id,
                        login_tg=update.message.chat.username)
            if user.get_user is True:
                user.del_user()
                logging.info(f"[bot.command_reset] Deleted user {user.login_staff}")
                update.message.reply_text(self.reply.ciao_reply)

    def command_me(self, bot, update):
        # Command for users
        if str(update.message.chat_id).startswith('-') is False:
            user = User(chat_id=update.message.chat_id,
                        login_tg=update.message.chat.username)
            if user.get_user is True:
                update.message.reply_text(user.__str__(), parse_mode="HTML")
        # Command for chats
        if str(update.message.chat_id).startswith('-'):
            chat = Chat(chat_id=update.message.chat.id,
                        name=update.message.chat.title)
            if chat.get_chat is True:
                update.message.reply_text(chat.__str__(), parse_mode="HTML")

    def duty_upcoming(self, team, _len):
        if _len > 31:
            _len = 31
        upcoming_duty = self.today_duty.upcoming_duty(team=team, length=_len)
        if upcoming_duty:
            return upcoming_duty
        else:
            return f"Не нашел команду {team}, или не удалось получить ответа от. \n\nОбратись к @hexyo"

    def duty_team(self, input_arg):
        input_arg = input_arg.replace(',', '')
        default = self.today_duty.result_dict.get(input_arg, None)
        if default is None:
            for team in self.today_duty.result_dict.keys():
                custom_dict = self.today_duty.duty_teams.get(team, [team])
                if custom_dict != [team]:
                    custom_dict = custom_dict.get("custom_dict")
                    custom_dict = custom_dict.split(",")
                    custom_dict.append(team)
                for alias in custom_dict:
                    if input_arg in alias:
                        return team
            return None
        else:
            return input_arg

    @run_async
    def command_duty(self, bot, update, args):
        # Command for users
        if str(update.message.chat_id).startswith('-') is False:
            user = User(chat_id=update.message.chat_id,
                        login_tg=update.message.chat.username)
            if user.get_user is True:
                if not args:
                    update.message.reply_text(self.today_duty.result,
                                              parse_mode="HTML",
                                              disable_web_page_preview=True)
                    update.message.reply_text(self.today_duty.kostyl,
                                              parse_mode="HTML",
                                              disable_web_page_preview=True)
                else:
                    if len(args) == 2:
                        logging.info(f"[bot.command_duty] {user.login_staff} checking upcoming {args[0]} duty for {args[1]} days")
                        reply = self.duty_upcoming(team=args[0].lower(), _len=int(args[1]))
                        update.message.reply_text(reply, parse_mode="HTML")
                    else:
                        # Temporary hardcode for mdb
                        # Expect, that everyone will forget soon
                        # About /duty mdb command
                        # TTL Until Q5 2020
                        if args[0] == 'network':
                            update.message.reply_text(text="Нет такой команды! Используй /duty vpc")
                            return
                        if args[0] == 'mdb':
                            update.message.reply_text(text="Команда MDB разделилась на несколько групп и теперь призывать дежурных следует именно по ним." \
                            "\nПосмотреть все команды можно в стандартной выдаче команды duty, а прочитать об изменениях дежурств MDB тут — https://clubs.at.yandex-team.ru/mdb/468")
                            return
                        if args[0] == 'clickhouse' or args[0] == 'ch':
                            target_team = self.duty_team('mdb-clickhouse')
                        target_team = self.duty_team(args[0].lower())
                        if target_team is None:
                            return
                        logging.info(f"[bot.command_duty] {target_team} called")
                        add_logs(**{"date": datetime.now(),
                                    "team": target_team,
                                    "prim": self.today_duty.raw.get(target_team).get('primary').get('staff'),
                                    "back": self.today_duty.raw.get(target_team).get('backup').get('staff')})
                        reply = self.today_duty.result_dict.get(target_team)
                        update.message.reply_text(reply,
                                                  parse_mode="HTML",
                                                  disable_web_page_preview=True)
        # Command for chats
        else:
            user = User(chat_id=update.message.from_user.id,
                        login_tg=update.message.from_user.username)
            chat = Chat(chat_id=update.message.chat.id,
                        name=update.message.chat.title)
            if chat.get_chat is True:
                if args:
                    # Temporary hardcode for mdb
                    # Expect, that everyone will forget soon
                    # About /duty mdb command
                    # TTL Until Q5 2020
                    if args[0] == 'network':
                        update.message.reply_text(text="Нет такой команды! Используй /duty vpc")
                        return
                    if args[0] == 'mdb':
                        update.message.reply_text(text="Команда MDB разделилась на несколько групп и теперь призывать дежурных следует именно по ним." \
                        "\nПосмотреть все команды можно в стандартной выдаче команды duty, а прочитать об изменениях дежурств MDB тут — https://clubs.at.yandex-team.ru/mdb/468")
                        return
                    target_team = self.duty_team(args[0].lower())
                    if target_team is None:
                        return
                    logging.info(f"[bot.command_duty] {target_team} called")
                    add_logs(**{"date": datetime.now(),
                                "team": target_team,
                                "prim": self.today_duty.raw.get(target_team).get('primary').get('staff'),
                                "back": self.today_duty.raw.get(target_team).get('backup').get('staff')})
                    reply = self.today_duty.result_dict.get(target_team)
                    update.message.reply_text(reply,
                                              parse_mode="HTML",
                                              disable_web_page_preview=True)
                else:
                    commands = ", ".join(sorted([key for key in self.today_duty.get_duty_teams().keys()]))
                    text = f"*Доступные для призыва команды:*\n```{commands}```\n___Выбери необходимую и напиши /duty команда, чтобы призвать дежурного сервиса___"
                    update.message.reply_text(text=text,
                                              parse_mode="Markdown",
                                              disable_web_page_preview=True)
            else:
                logging.info("[bot.command_duty] Chat wasnt activated")

    @run_async
    def command_duty_ticket(self, bot, update, args):
        if len(args) == 0:
            update.message.reply_text(text="Команду нельзя использовать без указания компоненты.\nПример использования: /dt compute")
            return
        user = User(chat_id=update.message.chat_id,
                    login_tg=update.message.chat.username)
        logging.info(f"[bot.command_duty_ticket] {user.login_staff} searching for {args[0]} ticket")
        if user.get_user is True:
            duty_ticket = get_duty_ticket(team=args[0])
            logging.info(f"[bot.command_duty_ticket] Resolved {duty_ticket}")
            if duty_ticket is not None:
                update.message.reply_text(text=f"Актуальный тикет по дежурству для команды {args[0]}\n<a href='http://st.yandex-team.ru/{duty_ticket}'>{duty_ticket}</a>",
                                          parse_mode="HTML")
            else:
                update.message.reply_text(text=f"У команды {args[0]} отсуствует актуальный тикет в очереди CLOUDDUTY")

    @run_async
    def command_update(self, bot, update):
        user = User(chat_id=update.message.chat_id,
                    login_tg=update.message.chat.username)
        logging.info(f"[bot.command_update] {user.login_staff} tried to use update")
        if user.bot_admin is True:
            update.message.reply_text(f"Duty update job started")
            self.job_update(bot, update)

    @run_async
    def command_post(self, bot, update):
        # Posting to yc-announcements
        if str(update.message.chat_id).startswith('-') is False:
            user = User(chat_id=update.message.chat_id,
                        login_tg=update.message.chat.username)
            if user.get_user is True and user.cloud is True:
                if update.message.text[5:] == '':
                    update.message.reply_text(text="❌ Не могу отправить пустое сообщение.\n\nПример использования команды:\n/post тут_сообщение_для_коллег")
                    logging.info(f"[bot.command_post] {user.login_tg} tried to send empty message with announcements_command()")
                    return
                announcement = f"{update.message.text_markdown[5:]}\n\nот [{user.login_staff}](http://t.me/{user.login_tg})"
                bot.send_message(chat_id="-1001191533225", text=announcement, parse_mode="Markdown", disable_web_page_preview=True)
                logging.info(f"[bot.command_post] {user.login_tg} sent message to yc_announcements.")
            elif user.cloud is False:
                update.message.reply_text(text=("У тебя нет доступа к данной команде. По умолчанию она доступна только для"
                                                "сотрудников департамента Яндекс Облако. Напиши @hexyo, если тебе нужен доступ к команде."))

    @run_async
    def command_notify(self, bot, update):
        user = User(chat_id=update.message.chat_id,
                    login_tg=update.message.chat.username)
        if user.bot_admin is True:
            user_list = users_orm.get_all_users(allowed=True)
            text = update.message.text[8:]
            update.message.reply_text(f"Sending message [{text}] to {len(user_list)} users")
            for bot_user in user_list:
                sleep(0.3)
                try:
                    bot.send_message(chat_id=bot_user[0],
                                     text=text)
                except BadRequest:
                    logging.info(f"[bot.command_notify] Can't send message to {bot_user[1]}, bot banned.")

    @run_async
    def command_myduty(self, bot, update):
        if str(update.message.chat_id).startswith('-') is False:
            user = User(chat_id=update.message.chat_id,
                        login_tg=update.message.chat.username)
            if user.get_user is True:
                schedule = user.get_schedule()[:5]
                logging.info(f"[bot.command_myduty] {user.login_staff} got {bool(schedule)}")
                exist = f"<b>Привет, {user.login_staff}!\nТы дежуришь:</b>\n"
                not_exist = f"Привет, {user.name_staff.split(' ')[0]}.\nБлижайших дежурств не найдено\
                    \nПроверить актуальное расписание можно в <a href='https://bb.yandex-team.ru/projects/CLOUD/repos/resps/browse/cvs'>BitBucket</a>"
                output = exist if bool(schedule) else not_exist
                for shift in schedule:
                    output += f"\nВ команде <b>{shift[0].capitalize()}</b> как <i>{shift[3]}</i>-дежурный:"
                    output += f"\n<i>C {shift[1]} до {shift[2]}</i>\n"
                update.message.reply_text(output, parse_mode="HTML")

    def command_incident(self, bot, update, args):
        if str(update.message.chat_id).startswith('-'):
            user = User(chat_id=update.message.from_user.id,
                        login_tg=update.message.from_user.username)
            if user.get_user is not True:
                return
        else:
            user = User(chat_id=update.message.chat_id,
                        login_tg=update.message.chat.username)
            chat = Chat(chat_id=update.message.chat.id,
                        name=update.message.chat.title)
            if chat.get_chat is not True:
                return

        if not args:
            update.message.reply_text(("Команда не работает без аргументов.\n"
                                       "Компоненты идентичны тем, что используются в выдаче бота.\n\n"
                                       "Пример использования: /inc компонента название_тикета"
                                       ))
            return
        if len(args) < 2:
            update.message.reply_text(("Необходимо указать заголовок тикета\n\n"
                                       "Пример использования: /inc компонента название_тикета"
                                       ))
            return
        team = self.duty_team(args[0])
        if team is None:
            update.message.reply_text((f"Не могу найти команду {args[0]}.\n"
                                       "Компоненты идентичны тем, что используются в выдаче бота."))
            return
        logging.info(f"[command_inicdent] {user.login_staff} creating ticket for {team} with {self.today_duty.duty_teams[team].get('map_components')}")
        ticket = self.st_client.issues.create(
                queue="CLOUDINC",
                summary=f"[{self.today_duty.duty_teams[team].get('map_components')}] {' '.join(args[1:])}",
                description=self.reply.incidents['blank_ticket'],
                author=user.login_staff,
                assignee=self.today_duty.raw.get(team).get('primary').get('staff'),
                components=[self.today_duty.duty_teams[team].get('map_components')],
                priority='normal'
            )
        update.message.reply_text(f"Завел инцидент st.yandex-team.ru/{ticket.key}\nИсполнитель: @{self.today_duty.raw.get(team).get('primary').get('tg')}")
        self.command_duty(bot, update, ['support'])
        self.command_duty(bot, update, ['incmanager'])

    def job_notify(self, bot, update):
        counter = 0
        logging.info("[bot.job_notify] Starting notify handler")
        # Get all supports from database
        supports = users_orm.get_all_users(support=True)
        # Statrek filters
        cloudsupport_filter = {
                'queue': 'CLOUDSUPPORT',
                'status': 'open'
                }
        ycloud_filter = {
                'queue': 'YCLOUD',
                'status': 'new',
                'assignee': 'empty()'
                }
        # Get YCLOUD'n'CLOUDSUPPORT tickets list
        issues = self.st_client.issues.find(
            filter=cloudsupport_filter
            )
        cloudsupport_tickets = sorted([[int(issue.key[13::]), issue.summary, issue.sla] for issue in issues if issue.pay != 'business'])
        business_tickets = sorted([[int(issue.key[13::]), issue.summary, issue.companyName, issue.sla] for issue in issues if issue.pay == 'business' and issue.type.name != 'incident' and issue.type.name != 'problem'])
        issues = self.st_client.issues.find(
            filter=ycloud_filter)
        ycloud_tickets = sorted([[int(issue.key[7::]), issue.summary] for issue in issues])
        # Generating message and check who send to
        if len(cloudsupport_tickets) != 0 or len(ycloud_tickets) != 0:
            # Generate message
            reply = f"<b>[{len(cloudsupport_tickets)+len(ycloud_tickets)+len(business_tickets)}] {'Открыт' if len(cloudsupport_tickets)+len(ycloud_tickets)==1 else 'Открыто'}</b>\n"
            # Cloudsupport part of message
            if len(business_tickets) != 0:
                reply += f"\n{Emojis.HOG} <b>Бизнес:</b>"
            for ticket in business_tickets:
                sla = self.generate_sla(ticket[3], 'business')
                reply += f"\n<b>{sla}</b> <a href='https://st.yandex-team.ru/CLOUDSUPPORT-{ticket[0]}'>Тикет от {ticket[2]}</a>"
            if len(cloudsupport_tickets) != 0:
                reply += f"\n{Emojis.MAIL} <b>Обращения:</b>"
            for ticket in cloudsupport_tickets:
                sla = self.generate_sla(ticket[2], 'standard')
                reply += f"\n<b>{sla}</b> <a href='https://st.yandex-team.ru/CLOUDSUPPORT-{ticket[0]}'>{ticket[1][:35]}...</a>"
            # Ycloud part of message
            if len(ycloud_tickets) != 0:
                reply += f"\n{Emojis.TOOLS} <b>Внутренние задачи:</b>"
            for ticket in ycloud_tickets:
                reply += f"\n<a href='https://st.yandex-team.ru/YCLOUD-{ticket[0]}'>{ticket[1][:35]}...</a>"
            # Send to supports
            for user in supports:
                # Check day of week, if its weekend go True
                user = User(chat_id=user)
                weekend = False if ((int(datetime.now().weekday())) >= 0 and (int(datetime.now().weekday())) < 5) else True
                # If its user workday now go True
                hour = int(datetime.now().hour)
                workday_start, workday_end = user.work_day.split(' ')
                user_workday = bool(hour >= int(workday_start) and hour < int(workday_end))
                # It isnt weekend
                if weekend is False and user_workday is True:
                    try:
                        bot.send_message(chat_id=user.chat_id,
                                         text=reply,
                                         parse_mode="HTML",
                                         disable_web_page_preview=True)
                        counter += 1
                    except (BadRequest, Unauthorized):
                        logging.info(f"[bot.job_notify] Can't send notify to user {user.login_staff}")
            # Send to oncall
            try:
                user = User(login_staff=self.today_duty.raw.get('support').get('primary').get('staff'))
                bot.send_message(chat_id=user.chat_id,
                                 text=reply,
                                 parse_mode="HTML",
                                 disable_web_page_preview=True)
                counter += 1
            except (BadRequest, Unauthorized):
                logging.info(f"[bot.job_notify] Can't send notify to user {user.login_staff}")
            logging.info(f"[bot.job_notify] Notify sent to {counter} users")
        else:
            logging.info("[bot.job_notify] No new tickets in YCLOUD or CLOUDSUPPORT, go sleep.")

    def generate_sla(self, sla, sup_type):
        for setting in sla:
            if sup_type == 'standard':
                if setting.get('settingsId') == 381:
                    fail_at = datetime.strptime((datetime.strptime(setting.get('failAt'), "%Y-%m-%dT%H:%M:%S.%f%z")
                                                + timedelta(hours=3)).strftime("%Y-%m-%dT%H:%M:%S.%f"), "%Y-%m-%dT%H:%M:%S.%f")
                    now = datetime.now()
                    time_to_fail_sla = (fail_at-now).total_seconds()
                    hours = int(time_to_fail_sla // 3600)
                    mins = int((time_to_fail_sla % 3600) // 60)
            else:
                if setting.get('settingsId') == 913:
                    fail_at = datetime.strptime((datetime.strptime(setting.get('failAt'), "%Y-%m-%dT%H:%M:%S.%f%z")
                                                + timedelta(hours=3)).strftime("%Y-%m-%dT%H:%M:%S.%f"), "%Y-%m-%dT%H:%M:%S.%f")
                    now = datetime.now()
                    time_to_fail_sla = (fail_at-now).total_seconds()
                    hours = int(time_to_fail_sla // 3600)
                    mins = int((time_to_fail_sla % 3600) // 60)
        if hours == 0:
            output = f"00:{mins:02}"
        else:
            output = f"{hours:02}:{mins:02}"
        return output

    @run_async
    def job_duty_notify(self, bot, update):
        # run daily at selected time, notify about upcoming duty
        logging.info("[bot.job_duty_notify] started")
        duty_teams = self.today_duty.get_duty_teams()
        for team in duty_teams:
            if team != "s3":
                logging.info(f"Team: {team}")
                date = datetime.strftime(datetime.today() + timedelta(1), "%d-%m-%Y")
                try:
                    oncall = get_oncall(team, date)
                    primary = User(login_staff=oncall.get('primary'))
                    backup = User(login_staff=oncall.get('backup'))
                    logging.info(f"Sending message to primary: {primary.chat_id}(@{primary.login_tg})")
                    logging.info(f"Sending message to backup: {backup.chat_id}(@{backup.login_tg})")
                    if self.today_duty.raw.get(team).get('primary').get('staff') != oncall['primary'] or primary.duty_notify is False:
                        bot.send_message(chat_id=primary.chat_id, text=f"Привет, {primary.login_staff}!\nНапоминаю, что <b>завтра ты primary-дежурный</b> в {team}.\nВместе с тобой будет дежурить {backup.login_staff}", parse_mode="HTML")
                    if self.today_duty.raw.get(team).get('backup').get('staff') != oncall['backup'] or backup.duty_notify is False:
                        bot.send_message(chat_id=backup.chat_id, text=f"Привет, {backup.login_staff}!\nНапоминаю, что <b>завтра ты backup-дежурный</b> в {team}.\nВместе с тобой будет дежурить {primary.login_staff}", parse_mode="HTML")
                except (BadRequest, Unauthorized, AttributeError, TypeError) as e:
                    logging.info(f"[bot.job_duty_notify] {e}")

    @run_async
    def job_update(self, bot, update):
        self.reply.update()
        self.today_duty.update_duty()

    @run_async
    def job_abuse(self, bot, update):
        logging.info(f"[bot.job_abuse] Starting abuse handler")
        users = users_orm.get_all_users(abuse=True)
        abuse_filter = {
                'queue': 'CLOUDABUSE',
                'status': 'open',
                'assignee': 'empty()'
                }
        issues = self.st_client.issues.find(filter=abuse_filter)
        abuse_tickets = [issue.key for issue in issues]
        if len(abuse_tickets) > 0:
            weekend = False if ((int(datetime.now().weekday())) >= 0 and (int(datetime.now().weekday())) < 5) else True
            hour = int(datetime.now().hour)
            reply = f'<b>[{len(abuse_tickets)}] Открыто абьюзов:</b>\n\n'
            logging.info(f"[bot.job_abuse] {len(abuse_tickets)} tickets, time {hour}, weekend {weekend}")
            for ticket in abuse_tickets:
                reply += f"<a href='https://st.yandex-team.ru/{ticket}'>{ticket}</a>\n"
            # Send to all users with abuse flag set
            for user in users:
                user = User(chat_id=user)
                workday_start, workday_end = user.work_day.split(' ')
                user_workday = bool(hour >= int(workday_start) and hour < int(workday_end))
                if weekend is False and user_workday is True:
                    try:
                        bot.send_message(chat_id=user.chat_id,
                                         text=reply,
                                         parse_mode="HTML",
                                         disable_web_page_preview=True)
                        logging.info(f"[job_abuse] sent to {user.login_staff}")
                    except (BadRequest, Unauthorized):
                        logging.info(f"[job_abuse] Can't send notify to user {user.login_staff}")
            # Send to oncall
            try:
                user = User(login_staff=self.today_duty.raw.get('support').get('primary').get('staff'))
                bot.send_message(chat_id=user.chat_id,
                                 text=reply,
                                 parse_mode="HTML",
                                 disable_web_page_preview=True)
                logging.info(f"[job_abuse] sent to {user.login_staff}")
            except (BadRequest, Unauthorized):
                logging.info(f"[job_abuse] Can't send notify to user {user.login_staff}")

    @run_async
    def job_incidents_pm(self, bot, update):
        logging.info("[bot.job_incidents_pm] Started")
        _incs = {"queue": "CLOUDINC",
                 "status": ["New", "In Progress"]}
        chats = {}

        # Grouping all chats by component
        for chat in chats_orm.get_all_chats(incidents=True):
            tags = chat[1].split(',')
            for tag in tags:
                if chats.get(tag) is None:
                    chats[tag] = [chat[0]]
                else:
                    chats[tag].append(chat[0])

        priority_types = Config.priority_types_startrek
        issues = self.st_client.issues.find(filter=_incs)

        result = [{"createdAt": datetime.strptime((datetime.strptime(x.createdAt, "%Y-%m-%dT%H:%M:%S.%f%z")
                                                  + timedelta(hours=3)).strftime("%Y-%m-%dT%H:%M:%S.%f"), "%Y-%m-%dT%H:%M:%S.%f"),
                   "key": x.key,
                   "priority": priority_types.get(x.priority.id),
                   "assignee": x.assignee.id if x.assignee is not None else "не указан",
                   "title": x.summary,
                   "component": [component.name.lower() for component in x.components]} for x in issues]

        for ticket in result:
            now = datetime.now()
            diff = (now - ticket.get('createdAt')).total_seconds()
            target = []
            for component in ticket.get('component'):
                if chats.get(component) is not None:
                    for chat in chats.get(component):
                        target.append(chat)

            for chat in chats.get('all'):
                target.append(chat)

            if ticket.get('priority') == 'критический':
                for chat in chats.get('critical'):
                    target.append(chat)

            # Notify only if ticket was created last 300 secs
            if diff < 120:
                logging.info(f"[bot.job_incidents_pm] {ticket.get('key')} {diff} mins from creation")
                assignee = User(login_staff=ticket.get('assignee'))
                if ticket.get('component'):
                    components = "<b>Компоненты</b>: " + ", ".join(ticket.get("component")).capitalize()
                else:
                    components = "<b>Компоненты</b>: не указаны :("
                text = self.reply.incidents['new_ticket'].format(ticket=ticket.get('key'),
                                                                 title=ticket.get('title'),
                                                                 assignee_staff=assignee.login_staff,
                                                                 assignee_name=assignee.name_staff,
                                                                 assignee_tg=assignee.login_tg,
                                                                 components=components,
                                                                 priority=ticket.get("priority").capitalize())
                inc_manager = User(login_staff=self.today_duty.raw.get('incmanager').get('primary').get('staff'))
                user_list = users_orm.get_all_users(incidents=True)
                for user in user_list:
                    bot.send_message(chat_id=user,
                                     text=text,
                                     parse_mode="HTML",
                                     disable_web_page_preview=True)
                if inc_manager.chat_id not in user_list:
                    bot.send_message(chat_id=inc_manager.chat_id,
                                     text=text,
                                     parse_mode="HTML",
                                     disable_web_page_preview=True)
                for chat in target:
                    bot.send_message(chat_id=chat,
                                     text=text,
                                     parse_mode="HTML",
                                     disable_web_page_preview=True)

        logging.info("[bot.job_incidents_pm] Finished")

    @run_async
    def job_start_duty(self, bot, update):
        logging.info(f"[bot.job_start_duty] Job started by schedule at {datetime.now().hour}")
        for team, config in self.today_duty.duty_teams.items():
            try:
                if config.get('custom_notify') is True:
                    current_time = int(datetime.now().hour)
                    if current_time == int(config.get('rotate_time')):
                        primary = User(login_staff=self.today_duty.raw.get(team).get('primary').get('staff'))
                        bot.send_message(chat_id=primary.chat_id,
                                         text=self.reply.custom_duty_start.get(team),
                                         parse_mode='HTML',
                                         disable_web_page_preview=True)
                        logging.info(f"[bot.job_start_duty] Start-duty message sent to {primary.login_staff} ({team})")
            except Exception as e:
                logging.info(f"[bot.job_start_duty] Failed with {e}")

    @run_async
    def job_support_business(self, bot, update):
        logging.info(f"[bot.job_support_business] Starting business handler")
        users = users_orm.get_all_users(support=True)
        business_filter = {
                'queue': 'CLOUDSUPPORT',
                'status': 'open',
                'pay': ['business', 'premium']
                }
        issues = self.st_client.issues.find(filter=business_filter)
        business_tickets = [issue.key for issue in issues if issue.type.key == 'problem'
                            or issue.type.key == 'incident']
        if len(business_tickets) > 0:
            weekend = False if ((int(datetime.now().weekday())) >= 0 and (int(datetime.now().weekday())) < 5) else True
            hour = int(datetime.now().hour)
            reply = f'<b>{Emojis.DYNOMITE} 30 мин SLA:</b>\n\n'
            logging.info(f"[bot.job_support_business] {len(business_tickets)} tickets, time {hour}, weekend {weekend}")
            for ticket in business_tickets:
                reply += f"<a href='https://st.yandex-team.ru/{ticket}'>{ticket}</a>\n"
            # Send to all users with support flag set
            for user in users:
                user = User(chat_id=user)
                workday_start, workday_end = user.work_day.split(' ')
                user_workday = bool(hour >= int(workday_start) and hour < int(workday_end))
                if weekend is False and user_workday is True:
                    try:
                        bot.send_message(chat_id=user.chat_id,
                                         text=reply,
                                         parse_mode="HTML",
                                         disable_web_page_preview=True)
                        logging.info(f"[bot.job_support_business] sent to {user.login_staff}")
                    except (BadRequest, Unauthorized):
                        logging.info(f"[bot.job_support_business] Can't send notify to user {user.login_staff}")
            # Send to oncall
            try:
                user = User(login_staff=self.today_duty.raw.get('support').get('primary').get('staff'))
                bot.send_message(chat_id=user.chat_id,
                                 text=reply,
                                 parse_mode="HTML",
                                 disable_web_page_preview=True)
                logging.info(f"[bot.job_support_business] sent to {user.login_staff}")
            except (BadRequest, Unauthorized):
                logging.info(f"[bot.job_support_business] Can't send notify to user {user.login_staff}")

    @run_async
    def command_polling(self, bot, update):
        output = {}
        if str(update.message.chat_id)[0] != "-":
            return
        output['text'] = update.message.text
        output['sender'] = {"telegram_login": update.message.from_user.username,
                            "chat_name": update.message.chat.title,
                            "chat_id": update.message.chat.id}
        output['date'] = str(datetime.now())
        logging.info(f"{output}")


def main():
    logging.basicConfig(level=logging.INFO,
                        format="[%(asctime)s] %(message)s",
                        datefmt='%D %H:%M:%S')
    logger = logging.getLogger('main')
    Gnome_Clockwork = DutyBot("prod")
    Gnome_Clockwork.start()


if __name__ == "__main__":
    main()
