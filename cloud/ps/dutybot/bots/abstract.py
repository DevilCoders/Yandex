import abc
import collections
import datetime
import logging
import threading
import time
from typing import Optional, List, Union, Sequence
from textwrap import dedent

import dataclasses
from startrek_client import Startrek
from telegram import InlineKeyboardMarkup, InlineKeyboardButton, ParseMode
from telegram.utils.helpers import escape_markdown as md

from bot_utils.cache import TeamsCache
from bot_utils.chats import Chat
from bot_utils.config import Config
from bot_utils.feedback import Feedback
from bot_utils.stat_collector import DutyStats
from bot_utils.team import Team, FakeTeam
from bot_utils.users import User
from clients.resps import RespsClient
from database.crud import read_query, update_query
from messengers.abstract import AbstractMessenger
from models import IncomingChat, IncomingMessage, Reply, IncomingCallbackQuery, CallbackQueryReply
from templaters.abstract import AbstractTemplater


@dataclasses.dataclass
class ChatSettings:
    id: str
    teams: Optional[List[str]] = None
    duty_noargs_chat: bool = False
    call_backup: bool = False


class AbstractDutyBot:
    UPDATE_CACHE_PAUSE = 60

    messenger: AbstractMessenger
    templater: AbstractTemplater

    def __init__(self, config: Config):
        self._config = config
        self._startrek_client = Startrek(
            useragent=Config.useragent, base_url=Config.tracker_endpoint, token=Config.ya_tracker
        )
        self._resps_client = RespsClient()

        # Populate cache for the first time
        self._cache: Optional[TeamsCache] = None
        self._cache_updating_timestamp = None
        self.update_cache()
        self._start_regular_cache_updating_thread()

        self._chat_settings_by_id = {}
        self._chat_settings_by_name = {}
        for chat_name, chats in self._config.well_known_chats.items():
            for messenger_name, chat_settings_dict in chats.items():
                chat_settings = ChatSettings(**chat_settings_dict)
                self._chat_settings_by_id[messenger_name, chat_settings.id] = chat_settings
                self._chat_settings_by_name[chat_name, messenger_name] = chat_settings

    def update_cache(self):
        logging.info("Cache updating started")
        new_cache = TeamsCache(datetime.datetime.now())
        new_cache.generate_cache(self.templater)
        logging.info("Cache updating finished")

        self._cache = new_cache
        self._cache_updating_timestamp = datetime.datetime.now()

    @abc.abstractmethod
    def run(self):
        raise NotImplementedError(f"{self.__class__.__name__} should implement run()")

    @staticmethod
    def _publish_stats(message: IncomingMessage, team_name: str, team_obj: Team):
        message_text = " ".join(message.args)
        DutyStats(
            caller=message.user.login_staff,
            chat=message.chat.title,
            message=message_text,
            team=team_name,
            primary=team_obj.primary.login_staff,
            backup=team_obj.backup.login_staff,
        ).write_record()

    def _get_chat_settings(self, chat: IncomingChat) -> ChatSettings:
        # If we don't need custom behavior just use "default" cache
        default_settings = self._chat_settings_by_id["all", "default"]
        return self._chat_settings_by_id.get((chat.messenger, chat.chat_id), default_settings)

    def _get_chat_settings_by_name(self, chat_name: str, messenger_name: str) -> ChatSettings:
        if (chat_name, messenger_name) not in self._chat_settings_by_name:
            raise ValueError(f"Chat settings for chat {chat_name} in messenger {messenger_name} not found in config!")
        return self._chat_settings_by_name[chat_name, messenger_name]

    def _find_team_in_chat(self, team_name: str, chat_settings: ChatSettings) -> Optional[Team]:
        found_team_name = self._cache.find_closest_team(team_name)
        logging.info(f"{team_name} matched for {found_team_name}")

        if found_team_name is None:
            return None

        if chat_settings.teams and found_team_name not in chat_settings.teams:
            logging.info(f"But team {found_team_name} is not enabled for chat {chat_settings.id}")
            return None

        return self._cache.get_team_data(found_team_name)

    def _start_regular_cache_updating_thread(self):
        def cache_updating_loop():
            while True:
                time.sleep(self.UPDATE_CACHE_PAUSE)
                try:
                    self.update_cache()
                except Exception as e:
                    logging.exception(f"Can not update cache: {e}, will try later", exc_info=e)

        self._cache_updating_thread = threading.Thread(target=cache_updating_loop)
        self._cache_updating_thread.start()

    def _generate_single_string_output_cache(self, team_names: Optional[List[str]]) -> List[str]:
        if team_names is None:
            team_names = self._cache.team_names

        single_string_outputs = []
        for team_name in team_names:
            team = self._cache.get_team_data(team_name)
            single_string_outputs.append(" ".join(team.single_string_output))

        # Splitting final output for two messages due to symbols limit
        first_part = single_string_outputs[: len(single_string_outputs) // 2]
        second_part = single_string_outputs[len(single_string_outputs) // 2 :]

        # If we have <= 20 teams, don't split them onto two messages
        # telegram message size limit allows sending single message
        if len(first_part) < 20:
            return ["\n".join(single_string_outputs)]

        return [
            "\n".join(first_part),
            "\n".join(second_part),
        ]

    # Regular jobs

    def job_send_reminders(self, _bot=None, _update=None):
        # TODO (andgein): there are teams with shifts less than 24 hours (i.e. support)
        # Here we send reminders only for now+1day oncall.
        tomorrow = datetime.datetime.now() + datetime.timedelta(days=1)
        tomorrow_duty = TeamsCache(tomorrow)
        tomorrow_duty.generate_cache(self.templater, full=False)

        reminders = collections.defaultdict(list)

        for team in tomorrow_duty.team_names:
            try:
                dt_today = self._cache.get_team_data(team)
                dt_tmrw = tomorrow_duty.get_team_data(team)

                if not dt_tmrw.timetable_filled:
                    continue

                if not dt_tmrw.send_reminders:
                    continue

                if dt_today.primary.login_staff != dt_tmrw.primary.login_staff and dt_tmrw.primary.send_reminders:
                    msg = "\n".join(self.templater.generate_duty_reminder("primary", dt_tmrw.backup.login_tg, team))
                    reminders[dt_tmrw.primary].append(msg)

                # GORE-107
                # Some teams have only one oncall-engineer.
                # In these cases we have backup=primary in Team-class.
                # You need this condition to avoid duplicate notifies
                if dt_tmrw.backup.login_staff == dt_tmrw.primary.login_staff:
                    continue

                if dt_today.backup.login_staff != dt_tmrw.backup.login_staff and dt_tmrw.backup.send_reminders:
                    msg = "\n".join(self.templater.generate_duty_reminder("backup", dt_tmrw.primary.login_tg, team))
                    reminders[dt_tmrw.backup].append(msg)
            except Exception as e:
                logging.error(e, exc_info=e)

        for user, user_reminders in reminders.items():
            msg = f"Привет, {user.login_staff}!\n" + "\n".join(user_reminders)
            logging.info(f"Trying to send reminder to {user.login_staff}:\n{msg}")

            self.messenger.send_message(user.chat_id, Reply(msg, format=None))

    def job_notify_team_leads_about_finished_timetable(self, _bot=None, _update=None):
        check_date = datetime.datetime.now() + datetime.timedelta(days=3)
        future_duty = TeamsCache(check_date)
        future_duty.generate_cache(self.templater, full=False)
        for team in future_duty.team_names:
            dt_future = future_duty.get_team_data(team)
            if dt_future.timetable_filled:
                logging.info(f"Timetable for {team} is filled, skip")
                continue
            msg = (
                f"Привет, `{dt_future.teamlead.login_staff}`!\n"
                f"Ты указан(а) как руководитель команды `{team}`"
                f" в [респсе](https://resps-api.cloud.yandex.net/api/v0/services/{team})\n"
                "и поэтому я хочу предупредить тебя, что *у вас заканчивается"
                " расписание меньше, чем через 3 дня*.\nПродли его, пожалуйста."
            )
            self.messenger.send_message(dt_future.teamlead.chat_id, Reply(msg))
            logging.info(f"Notified {dt_future.teamlead.login_staff} about {team}:\n{msg}")

    def job_ask_feedbacks(self, _bot=None, _update=None):
        # Send feedback request for everyone who was oncall 1 hour ago, but is not oncall now.
        # We call this routine every hour.
        hour_ago = datetime.datetime.now() - datetime.timedelta(hours=1)
        hour_ago_duty = TeamsCache(hour_ago)
        hour_ago_duty.generate_cache(self.templater, full=False)
        for team in hour_ago_duty.team_names:
            dt_now = self._cache.get_team_data(team)
            dt_hour_ago = hour_ago_duty.get_team_data(team)

            if not dt_hour_ago.timetable_filled:
                continue

            if dt_now.primary.login_staff != dt_hour_ago.primary.login_staff:
                should_ask_feedback = dt_now.ask_feedback and dt_hour_ago.primary.ask_feedback
                if should_ask_feedback:
                    logging.info(f"Trying to send poll to {dt_hour_ago.primary.login_staff} from {team}")
                    button_list = []
                    for id, score in enumerate(Config.DUTY_SCORE_LIST, start=1):
                        button_list.append([InlineKeyboardButton(score, callback_data=f"feedback:{team}:{id}")])
                    button_list.reverse()

                    markup = InlineKeyboardMarkup(button_list, resize_keyboard=True, one_time_keyboard=True)
                    msg = (
                        f"Как прошло дежурство за команду {self.templater.bold(team)}?\n\n"
                        "Эти опросы помогают понять, в каких командах больше всего болят дежурства. Мы очень "
                        "просим их заполнять, это несложно. Если же ты "
                        "не хочешь, чтобы бот присылал опросы, отправь `/disable_feedback`."
                    )

                    self.messenger.send_message(dt_hour_ago.primary.chat_id, Reply(msg, inline_keyboard_markup=markup))

    def job_ping_cloudinc_speaker(self, _bot=None, _update=None):
        missing_leads = {}
        issues = self._startrek_client.issues.find(Config.PING_CLOUDINC_FILTER)
        for issue in issues:
            for lead_login in {component.lead.login for component in issue.components}:
                logging.info(f"Trying to send reminder about IMs meeting to {lead_login}")

                lead = User(login_staff=lead_login)
                if lead.chat_id is None:
                    missing_leads[lead_login] = lead
                    continue

                msg = (
                    f"Придешь рассказать про <a href='https://st.yandex-team.ru/{issue.key}'>"
                    f"{issue.key} {issue.summary}</a> на продстатус сегодня?"
                )
                markup = self._generate_buttons_list(issue.key, "im_meeting")
                self.messenger.send_message(
                    lead.chat_id, Reply(msg, inline_keyboard_markup=markup, format=ParseMode.HTML)
                )

        for user in missing_leads.values():
            self._ask_to_activate_bot_in_yc_leads(user)

    def job_post_agenda(self, _bot=None, _update=None):
        msg = self._generate_agenda()
        leads_chat_settings = self._get_chat_settings_by_name("yc-leads", "telegram")

        logging.info(f"Posting agenda...")
        self.messenger.send_message(leads_chat_settings.id, Reply('\n'.join(msg), format=ParseMode.HTML))

    def _generate_agenda(self) -> List[str]:
        msg = ["<b>Сегодня на продстатусе</b>", ""]
        issues = self._startrek_client.issues.find(Config.TODAY_CLOUDINC_FILTER)
        msg.append("<b>Инциденты, готовые к разбору:</b>")
        if len(issues):
            logging.info("Today IM's meeting agenda:")
            msg.extend(self._compose_cloudinc_list(issues))
        else:
            msg.append("инцидентов нет\n")

        issues = self._startrek_client.issues.find(Config.FUTURE_CLOUDINC_FILTER)
        msg.append("<b>Инциденты, перенесенные на другую дату:</b>")
        if len(issues):
            logging.info("Future IM's meeting agenda:")
            msg.extend(self._compose_cloudinc_list(issues))
        else:
            msg.append("инцидентов нет\n")

        issues = self._startrek_client.issues.find(Config.NO_ANSWER_CLOUDINC_FILTER)
        if len(issues):
            msg.append("<b>Инциденты, по которым докладчики не ответили на призыв:</b>")
            logging.info("CLOUDINC without answer:")
            msg.extend(self._compose_cloudinc_list(issues))

        return msg

    def _compose_cloudinc_list(self, issues: list) -> List[str]:
        msg = []
        for issue in issues:
            logging.info(f"Process {issue.key} for agenda")
            speakers = []
            for lead_login in {component.lead.login for component in issue.components if component.lead}:
                speakers.append(User(login_staff=lead_login))

            msg.append(f"<a href='https://st.yandex-team.ru/{issue.key}'>{issue.key} {issue.summary}</a>, ")
            msg.append(', '.join([self.templater.render_tg_staff_logins(user, True) for user in speakers]))
            msg.append("")

        return msg

    def job_close_cloudinc(self, _bot=None, _update=None):
        prev_dt = datetime.datetime.now() - datetime.timedelta(hours=12)
        prev_duty = TeamsCache(prev_dt)
        prev_duty.generate_cache(self.templater, full=False)

        inc_manager = prev_duty.get_team_data("incmanager")
        if inc_manager.primary.chat_id is None:
            self._ask_to_activate_bot_in_yc_leads(inc_manager.primary)
            return

        issues = self._startrek_client.issues.find(Config.TODAY_CLOUDINC_FILTER)
        for issue in issues:
            logging.info(f"Trying to send reminder last IMs meeting to {inc_manager.primary.login_staff}")

            msg = (
                f"Инцидент <a href='https://st.yandex-team.ru/{issue.key}'>"
                f"{issue.key} {issue.summary}</a> рассмотрен на продстатусe сегодня?"
            )
            markup = self._generate_buttons_list(issue.key, "close_inc")
            self.messenger.send_message(
                inc_manager.primary.chat_id, Reply(msg, inline_keyboard_markup=markup, format=ParseMode.HTML)
            )

    def _generate_buttons_list(self, issue_key: str, callback_name: str) -> InlineKeyboardMarkup:
        button_list = [[InlineKeyboardButton("Да", callback_data=f"{callback_name}:{issue_key}:0")]]

        prefix = "Нет, перенести на"
        if datetime.datetime.today().weekday() < 4:
            button_list.append(
                [
                    InlineKeyboardButton(
                        f"{prefix} {Config.shift_translate[1]}", callback_data=f"{callback_name}:{issue_key}:1"
                    )
                ]
            )
        if datetime.datetime.today().weekday() < 3:
            button_list.append(
                [
                    InlineKeyboardButton(
                        f"{prefix} {Config.shift_translate[2]}", callback_data=f"{callback_name}:{issue_key}:2"
                    )
                ]
            )
        button_list.append(
            [
                InlineKeyboardButton(
                    f"{prefix} {Config.shift_translate[7]}", callback_data=f"{callback_name}:{issue_key}:7"
                )
            ]
        )
        return InlineKeyboardMarkup(button_list, resize_keyboard=True, one_time_keyboard=True)

    def _ask_to_activate_bot_in_yc_leads(self, user: User):
        logging.info(f"Asking {user} to say 'start' to DutyBot")
        leads_chat_settings = self._get_chat_settings_by_name("yc-leads", "telegram")
        tg_login = self.templater.render_login_tg(user, call=True).rstrip()
        msg = (
            f"Я попытался отправить {tg_login} сообщение о подготовке к продстатусу, но не смог.\n"
            f"{tg_login}, активируй меня в личке @{md('ycloud_duty_bot')} командой `/start`.\n"
            f"В случае проблем напиши дежурному `/duty bot`."
        )
        self.messenger.send_message(leads_chat_settings.id, Reply(msg))

    def job_check_incident(self, _bot=None, _update=None):
        # TODO: handle situation when more than one issue were created for last minute
        try:
            last_proceeded_issue_number = int(read_query("incidents")[0][0])
        except (IndexError, ValueError) as e:
            logging.warning("cant get issue number from database, no notify")
            self.messenger.notify_admins(
                Reply(f"job_check_incident failed, can't get issue number from incidents table: {e}")
            )
            return

        paginated_issues = self._startrek_client.issues.find(
            filter={"queue": "CLOUDINC"},
            order=[
                '-key',
            ],
            per_page=2,
        )
        last_issue = list(paginated_issues.get_page(1))[0]
        # last_issue.key is something like CLOUDINC-2792, we'd like to extract 2792 from it.
        last_issue_number = int(last_issue.key.split("-")[1])

        if last_issue_number == last_proceeded_issue_number:
            logging.info(f"No new incidents. Last issue: {last_issue.key}")
            return

        # GORE-105
        # This situation may happen when someone moved ticket to another queue
        # but bot already notified about it and iterated value in db
        if last_issue_number < last_proceeded_issue_number:
            logging.warning("incident info in base is hurrying up")
            self.messenger.notify_admins(
                Reply(
                    f"CLOUDINC issue number in the database is higher then the same one in https://st.yandex-team.ru/CLOUDINC. "
                    f"Autofixing it: CLOUDINC-{last_proceeded_issue_number} => {last_issue.key}"
                )
            )
            update_query(
                table="incidents",
                condition=f"WHERE last_issue={last_proceeded_issue_number}",
                fields={"last_issue": last_issue_number},
            )
            logging.warning(
                f"Autofixed CLOUDINC issue number in the database {last_proceeded_issue_number} => {last_issue_number}"
            )
            return

        all_chats = Chat.get_all_inc_notify_chats()
        components = {component.display.capitalize() for component in last_issue.components}
        notify_chats = self.generate_chats_list(all_chats, components, last_issue.priority)
        logging.info(f"Chats for notifying {notify_chats}")

        # Returns HTML formatting instead of markdown.
        msg = self.templater.generate_incident_output(
            issue_key=last_issue.key,
            summary=last_issue.summary,
            assignee=User(login_staff=last_issue.assignee.login) if last_issue.assignee else None,
            priority=last_issue.priority.display.lower(),
            components=components,
        )
        for chat in notify_chats:
            self.messenger.send_message(chat, Reply("\n".join(msg), format=ParseMode.HTML))

        update_issue_number = update_query(
            table="incidents",
            condition=f"WHERE last_issue={last_proceeded_issue_number}",
            fields={"last_issue": last_issue_number},
        )
        if update_issue_number:
            logging.info(f"New CLOUDINC last issue number has been set to {last_issue_number}")
            return
        logging.warning("Can't update last_issue field in incidents database")

    @staticmethod
    def generate_chats_list(chats, component, priority):
        output = []
        for chat in chats:
            if chat[1] == "all":
                output.append(chat[0])
                continue
            if chat[1] is not None and component.intersection(set(chat[1].split(","))):
                output.append(chat[0])
            if priority.display.lower() == "критический" and chat[1] == "critical":
                output.append(chat[0])
        return output

    # Command Handlers
    # TODO (andgein): extract handlers to separate classes

    def command_duty(self, message: IncomingMessage):
        chat_settings = self._get_chat_settings(message.chat)

        default_for_this_chat_team = None
        db_chat = Chat(message.chat.messenger, message.chat.chat_id)
        if db_chat.default_duty_team is not None:
            if chat_settings.teams is None or db_chat.default_duty_team in chat_settings.teams:
                default_for_this_chat_team = db_chat.default_duty_team

        if not message.args:
            if default_for_this_chat_team is not None:
                message.args = [default_for_this_chat_team]
                return self.command_duty_single_team(message, None, True)
            elif message.chat.is_personal or db_chat.duty_noargs:
                return self.command_duty_without_args(message)
            else:
                return

        if len(message.args) == 1:
            return self.command_duty_single_team(message, default_for_this_chat_team)

        self.command_duty_multiple_teams(message, default_for_this_chat_team)

    def command_duty_without_args(self, message: IncomingMessage):
        chat_settings = self._get_chat_settings(message.chat)
        for text in self._generate_single_string_output_cache(chat_settings.teams):
            self.messenger.reply(message, Reply(text))

    def command_duty_single_team(self, message: IncomingMessage, default_team, show_team_name=False):
        chat_settings = self._get_chat_settings(message.chat)

        team_name = message.args[0].lower()

        # GORE-157. Support fake commands.
        if team_name in self._config.fake_teams:
            self.messenger.reply(message, Reply("\n".join(self._config.fake_teams[team_name])))
            return

        team = self._find_team_in_chat(team_name, chat_settings)

        # GORE-114. Support default team
        if team is None and default_team is not None:
            team = self._find_team_in_chat(default_team, chat_settings)
            show_team_name = True
            logging.info(f"Fallback to default team: {default_team}")

        if team is None:
            self.messenger.reply(message, Reply(f"Я не знаю команду дежурных `{team_name}`"))
            return

        self._publish_stats(message, team_name, team)

        text = (self.templater.bold("Призвал") + f": {team.name}\n") if show_team_name else ""
        text = text + " \n".join(team.classic_output)
        keyboard_markup = self._get_keyboard_with_see_button(message, [team])
        self.messenger.reply(message, Reply(text, inline_keyboard_markup=keyboard_markup))

        self._alert_if_duty_is_not_member(message, (team,))

    def command_duty_multiple_teams(self, message: IncomingMessage, default_team: Optional[str] = None):
        chat_settings = self._get_chat_settings(message.chat)

        # Args passed by telegram lib are separated by spaces
        # Use split to remove excess commas
        # Only first 5 teams are matter
        teams = ",".join(message.args).split(",")[:5]
        # Remove empty entries
        teams = filter(bool, teams)

        # dict.fromkeys().keys() guarantee that
        # set will be ordered, instead of default set()
        teams = dict.fromkeys(map(str.lower, teams)).keys()

        show_team_name = False
        msg, teams_list = [], []
        break_team = None

        for team in teams:
            # GORE-157. Support fake commands.
            if team in self._config.fake_teams:
                msg.append("\n" + "\n".join(self._config.fake_teams[team]))
                teams_list.append(FakeTeam(team))
            else:
                team_obj = self._find_team_in_chat(team, chat_settings)
                if team_obj is None or team_obj.name in teams_list:
                    break_team = team
                    break

                teams_list.append(team_obj)
                if msg:
                    msg.append("\n")
                msg.append("\n".join(team_obj.classic_output))
                self._publish_stats(message, team, team_obj)

        # GORE-114. Support default team
        if not teams_list and default_team is not None:
            team_obj = self._find_team_in_chat(default_team, chat_settings)
            if team_obj is not None:
                teams_list.append(team_obj.name)
                msg.append("\n" + "\n".join(team_obj.classic_output))
                self._publish_stats(message, default_team, team_obj)
                show_team_name = True

        if not teams_list:
            self.messenger.reply(message, Reply(f"Не знаю команду `{break_team}`"))
            return

        summon_string = "\n".join(msg)
        output = summon_string

        if len(teams_list) > 1 or show_team_name:
            teams_string = self.templater.bold("Призвал") + f": {', '.join(t.name for t in teams_list)}\n"
            output = teams_string + summon_string

        keyboard_markup = self._get_keyboard_with_see_button(message, teams_list)
        self.messenger.reply(message, Reply(output, inline_keyboard_markup=keyboard_markup))

        self._alert_if_duty_is_not_member(message, teams_list)

    @staticmethod
    def _get_keyboard_with_see_button(
        message: IncomingMessage, teams_list: List[Union[FakeTeam, Team]]
    ) -> Optional[InlineKeyboardMarkup]:
        # Show buttons only in public chats
        if message.chat.is_personal:
            return None

        team_names = [t.name for t in teams_list]
        button_list = [
            [
                InlineKeyboardButton("👀 Смотрю", callback_data="see:" + ",".join(team_names)),
            ]
        ]
        return InlineKeyboardMarkup(button_list)

    @staticmethod
    def _get_ttl(seconds=1800) -> int:
        """Return the same value withing `seconds` time period"""
        return int(time.time() // seconds)

    def _alert_if_duty_is_not_member(self, message: IncomingMessage, teams_list: Sequence[Union[FakeTeam, Team]]):
        # See https://st.yandex-team.ru/GORE-156
        if not message.chat.is_personal:
            msg = []
            for team_obj in teams_list:
                if not isinstance(team_obj, Team):
                    continue

                is_member = True  # Skip warning is better than wrong warning
                if team_obj.primary.chat_id:
                    is_member = self.messenger.is_user_a_member_of_chat(
                        message.chat.chat_id, team_obj.primary.chat_id, ttl=self._get_ttl()
                    )

                if not is_member:
                    # TODO: support for non-telegram chats here
                    if team_obj.chat_link is not None and len(team_obj.chat_link.strip()):
                        chat_link = f"Обратитесь в [сервисный чат]({team_obj.chat_link})!"
                    elif "Cloud PROD Support" not in message.chat.title:
                        chat_link = "Обратитесь в [Cloud PROD Support](https://t.me/joinchat/AXmZSke_Y2Hv6HTStRCPMg)!"
                    else:
                        chat_link = ""

                    txt = f"⛔️ Вы призываете дежурного ([{md(team_obj.primary.login_tg)}]{self.templater.render_login_tg(team_obj.primary)}), "
                    txt += f"которого нет в этом чате. {chat_link}"
                    msg.append(txt)

            if len(msg):
                self.messenger.reply(message, Reply("\n".join(msg)))

    def command_update_cache(self, message: IncomingMessage):
        self.messenger.reply(message, Reply("Обновляю кеш 👀"))
        self.update_cache()
        self.messenger.reply(message, Reply("Кеш обновлён 😈"))

    def command_create_incident(self, message: IncomingMessage, queue_name: str = "CLOUDINC"):
        if not message.args:
            self.messenger.reply(message, Reply("Синтаксис команды: `/inc <название-команды> <заголовок-инцидента>`"))
            return

        if len(message.args) < 2:
            self.messenger.reply(
                message,
                Reply(
                    "Ты не указал заголовок тикета.\n"
                    "Синтаксис команды: `/inc <название-команды> <заголовок-инцидента>`"
                ),
            )
            return

        passed_team_name = message.args[0]
        team_name = self._cache.find_closest_team(passed_team_name)
        if not team_name:
            self.messenger.reply(message, Reply(f"Я не знаю команду `{passed_team_name}` 😔"))
            return

        team = self._cache.get_team_data(team_name)
        inc_manager = self._cache.get_team_data("incmanager")
        logging.info(f"User {message.user.login_staff} requested to create CLOUDINC issue for {team_name}")

        components = None
        if team.startrack_component in [c.name for c in self._startrek_client.queues[queue_name].components]:
            components = team.startrack_component

        header = f"[{datetime.datetime.strftime(datetime.datetime.now(), '%d.%m.%Y')}][{team_name.capitalize()}] "
        # GORE-145
        reply_before = datetime.datetime.strftime(
            datetime.datetime.utcnow() + datetime.timedelta(days=1), "%Y-%m-%dT%H:%M"
        )

        issue_description = dedent("""\
            ====Что произошло
            ====Таймлайн
            ====Причины и подробное описание
            ====Как избежать повторения\
            """)
        try:
            issue = self._startrek_client.issues.create(
                queue=queue_name,
                components=components,
                assignee=team.primary.login_staff,
                summary=header + " ".join(message.args[1:]),
                description=issue_description,
                followers=[team.backup.login_staff, team.teamlead.login_staff, inc_manager.primary.login_staff],
                priority={
                    "key": "normal",
                },
                replyBefore=reply_before,
            )
            if not issue:
                raise ValueError("Startrek returned empty response, issue was not created")
        except Exception:
            self.messenger.reply(message, Reply(f"Не смог создать инцидентный тикет для команды {team_name} 🤕"))
            raise

        if not team.startrack_component:
            response_msg = (
                f"Для команды `{team_name}` не заполнена компонента в боте\n"
                f"Создал тикет без компоненты: [{issue.key}](https://st.yandex-team.ru/{issue.key})\n"
                f"Проставь, пожалуйста, компоненту в тикете самостоятельно и обратись к `/duty bot`"
            )
            self.messenger.reply(message, Reply(response_msg))
        elif not issue.components:
            response_msg = (
                f"Для команды `{team_name}` мы знаем компоненту `{team.startrack_component}`\n"
                f", но такой компоненты нет в очереди `{issue.queue.key}` ({issue.queue.display})\n"
                f"Поэтому я создал тикет без компоненты: [{issue.key}](https://st.yandex-team.ru/{issue.key})\n"
                f"Проставь, пожалуйста, компоненту в тикете самостоятельно и обратись к `/duty bot`"
            )
            self.messenger.reply(message, Reply(response_msg))
        else:
            self.messenger.reply(message, Reply(f"Создал тикет: [{issue.key}](https://st.yandex-team.ru/{issue.key})"))

        logging.info(f"Created startrek issue {issue.key}")

        # Hack message.args to summon current oncall duties
        message.args = ["support", "incmanager", team_name]
        self.command_duty(message)

    def command_help(self, message: IncomingMessage):
        help_msg = [
            self.templater.bold("Вот, что я умею:"), "",
            "`/duty` — получить полный список дежурных (работает только в личке)", "",
            "`/duty <имя-команды> [<текст-призыва>]` — призвать дежурных команды с конкретным вопросом", "",
            "`/dt <имя-команды>` — узнать дежурный тикет команды", "",
            "`/post <текст>` — переслать сообщение в телеграм-канал "
            "[YC site announcements](https://t.me/joinchat/AAAAAEcFWqlPw_TfsCPUow) (работает только в телеграме)", "",
            "`/myduty` – посмотреть свои дежурства на ближайший месяц  (работает только в телеграме)", "",
            "`/inc <имя-команды> <заголовок-инцидента>` — завести инцидент", "",
            "`номер_тикета` — резолвит номер тикета из раздела поддержки в CLOUDSUPPORT/CLOUDLINETWO-тикет", "",
            "`/disable_feedback` — не присылать опросы о прошедших дежурствах (работает только в телеграме)", "",
            "`/enable_feedback` — присылать опросы о прошедших дежурствах (работает только в телеграме)", "",
            self.templater.bold("Что еще:"),
            "• Чтобы активировать бота в чате в телеграм-чате, напишите /start. Дежурный всё проверит "
            "и активирует бота в ближайшее время.",
            "• Дежурные подтягиваются в бота из resps-api (aka GoRe) примерно раз в минуту.",
            "• Напоминания о дежурстве приходят в телеграм в 18:00 МСК.",
        ]
        self.messenger.reply(message, Reply("\n".join(help_msg)))
        logging.info(f"Sent help to {message.chat.title}")

    def command_resolve_support_ticket(self, message: IncomingMessage):
        query = f'(Queue: "CLOUDSUPPORT" or Queue: "CLOUDLINETWO") and "Ticked ID": "{message.text.strip()}"'
        try:
            tickets = self._startrek_client.issues.find(query)
            ticket_key = tickets[0].key
        except Exception as e:
            logging.warning(f"Can't fetch tickets from startrek: {e}", exc_info=e)
            ticket_key = None

        if not ticket_key:
            self.messenger.reply(
                message,
                Reply(
                    "Я поискал обращение в поддержку с таким ID, но ничего не нашёл. "
                    "Если ты уверен, что номер правильный, а я просто не справился — напиши `/duty bot`."
                ),
            )
            return

        self.messenger.reply(
            message,
            Reply(
                f"По этому обращению есть тикет в поддержке — [{ticket_key}](https://st.yandex-team.ru/{ticket_key})"
            ),
        )

    def command_disable_feedback(self, message: IncomingMessage):
        if not message.chat.is_personal:
            self.messenger.reply(message, Reply("Эта команда работает только в личных чатах"))
            return

        message.user.set_ask_feedback(False)

        self.messenger.reply(
            message,
            Reply(
                self.templater.bold("Выключили!")
                + "\n\nБольше не будем беспокоить тебя вопросами о прошедших дежурствах. "
                "Если захочешь включить опросы обратно, отправь команду `/enable_feedback`."
            ),
        )

    def command_enable_feedback(self, message: IncomingMessage):
        if not message.chat.is_personal:
            self.messenger.reply(message, Reply("Эта команда работает только в личных чатах"))
            return

        message.user.set_ask_feedback(True)

        self.messenger.reply(
            message,
            Reply(
                self.templater.bold("Включили!")
                + "\n\nТеперь будем спрашивать тебя о прошедших дежурствах в 13:00 МСК. "
                "Если захочешь выключить опросы, просто отправь команду `/disable_feedback`."
            ),
        )

    def command_admin_get_statistics(self, message: IncomingMessage):
        if not message.user.bot_admin:
            return

        if not message.chat.is_personal:
            self.messenger.reply(message, Reply("Эта команда работает только в личных чатах"))
            return

        statistics = {}
        statistics["Cache updating thread liveness"] = self._cache_updating_thread.is_alive()
        statistics["Cache updated at"] = (
            self._cache_updating_timestamp.strftime("%Y-%m-%d %H:%M:%S.%f") if self._cache_updating_timestamp else None
        )
        statistics["Cache keys"] = list(self._cache.team_data.keys())

        statistics_message = self.templater.bold("Statistics") + "\n\n"
        statistics_message += "\n".join(
            f"{key}: {self.templater.mono(repr(value))}" for key, value in statistics.items()
        )

        self.messenger.reply(message, Reply(statistics_message))

    def command_admin_send_reminders(self, message: IncomingMessage):
        self.messenger.reply(message, Reply("Started notify process 👀"))
        self.job_send_reminders()
        self.messenger.reply(message, Reply("Finished notify 😇"))

    def command_admin_notify_team_leads_about_finished_timetable(self, message: IncomingMessage):
        self.messenger.reply(message, Reply("Started notify process 👀"))
        self.job_notify_team_leads_about_finished_timetable()
        self.messenger.reply(message, Reply("Finished notify process 😇"))

    def command_admin_ask_feedbacks(self, message: IncomingMessage):
        self.messenger.reply(message, Reply("Started asking process 👀"))
        self.job_ask_feedbacks()
        self.messenger.reply(message, Reply("Finished asking process 😇"))

    def command_admin_ping_cloudinc_speaker(self, message: IncomingMessage):
        self.messenger.reply(message, Reply("Started asking process 👀"))
        self.job_ping_cloudinc_speaker()
        self.messenger.reply(message, Reply("Finished asking process 😇"))

    def command_admin_post_agenda(self, message: IncomingMessage):
        self.messenger.reply(message, Reply("Started posting process 👀"))
        self.job_post_agenda()
        self.messenger.reply(message, Reply("Finished posting process 😇"))

    def command_admin_close_cloudinc(self, message: IncomingMessage):
        self.messenger.reply(message, Reply("Started closing inc process 👀"))
        self.job_close_cloudinc()
        self.messenger.reply(message, Reply("Finished asking process 😇"))

    def command_admin_check_incident(self, message: IncomingMessage):
        self.messenger.reply(message, Reply("Started asking process 👀"))
        self.job_check_incident()
        self.messenger.reply(message, Reply("Finished asking process 😇"))

    def command_get_duty_ticket(self, message: IncomingMessage):
        chat_settings = self._get_chat_settings(message.chat)
        team_name = message.args[0]
        team = self._find_team_in_chat(team_name, chat_settings)
        if not team:
            self.messenger.reply(message, Reply(f"Я не знаю команду дежурных `{team_name}`"))
            return

        if not team.ticket:
            self.messenger.reply(
                message, Reply(f"Дежурный тикет для команды `{team_name}` не заполнен в resps-api aka GoRe")
            )
            return

        self.messenger.reply(message, Reply(f"https://st.yandex-team.ru/{team.ticket}", disable_web_page_preview=True))

    def command_list_my_duties(self, message: IncomingMessage):
        schedule = self._resps_client.get_upcoming_duties_by_user(message.user.login_staff)
        output = self.templater.generate_user_duties_output(schedule)

        self.messenger.reply(message, Reply("\n".join(output)))

    def command_start(self, message: IncomingMessage):
        if message.chat.is_personal:
            self._initialize_private_chat(message)
            return
        self._initialize_group_chat(message)

    def _initialize_private_chat(self, message: IncomingMessage):
        if message.user.exists_in_db:
            self.messenger.reply(message, Reply(f"Мы уже знакомы, {message.user.login_staff}!"))
            return

        if not message.user.allowed:
            self.messenger.reply(message, Reply("Я не могу тебе ответить, так как ты не являешься сотрудником."))
            return

        result = message.user.add_user()
        if not result:
            self.messenger.notify_admins(Reply(f"Can't add new user {message.user.login_tg}"))
            self.messenger.reply(
                message,
                Reply(
                    "Не могу добавить тебя в список пользователей из-за ошибки, "
                    "но я уже написал моим хозяевам об этом."
                ),
            )
            return

        logging.info(f"New user added: staff={message.user.login_staff}, telegram={message.user.login_tg}")
        self.messenger.reply(message, Reply("Готов к работе! Загляни в /help"))

    def _initialize_group_chat(self, message: IncomingMessage):
        chat = Chat("telegram", chat_id=message.chat.chat_id, chat_name=message.chat.title)
        if chat.allowed:
            self.messenger.reply(message, Reply("Готов к работе!"))
            return

        logging.info(f"New chat: {chat.id}, {chat.name}")
        msg = (
            "Меня активируют в ближайшее время. "
            "Если этого не произойдет, обратитесь в [чат](https://t.me/joinchat/TunREb2h4a0SRWN8). \n"
            + "А пока проверьте, что вы [добавили](https://wiki.yandex-team.ru/security/chats/telegramissues/#instrukciiporabotestelegramom) "
            "[tashanaturalbot](https://t.me/TashaNaturalBot) в ваш чат."
        )
        self.messenger.reply(message, Reply(msg, disable_web_page_preview=True))

        caller = ""
        if message.user.login_tg:
            caller = f"\nЗапросил: [{md(message.user.login_staff)}](https://t.me/{message.user.login_tg})\n"

        bot_support_chat_settings = self._get_chat_settings_by_name("bot-support", message.chat.messenger)
        bot_team = self._find_team_in_chat("bot", bot_support_chat_settings)
        msg = f"Новый запрос на активацию бота в чате: '{chat.name}'.{caller}\n"
        msg = msg + " \n".join(bot_team.classic_output)
        reply_markup = self._render_activate_chat_button(chat)
        self.messenger.send_message(bot_support_chat_settings.id, Reply(msg, inline_keyboard_markup=reply_markup))

    @staticmethod
    def _render_activate_chat_button(chat: Chat) -> InlineKeyboardMarkup:
        button_list = [
            [
                InlineKeyboardButton("Активировать", callback_data=f"activate:{chat.id}:{chat.name}"),
            ]
        ]
        return InlineKeyboardMarkup(button_list)

    def command_reset(self, message: IncomingMessage):
        if not message.chat.is_personal:
            return

        if message.user.del_user():
            self.messenger.reply(message, Reply("Если передумаешь — пиши /start!"))
            return

        self.messenger.reply(message, Reply("Не смог удалить тебя из базы, уведомил об этом своих хозяев"))
        self.messenger.notify_admins(Reply(f"Cant delete user {message.user.login_staff}"))

    def command_post(self, message: IncomingMessage):
        if len(message.text) < 6:
            self.messenger.reply(message, Reply("Не могу отправить в канал пустое сообщение"))
            logging.info(f"{message.user.login_tg} tried to post empty message to YC site announcements")
            return

        try:
            yc_announcements_chat_settings = self._get_chat_settings_by_name("yc-announcements", message.chat.messenger)
        except ValueError:
            self.messenger.notify_admins(
                Reply(f"{message.user.login_staff} tried to use /post, but yc-announcements channel not found")
            )
            self.messenger.reply(
                message, Reply("Не смог запостить сообщение из-за внутренней ошибки. Уже написал своим хозяевам!")
            )
            return

        post_text = message.text_markdown[5:]
        msg = f"{post_text}\n\nот [{message.user.login_staff}](https://t.me/{message.user.login_tg})"
        self.messenger.send_message(yc_announcements_chat_settings.id, Reply(msg))

    def command_send_feedback_text(self, message: IncomingMessage):
        if message.chat.is_personal:
            feedback = Feedback.get_last_feedback(message.chat.chat_id)
            Feedback.update_feedback(feedback.id, message.text.strip())
            self.messenger.reply(message, Reply(f"Спасибо, твой отзыв по дежурству *{feedback.component}* сохранен!"))

    def command_get_agenda(self, message: IncomingMessage):
        if message.chat.is_personal:
            output = self._generate_agenda()
            self.messenger.reply(message, Reply("\n".join(output), format=ParseMode.HTML))

    # Callback handlers

    def callback_process_summon(self, query: IncomingCallbackQuery):
        _, team_names = query.data.split(":")
        team_duties = set()
        for team_name in team_names.split(","):
            team = self._cache.get_team_data(team_name)
            if team is not None:
                team_duties |= team.get_duty_telegram_usernames()

        if query.user.login_tg.lower() not in team_duties:
            logging.debug(f"Summon reply is allowed only for duties from following teams: {team_names}")
            self.messenger.answer_callback_query(
                query, CallbackQueryReply("Кнопка только для дежурных! :)", show_alert=True)
            )
            return

        self.messenger.edit_message_reply_markup(
            query.chat.chat_id, query.message.message_id, new_inline_keyboard_markup=None
        )

        login = f"[{md(query.user.login_tg)}](https://t.me/{query.user.login_tg})"
        reply_to_message_id = query.message.reply_to_message.message_id if query.message.reply_to_message else None
        self.messenger.send_message(
            query.chat.chat_id, Reply(f"{login} смотрит"), reply_to_message_id=reply_to_message_id
        )

    def callback_feedback_score(self, query: IncomingCallbackQuery):
        _, component, score = query.data.split(":")

        feedback = Feedback(chat_id=query.message.chat.chat_id, component=component, score=int(score))
        result = feedback.add_feedback()

        msg = (
            f"Как прошло дежурство за команду *{component}*?\n"
            + f"Вы ответили: {Config.DUTY_SCORE_LIST[int(score) - 1] if (0 < int(score) <= 5) else ''}"
        )

        self.messenger.edit_message(
            query.message.chat.chat_id,
            query.message.message_id,
            Reply(msg),
        )

        msg = f"Спасибо! Если хотите, оставьте отзыв на дежурство в команде *{component}*:"
        if not result:
            logging.info(f"Can't add feedback from user with chat: {query.message.chat.chat_id}")
            msg = "Что-то пошло не так ;( Напишите дежурному `/duty bot`"

        self.messenger.reply(query.message, Reply(msg))

    def callback_activate_chat(self, query: IncomingCallbackQuery):
        team_obj = self._cache.get_team_data(self._cache.find_closest_team("bot"))
        if query.user.login_tg.lower() not in team_obj.get_duty_telegram_usernames():
            logging.debug("Only duty from resps/bot team can activate dutybot")
            self.messenger.answer_callback_query(
                query, CallbackQueryReply("Кнопка только для дежурных! :)", show_alert=True)
            )
            return

        _, chat_id, chat_name = query.data.split(":", 2)
        chat = Chat("telegram", chat_id=chat_id, chat_name=chat_name)
        logging.debug(f"Activating chat {chat_id} with chat name: {chat_name}...")
        if chat.add_chat():
            self.messenger.edit_message_reply_markup(
                query.message.chat.chat_id, query.message.message_id, new_inline_keyboard_markup=None
            )
            msg = "Чат активирован"
        else:
            msg = "Не могу активировать чат. Помогите!"

        self.messenger.reply(query.message, Reply(msg))

    def callback_im_meeting(self, query: IncomingCallbackQuery):
        _, issue_key, action = query.data.split(":")
        action = int(action)
        if action != 0 and action not in Config.shift_translate.keys():
            return

        issue = self._startrek_client.issues[issue_key]
        orig = (
            f"Придешь рассказать про <a href='https://st.yandex-team.ru/{issue.key}'>"
            f"{issue.key} {issue.summary}</a> продстатус сегодня?\nВы ответили: "
        )
        today = datetime.date.today()

        if action == 0:
            dt = today
            repl = "Ждем сегодня на [продстатусе](https://calendar.yandex-team.ru/event/66196033)!"
            msg = "да!"
        else:
            shift = action
            if action == 7:
                shift = 7 - today.weekday()

            dt = today + datetime.timedelta(days=shift)
            formated_dt = self.templater.render_russian_date_range(dt, dt)
            repl = f"Доклад перенесен на *{formated_dt}*"
            msg = f"нет, перенести на {Config.shift_translate[action]}"

        meeting_date = datetime.datetime.strptime(issue.meetingDate, '%Y-%m-%d').date()
        if meeting_date < dt:
            try:
                issue.update(meetingDate=dt.strftime('%Y-%m-%d'))
                issue.comments.create(text=f"{query.user.login_staff} перенес(ла) доклад на {formated_dt}")
            except Exception:
                self.messenger.reply(query.message, Reply("Не смог обновить тикет 🤕"))
                raise
        else:
            repl = (
                f"Ваш коллега уже перенес доклад на "
                f"*{self.templater.render_russian_date_range(meeting_date, meeting_date)}*."
            )

        self.messenger.edit_message(
            query.message.chat.chat_id,
            query.message.message_id,
            Reply(f"{orig} {msg}", format=ParseMode.HTML),
        )
        self.messenger.reply(query.message, Reply(repl))

    def callback_close_inc(self, query: IncomingCallbackQuery):
        _, issue_key, action = query.data.split(":")
        action = int(action)
        if action != 0 and action not in Config.shift_translate.keys():
            return

        issue = self._startrek_client.issues[issue_key]
        orig = (
            f"Инцидент <a href='https://st.yandex-team.ru/{issue.key}'>"
            f"{issue.key} {issue.summary}</a> рассмотрен на продстатусе сегодня?\nВы ответили: "
        )
        today = datetime.date.today()

        if action == 0:
            self.messenger.edit_message(
                query.message.chat.chat_id,
                query.message.message_id,
                Reply(f"{orig} да!", format=ParseMode.HTML),
            )
            try:
                issue.update(tags={'add': ['inc-report:done']})
            except Exception:
                self.messenger.reply(query.message, Reply(f"Не смог добавить тэг 'inc-report:done' в {issue.key} 🤕"))
                raise
        else:
            shift = action
            if action == 7:
                shift = 7 - today.weekday()

            dt = today + datetime.timedelta(days=shift)
            msg = f"нет, перенести на {Config.shift_translate[action]}"
            self.messenger.edit_message(
                query.message.chat.chat_id,
                query.message.message_id,
                Reply(f"{orig} {msg}", format=ParseMode.HTML),
            )
            try:
                issue.update(meetingDate=dt.strftime('%Y-%m-%d'))
                formated_dt = self.templater.render_russian_date_range(dt, dt)
                issue.comments.create(text=f"{query.user.login_staff} перенес(ла) доклад на {formated_dt}")
            except Exception:
                self.messenger.reply(query.message, Reply(f"Не смог обновить тикет {issue.key} 🤕"))
                raise

            self.messenger.reply(
                query.message, Reply(f"Доклад перенесен на *{self.templater.render_russian_date_range(dt, dt)}*")
            )
