import logging

from sqlalchemy import or_
from sqlalchemy.exc import SQLAlchemyError

from app.database.db import Database as db
from app.database.models import Users

session = db.Session()

"""
ORM CRUD for User-object

    get_all_users()
    Input: kwargs filter for user list
    Output: list with matched users chat_id

    get_user()
    Input: user {staff|chat_id|telegram}
    Output: user object

    delete_user()
    Input: user {staff|chat_id|telegram}
    Output: bool result of operation

    update_user()
    Input: kwargs with data for update
    Output: updated user object

    add_user()
    Input: kwargs with user data
    Output: created user object
"""


def get_all_users(**kwargs):
    if not kwargs:
        try:
            users_list = session.query(Users).all()
            data = [user.chat_id for user in users_list]
            return data
        except Exception as e:
            # session.rollback()
            logging.info(f"[ORM.get_all_users] {e}")
            return []
    else:
        try:
            users_list = session.query(Users).filter_by(**kwargs)
            data = [user.chat_id for user in users_list]
            return data
        except Exception as e:
            # session.rollback()
            logging.info(f"[ORM.get_all_users {kwargs}] {e}")
            return []


def get_user(user):
    try:
        _user = session.query(Users).filter(
            or_(Users.login_staff == user,
                Users.login_tg == user,
                Users.chat_id == user)
                ).first()
        data = _user.__repr__()
        return data
    except Exception as e:
        # session.rollback()
        logging.info(f"[ORM.get_user] {e}")
        return {}


def add_user(**kwargs):
    try:
        _user = Users(**kwargs)
        session.add(_user)
        # session.commit()
        return get_user(_user.login_staff)
    except Exception as e:
        # session.rollback()
        logging.info(f"[ORM.add_user] {e}")
        return {}


def update_user(user, **kwargs):
    try:
        session.query(Users).filter(
            or_(
                Users.login_staff == user,
                Users.login_tg == user,
                Users.chat_id == user)
        ).update(**kwargs)
        # session.commit()
        return get_user(user)
    except Exception as e:
        # session.rollback()
        logging.info(f"[ORM.update_user] {e}")
        return {}


def delete_user(user):
    try:
        result = session.query(Users).filter(
            or_(Users.login_staff == user,
                Users.login_tg == user,
                Users.chat_id == user)
                ).delete()
        # session.commit()
        return bool(result)
    except Exception as e:
        # session.rollback()
        logging.info(f"[ORM.delete_user] {e}")
        return  False
