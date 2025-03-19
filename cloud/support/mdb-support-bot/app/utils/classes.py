import json
import logging
from datetime import (
    datetime,
    timedelta
)
import app.database.cache as cache_orm
import app.database.components as components_orm
import app.database.users as users_orm
import app.utils.staff as staff

class User:

    __slots__ = ('chat_id', 'login_tg', 'login_staff', 'name_staff',
                 'abuse', 'support', 'work_day', 'bot_admin',
                 'allowed', 'cloud', 'duty_notify','incidents')

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
                f'<b>Рабочий день</b> с {self.work_day.split(" ")[0]} до {self.work_day.split(" ")[1]}\n' \
                f'<b>Нотификации об абьюзах</b> {"Нет" if not self.abuse else "Да"}\n' \
                f'<b>Нотификации о саппортских тикетах</b> {"Нет" if not self.support else "Да"}\n' \
                f'<b>Нотификации о ближайших дежурствах</b> {"Нет" if not self.duty_notify else "Да"}\n' \
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

class Duty:
    __slots__ = ('result_dict', 'duty_args', 'raw',
                 'duty_reply', 'result', 'duty_teams',
                 'temp_result', 'teams')

    def __init__(self):
        self.result_dict = {}
        self.duty_args = {}
        self.raw = {}
        self.duty_reply = ""
        self.result = ""
        self.temp_result = ""
        self.teams = ("api-gateway", "console", "docs",
                      "iam", "mdb-clickhouse", "mdb-core",
                      "mdb-mongodb", "mdb-mysql", "mdb-postgres",
                      "mdb-redis", "support", "yql",
                      "clickhouse-dev")
        
    def get_duty_teams(self):
        self.duty_teams = {}
        self.teams = sorted(self.teams)
        for team in self.teams:
            # Match resps data with additional cfg from database
            self.duty_teams[team] = components_orm.get_component(team)
        return self.duty_teams

    def update_cache(self):
        logging.info("[bot_utils.update_cache] Started")
        self.raw = cache_orm.get_all_cache()
        self.raw = {key:self.raw[key] for key in sorted(self.raw.keys())}
        self.duty_teams = self.get_duty_teams()
        for team, data in self.raw.items():
            if team in self.teams:
                self.generate_output_string(team, data)
        self.result = f"{Emojis.TOOLS} Сегодня дежурят: \n{self.temp_result}"
        self.temp_result = ""
        logging.info("[bot_utils.update_cache] Finished")

    def generate_output_string(self, team: str, data: dict) -> set:
        team_options = components_orm.get_component(team)
        product_owner = User(login_staff=team_options.get('component_admin'))
        backup, primary = data.get('backup'), data.get('primary')
        if primary.get('staff') is None:
            self.temp_result += f"<b>{team.upper()}</b>: is not filled in\n"
            self.result_dict[team] = f"The timetable of the {team} is not filled in.\n"
            self.result_dict[team] += f"<b>lead > </b> <a href='t.me/{product_owner.login_tg}'>"
            self.result_dict[team] += f"{product_owner.login_tg}</a> {product_owner.login_staff}"
        else:
            if backup == primary or backup.get('tg') in ['None', '']:
                self.temp_result += f"<b>{team.upper()}</b>: <a href='t.me/{primary.get('tg')}'>{primary.get('staff')}</a>\n"
                self.result_dict[team] = f"prim > @{primary.get('tg')} {primary.get('staff')}"
                self.result_dict[team] += f"\nlead > <a href='t.me/{product_owner.login_tg}'>"
                self.result_dict[team] += f"{product_owner.login_tg}</a> {product_owner.login_staff}"
            else:
                self.temp_result += f"<b>{team.upper()}</b>: <a href='t.me/{primary.get('tg')}'>{primary.get('staff')}</a> (<a href='t.me/{backup.get('tg')}'>{backup.get('staff')}</a>)\n"
                self.result_dict[team] = f"prim > @{primary.get('tg')} {primary.get('staff')}"
                self.result_dict[team] += f"\nback > <a href='t.me/{backup.get('tg')}>{backup.get('staff')}</a>"
                self.result_dict[team] += f"\nlead > <a href='t.me/{product_owner.login_tg}'>"
                self.result_dict[team] += f"{product_owner.login_tg}</a> {product_owner.login_staff}"

    def update_duty(self):
        self.update_cache()


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
