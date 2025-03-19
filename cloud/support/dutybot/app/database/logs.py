import logging

from sqlalchemy.exc import SQLAlchemyError

from app.database.db import Database as db
from app.database.models import Logs

session = db.Session()

"""Simple-ORM for /duty-mention logs
Might be expanded later to complete CRUD
"""

def add_logs(**kwargs):
    try:
        _logdata = Logs(**kwargs)
        session.add(_logdata)
        # session.commit()
        return True
    except Exception as e:
        # session.rollback()
        logging.info(f"[ORM.add_logs] {e}")
        return False
