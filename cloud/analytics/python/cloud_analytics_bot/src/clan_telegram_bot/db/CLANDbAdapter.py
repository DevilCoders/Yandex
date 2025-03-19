import logging

from sqlalchemy import Column, Float, Integer, func, cast, create_engine, String
from sqlalchemy.orm import sessionmaker, scoped_session
from sqlalchemy.ext.declarative import declarative_base
import time

import pandas as pd
from tbc.db_adapter import DbAdapter, User, Base



logger = logging.getLogger(__name__)


class CLANUser(User):
    selected_project = Column(String)
    selected_project_components = Column(String)
    is_subscribed = Column(Integer)


class Projects(Base):
    __tablename__ = 'projects'
    id = Column(Integer, primary_key=True, autoincrement=True)
    name = Column(String)


class TimeAllocation(Base):
    __tablename__ = 'time_allocation'
    id = Column(Integer, primary_key=True, autoincrement=True)
    time = Column(Float)
    user_id = Column(Integer)
    project_name = Column(String)
    components = Column(String)
    hours = Column(Float)

class CLANDbAdapter(DbAdapter):
    def __init__(self, db_con_path, user_class=CLANUser):
        DbAdapter.__init__(self, db_con_path, user_class)
        self.user_class = User if user_class is None else user_class

        engine = create_engine(db_con_path, pool_recycle=3600,  pool_pre_ping=True)
        self.Session = scoped_session(sessionmaker(bind=engine))
        self.engine = engine


    def get_user(self, eff_user):
        user = DbAdapter.get_user(self, eff_user)
        return user

    def get_subscribed_users(self):
        session = self.Session()
        users = session.query(self.user_class) \
            .filter(self.user_class.is_subscribed == 1)
        session.commit()
        return users

    def add_project(self, project_name:str):
        project = Projects()
        project.name = project_name
        session = self.Session()
        session.add(project)
        session.commit()
    
    def get_projects(self):
        session = self.Session()
        projects = session.query(Projects) 
        session.commit()
        return projects

    def update_selected_project(self, user_id, project_name, project_components=None):
        session = self.Session()
        session.query(self.user_class) \
            .filter(self.user_class.id == int(user_id))\
            .update({'selected_project':project_name ,
            'selected_project_components':project_components ,
             })
        session.commit()

    


    def add_time_allocation(self, user_id, project_name, hours, components=None):
        logger.debug(f'Adding {hours} hours for project "{project_name}" for user {user_id}')
        session = self.Session()
        time_allocation = TimeAllocation()
        time_allocation.user_id=int(user_id)
        time_allocation.project_name = project_name
        time_allocation.components = components
        time_allocation.time=time.time()
        time_allocation.hours=hours
        session.add(time_allocation)
        session.commit()
  

    def get_time_allocation(self):
        session = self.Session()
        records = session.query(TimeAllocation).all()
        columns = [col.name for col in TimeAllocation.__mapper__.columns]
        ta_records =  [[getattr(curr, column) for column in columns] for curr in records]
        time_allocation_df = pd.DataFrame.from_records(ta_records, columns=columns)
        session.commit()
        return time_allocation_df

    def get_users_df(self):
        session = self.Session()
        records = session.query(self.user_class).all()
        columns = [col.name for col in self.user_class.__mapper__.columns]
        users_records =  [[getattr(curr, column) for column in columns] for curr in records]
        users = pd.DataFrame.from_records(users_records, columns=columns)
        session.commit()
        return users
