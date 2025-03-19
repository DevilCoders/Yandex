import json
import logging
from datetime import (
    datetime,
    timedelta
)

import boto3

import app.database.cache as cache_orm
import app.database.chats as chats_orm
import app.database.components as components_orm
import app.database.users as users_orm
import app.utils.resps as resps
import app.utils.staff as staff


class User:
    __slots__ = ('chat_id', 'login_tg', 'login_staff', 'name_staff',
                 'abuse', 'support', 'work_day', 'bot_admin',
                 'allowed', 'cloud', 'duty_notify', 'incidents')

    def __init__(self, chat_id=None,
                 login_tg=None, login_staff=None):
        if login_staff is not None:
            _data = users_orm.get_user(login_staff)
            if _data and _data != 'None':
                for _key, _value in _data.items():
                    setattr(self, _key, _value)
            else:
                self.login_staff = login_staff
                _staff_data = staff.get_data_from_staff(staff=login_staff)
                for _key, _value in _staff_data.items():
                    setattr(self, _key, _value)
        elif chat_id is not None:
            chat_id = str(chat_id)
            _data = users_orm.get_user(chat_id)
            if _data and _data != 'None':
                for _key, _value in _data.items():
                    setattr(self, _key, _value)
            else:
                self.chat_id, self.login_tg = chat_id, login_tg
                self.abuse, self.support = False, False
                self.incidents, self.bot_admin = False, False
                self.work_day = '10 20'
                _staff_data = staff.get_data_from_staff(telegram=self.login_tg)
                for _k, _v in _staff_data.items():
                    setattr(self, _k, _v)

    def __str__(self):
        return f'{self.name_staff.split(" ", 1)[0]},' \
                f'вот информация, которую я о тебе знаю:\n\n' \
                f'<b>Chat ID</b> {self.chat_id}\n' \
                f'<b>Админ бота</b> {"Нет" if not self.bot_admin else "Да"}\n' \
                f'<b>Рабочий день</b> с {self.work_day.split(" ")[0]}' \
                f'до {self.work_day.split(" ")[1]}\n' \
                f'<b>Нотификации об абьюзах</b> {"Нет" if not self.abuse else "Да"}\n' \
                f'<b>Нотификации о саппортских тикетах</b> ' \
                f'{"Нет" if not self.support else "Да"}\n' \
                f'<b>Нотификации о ближайших дежурствах</b>' \
                f'{"Нет" if not self.duty_notify else "Да"}\n' \
                f'<b>Нотификации об инцидентных тикетых</b> {"Нет" if not self.incidents else "Да"}'

    def __dict__(self):
        return dict(chat_id=self.chat_id,
                    login_tg=self.login_tg,
                    login_staff=self.login_staff,
                    name_staff=self.name_staff,
                    abuse=self.abuse,
                    support=self.support,
                    work_day=self.work_day,
                    bot_admin=self.bot_admin,
                    allowed=self.allowed,
                    cloud=self.cloud,
                    duty_notify=self.duty_notify,
                    incidents=self.incidents)

    def add_user(self):
        logging.info(f"[bot_utils.add_user] Adding new user ({self.chat_id}, {self.login_staff})")
        users_orm.add_user(**(self.__dict__()))

    @property
    def get_user(self):
        return False if users_orm.get_user(self.chat_id) == 'None' else True

    def del_user(self):
        status = users_orm.delete_user(self.chat_id)
        logging.info(f"[bot_utils.del_user] del {self.login_staff} operation status: {status}")

    def get_schedule(self):
        return resps.get_user_upcoming_duties(self.login_staff)

    def update_user(self, **kwargs):
        logging.info(f"[bot_utils.update_user] Updating user ({self.chat_id}, {self.login_staff})")
        users_orm.update_user(**kwargs)

    def list_users(self, **kwargs):
        if not kwargs:
            result = users_orm.get_all_users()
        else:
            result = users_orm.get_all_users(**kwargs)
        return result


