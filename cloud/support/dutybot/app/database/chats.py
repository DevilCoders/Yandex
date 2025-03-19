import logging

from sqlalchemy import or_
from sqlalchemy.exc import SQLAlchemyError

from app.database.db import Database as db
from app.database.models import Chats

session = db.Session()

"""
ORM CRUD for Chats-object

    get_all_chatst()
    Input: kwargs filter for chat list
    Output: list with matched chat chat_id

    get_chat()
    Input: chat {chat_id|telegram_name}
    Output: chat object

    delete_chat()
    Input: user {chat_id|telegram_name}
    Output: bool result of operation

    update_chat()
    Input: kwargs with data for update
    Output: updated chat object

    add_chat()
    Input: kwargs with chat data
    Output: created chat object
"""

def get_all_chats(**kwargs):
    if not kwargs:
        try:
            chats_list = session.query(Chats).all()
            return [chat.chat_id for chat in chats_list]
        except Exception as e:
            # session.rollback()
            logging.info(f"[ORM.get_all_chats] {e}")
            return []
    else:
        try:
            chats_list = session.query(Chats).filter_by(**kwargs)
            return [[chat.chat_id, chat.component] for chat in chats_list]
        except Exception as e:
            # session.rollback()
            logging.info(f"[ORM.get_all_chats {kwargs}] {e}")
            return []

def get_chat(chat):
    try:
        _chat = session.query(Chats).filter(
            or_(Chats.chat_id == chat,
                Chats.chat_name == chat)
                ).first()
        data = _chat.__repr__()
        data = False if data == 'None' else data
        return data
    except Exception as e:
        # session.rollback()
        logging.info(f"[ORM.get_chat] {e}")
        return {}

def add_chat(**kwargs):
    try:
        _chat = Chats(**kwargs)
        session.add(_chat)
        # session.commit()
        return get_chat(_chat.chat_id)
    except Exception as e:
        # session.rollback()
        logging.info(f"[ORM.add_chat] {e}")
        return {}

def update_chat(chat, **kwargs):
    try:
        session.query(Chats).filter(
            or_(
                Chats.chat_id == chat,
                Chats.chat_name == chat)
        ).update(kwargs)
        # session.commit()
        return get_chat(chat)
    except Exception as e:
        # session.rollback()
        logging.info(f"[ORM.update_chat] {e}")
        return {}

def delete_chat(chat):
    try:
        result = session.query(Chats).filter(
            or_(Chats.chat_id == chat,
                Chats.chat_name == chat)
                ).delete()
        # session.commit()
        return bool(result)
    except Exception as e:
        # session.rollback()
        logging.info(f"[ORM.delete_chat] {e}")
        return  False
