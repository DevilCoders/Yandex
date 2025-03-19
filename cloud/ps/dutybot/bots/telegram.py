import datetime
import logging
from typing import Literal, List, Optional

from telegram import Bot, Update, Message, CallbackQuery
from telegram.ext import Updater, CallbackQueryHandler, CommandHandler, MessageHandler
from telegram.ext.jobqueue import Days

from bot_utils.config import Config
from bot_utils.filters import ForwardProtectedHandler, TicketsResolver, FeedbackFilter
from bot_utils.users import User
from bots.abstract import AbstractDutyBot
from messengers.telegram import TelegramMessenger
from models import IncomingMessage, IncomingChat, IncomingCallbackQuery
from security.watchdog import (
    check_telegram_caller_permission,
    check_telegram_admin_privileges,
    check_telegram_cloud_department,
)
from templaters.telegram import TelegramTemplater


class TelegramDutyBot(AbstractDutyBot):
    templater = TelegramTemplater()

    def __init__(self, config: Config, environment: Literal["prod", "preprod", "debug"]):
        super().__init__(config)

        self._environment = environment

        token = self._get_token(environment)
        self.bot_id = token.split(":")[0]
        # Replace updater for context-model with update to 12+ version
        # of python-telegram-bot api
        self.updater = Updater(token=token)
        self.messenger = TelegramMessenger(self.updater.bot)

        self.job_handler = self.updater.job_queue
        self.dispatcher = self.updater.dispatcher
        self.commands_list = [
            # Regular commands
            ForwardProtectedHandler("duty", self.telegram_command_duty, pass_args=True),
            ForwardProtectedHandler("inc", self.telegram_command_inc, pass_args=True),
            CommandHandler("adm_update", self.telegram_command_adm_update),
            CommandHandler("adm_statistics", self.telegram_command_adm_statistics),
            CommandHandler("help", self.telegram_command_help),
            CommandHandler("disable_feedback", self.telegram_command_disable_feedback),
            CommandHandler("enable_feedback", self.telegram_command_enable_feedback),
            CommandHandler(["dt", "dutyticket"], self.telegram_command_duty_ticket, pass_args=True),
            CommandHandler("myduty", self.telegram_command_myduty),
            CommandHandler("agenda", self.telegram_command_get_agenda),
            ForwardProtectedHandler("start", self.telegram_command_start),
            ForwardProtectedHandler("reset", self.telegram_command_reset),
            ForwardProtectedHandler("post", self.telegram_command_post),
            # -----------------------------------------------------------
            # Admin commands
            CommandHandler("adm_send_reminders", self.telegram_command_send_reminders),
            CommandHandler("adm_teamlead_notify", self.telegram_command_teamlead_notify),
            CommandHandler("adm_ask_feedbacks", self.telegram_command_ask_feedbacks),
            CommandHandler("adm_check_incident", self.telegram_command_check_incident),
            CommandHandler("adm_ping_cloudinc_speaker", self.telegram_command_ping_cloudinc_speaker),
            CommandHandler("adm_post_agenda", self.telegram_command_adm_post_agenda),
            CommandHandler("adm_close_cloudinc", self.telegram_command_close_cloudinc),
            # -----------------------------------------------------------
            # Callback queries
            CallbackQueryHandler(self.telegram_callback_quick_reply, pattern=r'^see:.*'),
            CallbackQueryHandler(self.telegram_callback_feedback_score, pattern=r'^feedback:.*'),
            CallbackQueryHandler(self.telegram_callback_activate_chat, pattern=r'^activate:.*'),
            CallbackQueryHandler(self.telegram_callback_im_meeting, pattern=r'^im_meeting:.*'),
            CallbackQueryHandler(self.telegram_callback_close_inc, pattern=r'^close_inc:.*'),
            # -----------------------------------------------------------
            # FeedbackFilter() should be last one
            MessageHandler(filters=TicketsResolver(), callback=self.telegram_command_ticket_resolver),
            MessageHandler(filters=FeedbackFilter(), callback=self.telegram_command_send_feedback_text),
        ]

        self.repeating_jobs_list = {}
        # Run regular jobs only on one instance in group
        if self._config.run_regular_jobs:
            # Incidents are checked only in prod environment
            if environment == "prod":
                self.repeating_jobs_list[self.job_check_incident] = [60, 15]
            self.repeating_jobs_list[self.job_ask_feedbacks] = [60 * 60, 15]
            self.daily_jobs_list = [
                [self.job_send_reminders, datetime.time(18, 0, 0), Days.EVERY_DAY],
                [self.job_notify_team_leads_about_finished_timetable, datetime.time(18, 10, 0), Days.EVERY_DAY],
                [self.job_ping_cloudinc_speaker, datetime.time(12, 0), Config.WORKDAYS],
                [self.job_post_agenda, datetime.time(14, 30), Config.WORKDAYS],
                [self.job_close_cloudinc, datetime.time(16, 10), Config.WORKDAYS],
            ]
        else:
            self.daily_jobs_list = []

        self._setup_command_handlers()
        self._setup_jobs()

    def run(self):
        logging.info("Start telegram bot for environment %s", self._environment)
        if self._config.use_webhook:
            logging.info("Setting webhook at %s", self._config.telegram_webhook_url)
            with open("YandexInternalRootCA.pem", "rb") as certificate_file:
                self.updater.bot.set_webhook(url=self._config.telegram_webhook_url, certificate=certificate_file)
            self.updater.start_webhook(
                listen="0.0.0.0",
                port=8080,
                url_path="bot" + self.bot_id,
                webhook_url=self._config.telegram_webhook_url,
            )
        else:
            self.updater.start_polling()

    def _setup_jobs(self):
        for function, interval in self.repeating_jobs_list.items():
            self.job_handler.run_repeating(function, interval[0], interval[1])

        for job in self.daily_jobs_list:
            self.job_handler.run_daily(job[0], job[1], days=job[2])

    def _setup_command_handlers(self):
        for command in self.commands_list:
            self.dispatcher.add_handler(command)

    def _get_token(self, environment: Literal["prod", "preprod", "debug"]):
        if environment == "prod":
            return self._config.tg_prod
        if environment == "preprod":
            return self._config.tg_preprod
        return self._config.tg_debug

    def _convert_to_incoming_message(self, message: Message, args: Optional[List[str]] = None) -> IncomingMessage:
        caller = message.from_user.username if message.from_user.username else "dutybot"
        chat_title = message.chat.title if message.chat.title else caller
        chat = IncomingChat.telegram(message.chat_id, chat_title)

        return IncomingMessage(
            chat,
            User(chat_id=message.from_user.id, login_tg=message.from_user.username),
            message.message_id,
            message.text,
            message.text_markdown,
            args or [],
            self._convert_to_incoming_message(message.reply_to_message) if message.reply_to_message else None,
        )

    def _convert_to_incoming_callback_query(self, callback_query: CallbackQuery) -> IncomingCallbackQuery:
        chat = IncomingChat.telegram(callback_query.message.chat.id, "")
        return IncomingCallbackQuery(
            chat,
            User(chat_id=callback_query.from_user.id, login_tg=callback_query.from_user.username),
            callback_query.id,
            self._convert_to_incoming_message(callback_query.message),
            callback_query.data,
        )

    @check_telegram_caller_permission
    def telegram_command_duty(self, _: Bot, update: Update, args: List[str]):
        message = self._convert_to_incoming_message(update.message, args)
        self.command_duty(message)

    @check_telegram_admin_privileges
    def telegram_command_adm_update(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_update_cache(message)

    @check_telegram_caller_permission
    def telegram_command_inc(self, _: Bot, update: Update, args: List[str]):
        message = self._convert_to_incoming_message(update.message, args)
        self.command_create_incident(message)
        self.job_check_incident()

    @check_telegram_caller_permission
    def telegram_command_help(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_help(message)

    @check_telegram_caller_permission
    def telegram_command_disable_feedback(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_disable_feedback(message)

    @check_telegram_caller_permission
    def telegram_command_enable_feedback(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_enable_feedback(message)

    @check_telegram_admin_privileges
    def telegram_command_adm_statistics(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_admin_get_statistics(message)

    @check_telegram_caller_permission
    def telegram_command_duty_ticket(self, _: Bot, update: Update, args: List[str]):
        message = self._convert_to_incoming_message(update.message, args)
        self.command_get_duty_ticket(message)

    @check_telegram_caller_permission
    def telegram_command_myduty(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_list_my_duties(message)

    @check_telegram_caller_permission
    def telegram_command_get_agenda(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_get_agenda(message)

    def telegram_command_start(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_start(message)

    @check_telegram_caller_permission
    def telegram_command_reset(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_reset(message)

    @check_telegram_cloud_department
    def telegram_command_post(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_post(message)

    @check_telegram_admin_privileges
    def telegram_command_send_reminders(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_admin_send_reminders(message)

    @check_telegram_admin_privileges
    def telegram_command_teamlead_notify(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_admin_notify_team_leads_about_finished_timetable(message)

    @check_telegram_admin_privileges
    def telegram_command_ask_feedbacks(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_admin_ask_feedbacks(message)

    @check_telegram_admin_privileges
    def telegram_command_ping_cloudinc_speaker(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_admin_ping_cloudinc_speaker(message)

    @check_telegram_admin_privileges
    def telegram_command_adm_post_agenda(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_admin_post_agenda(message)

    @check_telegram_admin_privileges
    def telegram_command_close_cloudinc(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_admin_close_cloudinc(message)

    @check_telegram_admin_privileges
    def telegram_command_check_incident(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_admin_check_incident(message)

    @check_telegram_caller_permission
    def telegram_command_ticket_resolver(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_resolve_support_ticket(message)

    @check_telegram_caller_permission
    def telegram_command_send_feedback_text(self, _: Bot, update: Update):
        message = self._convert_to_incoming_message(update.message)
        self.command_send_feedback_text(message)

    @check_telegram_caller_permission
    def telegram_callback_quick_reply(self, _: Bot, update: Update):
        query = self._convert_to_incoming_callback_query(update.callback_query)
        self.callback_process_summon(query)

    @check_telegram_caller_permission
    def telegram_callback_feedback_score(self, _: Bot, update: Update):
        query = self._convert_to_incoming_callback_query(update.callback_query)
        self.callback_feedback_score(query)

    @check_telegram_caller_permission
    def telegram_callback_activate_chat(self, _: Bot, update: Update):
        query = self._convert_to_incoming_callback_query(update.callback_query)
        self.callback_activate_chat(query)

    @check_telegram_caller_permission
    def telegram_callback_im_meeting(self, _: Bot, update: Update):
        query = self._convert_to_incoming_callback_query(update.callback_query)
        self.callback_im_meeting(query)

    @check_telegram_caller_permission
    def telegram_callback_close_inc(self, _: Bot, update: Update):
        query = self._convert_to_incoming_callback_query(update.callback_query)
        self.callback_close_inc(query)
