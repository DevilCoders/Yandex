from sqlalchemy import (
    BigInteger,
    Boolean,
    Column,
    ForeignKey,
    String,
    Table,
    Time
)

from app.database.db import Database

class Base():
    def __repr__(self):
        return {key:val for (key, val) in self.__dict__.items() if not key.startswith('_')}


class Users(Base, Database.Base):
    __tablename__ = 'users'

    chat_id = Column(String, primary_key=True)
    login_staff = Column(String)
    login_tg = Column(String)
    name_staff = Column(String)
    work_day = Column(String)
    abuse = Column(Boolean)
    support = Column(Boolean)
    bot_admin = Column(Boolean)
    cloud = Column(Boolean)
    duty_notify = Column(Boolean)
    allowed = Column(Boolean)
    incidents = Column(Boolean)

    def __init__(self, chat_id, login_staff, login_tg,
                 name_staff, work_day, abuse, support,
                 bot_admin, cloud, duty_notify, allowed,
                 incidents):
        self.chat_id = chat_id
        self.login_staff = login_staff
        self.login_tg = login_tg
        self.name_staff = name_staff
        self.work_day = work_day
        self.abuse = abuse
        self.support = support
        self.bot_admin = bot_admin
        self.cloud = cloud
        self.duty_notify = duty_notify
        self.allowed = allowed
        self.incidents = incidents

    def __repr__(self):
        return super().__repr__()


class Chats(Base, Database.Base):
    __tablename__ = 'chats'

    chat_id = Column(String, primary_key=True)
    chat_name = Column(String)
    component = Column(String)
    admin = Column(String)
    incidents = Column(Boolean)
    welcome_message = Column(Boolean)

    def __init__(self, chat_id, chat_name, component,
                 admin, incidents, welcome_message):
        self.chat_id = chat_id
        self.chat_name = chat_name
        self.component = component
        self.admin = admin
        self.incidents = incidents
        self.welcome_message = welcome_message

    def __repr__(self):
        return super().__repr__()


class Cache(Base, Database.Base):
    __tablename__ = 'cache'

    team = Column(String, primary_key=True)
    backup = Column(String)
    main = Column(String)
    main_tg = Column(String)
    backup_tg = Column(String)

    def __init__(self, team, backup, main,
                 main_tg, backup_tg):
        self.team = team
        self.backup = backup
        self.main = main
        self.backup_tg = backup_tg
        self.main_tg = main_tg

    def __repr__(self):
        return super().__repr__()


class Components(Base, Database.Base):
    __tablename__ = 'components'

    component_name = Column(String, primary_key=True)
    component_admin = Column(String)
    rotate_time = Column(String)
    custom_notify = Column(String)
    custom_dict = Column(String)
    chat_link = Column(String)
    map_components = Column(String)

    def __init__(self, component_name, component_admin, rotate_time,
                 custom_notify, custom_dict, map_components, chat_link):
        self.component_name = component_name
        self.component_admin = component_admin
        self.rotate_time = rotate_time
        self.custom_notify = custom_notify
        self.custom_dict = custom_dict
        self.chat_link = chat_link
        self.map_components = map_components

    def __repr__(self):
        return super().__repr__()


class Logs(Base, Database.Base):
    __tablename__ = 'stats'

    team = Column(String, primary_key=True)
    prim = Column(String)
    back = Column(String)
    date = Column(Time)

    def __init__(self, team, prim, back, date):
        self.team = team
        self.prim = prim
        self.back = back
        self.date = date

    def __repr__(self):
        return super().__repr__()