class Chat:
    __slots__ = ('chat_id', 'chat_name', 'component',
                 'admin', 'incidents', 'welcome_message',
                 )

    def __init__(self, chat_id=None, name=None):
        if chat_id is not None:
            chat_id = str(chat_id)
            _data = chats_orm.get_chat(chat_id)
            if _data and _data != 'None':
                for _k, _v in _data.items():
                    setattr(self, _k, _v)
            else:
                self.chat_id = str(chat_id)
                self.chat_name = name
                self.component = None
                self.admin = None
                self.incidents = False
                self.welcome_message = False
        else:
            _data = chats_orm.get_chat(name)
            if _data:
                for _k, _v in _data.items():
                    setattr(self, _k, _v)

    def __str__(self):
        return (f"<b>ID чата</b> {self.chat_id}\n"
                f"<b>Название чата</b> {self.chat_name}\n"
                f"<b>Компонента чата</b> {self.component}\n"
                f"<b>Отвественный за чат</b> {self.admin}\n"
                f"<b>Оповещения об инцидентах</b> {self.incidents}")

    def list_chats(self, **kwargs):
        if not kwargs:
            result = chats_orm.get_all_chats()
        else:
            result = chats_orm.get_all_chats(**kwargs)
        return result

    @property
    def get_chat(self):
        return bool(chats_orm.get_chat(self.chat_id))

    def __dict__(self):
        return dict(chat_id=self.chat_id,
                    chat_name=self.chat_name,
                    component=self.component,
                    admin=self.admin,
                    incidents=self.incidents,
                    welcome_message=self.welcome_message)


