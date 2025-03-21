from sqlalchemy import create_engine, Column, Integer, String
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker, scoped_session

Base = declarative_base()


class User(Base):
    """
    Class which represents the User table in users' states db. This is the original class which is used in DbAdapter,
        but custom User class could be inherited from this one and passed as user_class parameter in DbAdapter
        initialization
    """
    __tablename__ = 'user'
    id = Column(Integer, primary_key=True)
    state = Column(String)
    first_name = Column(String)
    last_name = Column(String)
    name = Column(String)
    username = Column(String)


class DbAdapter:
    """
    Class which deals with users' states db. This is the original handler class, but the custom class could be inherited
        from this one. It's important not to override the default methods (get_user, commit_user and init_db) becouse
        they could be used in the default processing
    """

    def __init__(self, db_con_path, user_class=None):
        """
        :param db_con_path: connection string in sqlalchemy style
        :param user_class: custom User class, or None (original class)

        :type db_con_path: str
        :type user_class: User
        """
        self.user_class = User if user_class is None else user_class
        engine = create_engine(db_con_path)
        self.Session = scoped_session(sessionmaker(bind=engine))
        self.engine = engine

    def get_user(self, eff_user):
        """
        Get the user object from db within effective_user object, obtained from telegram update
        :param eff_user:
        :return: User
        """
        user_id = eff_user.id
        user = self.get_user_by_id(user_id)
        if not user:
            user = self.user_class()
            user.id = user_id
            user.first_name = eff_user.first_name
            user.last_name = eff_user.last_name
            user.name = eff_user.name
            user.username = eff_user.username
            session = self.Session()
            session.add(user)
            session.commit()
            # self.Session.remove()
        return user

    def get_user_by_id(self, user_id):
        """
        Get the user object from db within effective_user object, obtained from telegram update
        :param user_id:
        :return: User
        """
        session = self.Session()
        user = session.query(self.user_class).filter(self.user_class.id == user_id).one_or_none()
        session.commit()
        return user

    def commit_user(self, user):
        """
        Commit User object to db
        :param user: user object

        :type user: User
        """
        session = self.Session()
        session.add(user)
        session.commit()
        # self.Session.remove()

    def init_db(self):
        """
        Method for dropping and the initialization the db
        """
        Base.metadata.drop_all(self.engine)
        Base.metadata.create_all(self.engine)