class Duty:
    __slots__ = ('result_dict', 'duty_args', 'raw',
                 'duty_reply', 'result', 'duty_teams',
                 'temp_result', 'kostyl', 'temp_result_2')

    def __init__(self):
        self.result_dict = {}
        self.duty_args = {}
        self.raw = {}
        self.duty_reply = ""
        self.result = ""
        self.temp_result, self.kostyl = "", ""

    def get_duty_teams(self):
        self.duty_teams = {}
        for team in resps.get_teams_list():
            # Match resps data with additional cfg from database
            self.duty_teams[team] = components_orm.get_component(team)
        return self.duty_teams

    def update_cache(self):
        # Static teams: marketplace, business, bot, resps, ui, s3
        logging.info("[bot_utils.update_cache] Started")
        self.duty_teams = self.get_duty_teams()
        if self.duty_teams is None:
            logging.info(f"[bot_utils.update_cache] Update stopped by resps error, using last data")
            return f"{Emojis.FAIL} Failed"
        for team, config in self.duty_teams.items():
            try:
                hour = int(datetime.now().hour)
                if hour >= int(config.get('rotate_time')):
                    date = datetime.strftime(datetime.today(), "%d-%m-%Y")
                else:
                    date = datetime.strftime(datetime.today() - timedelta(1), "%d-%m-%Y")
                oncall = resps.get_oncall(team, date)
                if oncall is None:
                    raise NoActualDuty
                if team == "support":
                    support = "backup" if (hour < 10 or hour >= 22) else "primary"
                    primary = User(login_staff=oncall[support])
                    backup = User(login_staff=oncall[support])
                else:
                    if oncall.get('primary') is None:
                        raise NoActualDuty
                    if oncall.get('backup') is None:
                        oncall['backup'] = oncall['primary']
                    primary = User(login_staff=oncall.get('primary'))
                    backup = User(login_staff=oncall.get('backup'))
                cache = {"main": primary.login_staff,
                         "backup": backup.login_staff,
                         "main_tg": primary.login_tg,
                         "backup_tg": backup.login_tg}
                result = cache_orm.update_cache(team, **cache)
                if result == 'None':
                    logging.info(f"[bot_utils.update_cache] {team} wasn't exist, adding...")
                    cache['team'] = team
                    cache_orm.add_cache(**cache)
            except NoActualDuty:
                logging.info(f"[bot_utils.update_cache] {team} doesnt have actual duty")
                cache = {"main": None,
                         "backup": None,
                         "main_tg": None,
                         "backup_tg": None}
                cache_orm.update_cache(team, **cache)
            except Exception as e:
                logging.info(f"[bot_utils.update_cache] {team} failed {e}. {oncall}, {config}")
        logging.info("[bot_utils.update_cache] Finished")

    def generate_duty(self):
        self.raw = cache_orm.get_all_cache()
        self.temp_result, self.temp_result_2 = "", ""
        for team, duty in sorted(self.raw.items()):
            self.generate_output_string(team, duty)
        self.result = f"{Emojis.FIRE} Сегодня дежурят: \n\n{self.temp_result}"
        self.kostyl = f"{self.temp_result_2} "
        logging.info(f"[bot_utiles.generate_duty] Output generated. Len" \
                     f" is {len(self.result) + len(self.kostyl)}")

    def generate_output_string(self, team: str, data: dict) -> set:
        team_options = components_orm.get_component(team)
        product_owner = User(login_staff=team_options.get('component_admin'))
        backup, primary = data.get('backup'), data.get('primary')
        if primary.get('staff') is None:
            if len(self.temp_result) > 3000:
                self.temp_result_2 += f"<b>{team.upper()}</b>: is not filled in\n"
            else:
                self.temp_result += f"<b>{team.upper()}</b>: is not filled in\n"
            self.result_dict[team] = f"The timetable of the {team} is not filled in.\n"
            self.result_dict[team] += f"<b>lead > </b> <a href='t.me/{product_owner.login_tg}'>"
            self.result_dict[team] += f"{product_owner.login_tg}</a> {product_owner.login_staff}"
            if team_options.get('chat_link'):
                self.result_dict[team] += f"\nchat > <a href='{team_options.get('chat_link')}'>join</a>"
        else:
            if backup == primary or backup.get('tg') in ['None', '']:
                if len(self.temp_result) > 3000:
                    self.temp_result_2 += f"<b>{team.upper()}</b>: <a href='t.me/{primary.get('tg')}'>{primary.get('staff')}</a>\n"
                else:
                    self.temp_result += f"<b>{team.upper()}</b>: <a href='t.me/{primary.get('tg')}'>{primary.get('staff')}</a>\n"
                self.result_dict[team] = f"prim > @{primary.get('tg')} {primary.get('staff')}"
                self.result_dict[team] += f"\nlead > <a href='t.me/{product_owner.login_tg}'>"
                self.result_dict[team] += f"{product_owner.login_tg}</a> {product_owner.login_staff}"
                duty_ticket = resps.get_duty_ticket(team)
                if team_options.get('chat_link'):
                    self.result_dict[team] += f"\nchat > <a href='{team_options.get('chat_link')}'>join</a>"
                if duty_ticket is not None:
                    self.result_dict[team] += f"\nst.yandex-team.ru/{duty_ticket.lower()}"
            else:
                if len(self.temp_result) > 3000:
                    self.temp_result_2 += f"<b>{team.upper()}</b>: <a href='t.me/{primary.get('tg')}'>{primary.get('staff')}</a> (<a href='t.me/{backup.get('tg')}'>{backup.get('staff')}</a>)\n"
                else:
                    self.temp_result += f"<b>{team.upper()}</b>: <a href='t.me/{primary.get('tg')}'>{primary.get('staff')}</a> (<a href='t.me/{backup.get('tg')}'>{backup.get('staff')}</a>)\n"
                self.result_dict[team] = f"prim > @{primary.get('tg')} {primary.get('staff')}"
                self.result_dict[team] += f"\nback > @{backup.get('tg')} {backup.get('staff')}"
                self.result_dict[team] += f"\nlead > <a href='t.me/{product_owner.login_tg}'>"
                self.result_dict[team] += f"{product_owner.login_tg}</a> {product_owner.login_staff}"
                if team_options.get('chat_link'):
                    self.result_dict[team] += f"\nchat > <a href='{team_options.get('chat_link')}'>join</a>"
                duty_ticket = resps.get_duty_ticket(team)
                if duty_ticket is not None:
                    self.result_dict[team] += f"\nst.yandex-team.ru/{duty_ticket.lower()}"

    def upcoming_duty(self, team: str, length: int) -> str:
        blank = f"Расписание в <b>{team}</b> до {datetime.strftime((datetime.today() + timedelta(length)), '%d.%m.%y')}\n"
        for day in range(0, length+1):
            date = datetime.strftime((datetime.today() + timedelta(day)), "%d-%m-%Y")
            oncall = resps.get_oncall(team, date)
            if oncall.get('primary') is None and oncall != {}:
                if day == 1:
                    return False
                blank += f"\n\nДальше расписание не заполнено"
                return blank
            blank += f"\n[{date}] p:{oncall.get('primary')}, b:{oncall.get('backup')}"
        return blank

    def update_duty(self):
        self.update_cache()
        self.generate_duty()


class Reply:
    __slots__ = ('storage', 'help_reply', 'ciao_reply', 'incidents',
                 'custom_duty_start', 'data', 'help_reply_admin')

    def __init__(self):
        session = boto3.session.Session()
        self.storage = session.client(
            service_name='s3',
            endpoint_url='https://storage.yandexcloud.net'
        )
        self.help_reply, self.ciao_reply = '', ''
        self.update()

    def update(self):
        self.data = json.loads(self.storage.get_object(Bucket='dutybot-reply', Key='reply.json')['Body'].read().decode('utf-8'))
        self.help_reply = self.data.get("help_reply")
        self.ciao_reply = self.data.get("ciao_reply")
        self.help_reply_admin = self.data.get("help_reply_admin")
        self.custom_duty_start = self.data.get("custom_duty_start")
        self.incidents = self.data.get("incidents")


class Emojis(enumerate):
    FIRE = "🔥"
    SUCCESS = "✅"
    FAIL = "❌"
    CLOCK = "🕑"
    HOG = "🐗"
    MAIL = "💌"
    TOOLS = "🛠"
    DYNOMITE = "🧨"
    HEART = "❤️"


class NoActualDuty(Exception):
    pass
