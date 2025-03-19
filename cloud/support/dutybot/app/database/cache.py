import logging

from sqlalchemy.exc import InvalidRequestError, SQLAlchemyError

from app.database.db import Database as db
from app.database.models import Cache

session = db.Session()

def get_all_cache(**kwargs):
    if not kwargs:
        try:
            _cache = session.query(Cache).all()
            data = {team.team: {
                                'primary': {
                                    'staff': team.main,
                                    'tg': team.main_tg
                                },
                                'backup': {
                                    'staff': team.backup,
                                    'tg': team.backup_tg
                                }
                            }
                     for team in _cache}
            return data
        except Exception as e:
            logging.info(f"[ORM.get_all_cache] {e}")
            # session.rollback()
            data = {}
            return data
    else:
        try:
            _cache = session.query(Cache).filter_by(**kwargs)
            data = [[team.team, team.main, team.backup, team.main_tg, team.backup_tg] for team in _cache]
            return data
        except Exception as e:
            logging.info(f"[ORM.get_all_cache {kwargs}] {e}")
            # session.rollback()
            data = []
            return data

def get_cache(team):
    try:
        _cache = session.query(Cache).filter(Cache.team == team).first()
        data = _cache.__repr__()
        return data
    except SQLAlchemyError as e:
        # session.rollback()
        logging.info(f"[ORM.get_cache] {e}")
        return {}

def update_cache(team, **kwargs):
    try:
        session.query(Cache).filter(Cache.team == team).update(kwargs)
        # session.commit()
        return get_cache(team)
    except Exception as e:
        # session.rollback()
        logging.info(f"[ORM.update_cache] {e}")
        return {}

def add_cache(**kwargs):
    try:
        _cache = Cache(**kwargs)
        session.add(_cache)
        # session.commit()
        return get_cache(_cache.team)
    except Exception as e:
        session.rollback()
        logging.info(f"[ORM.add_cache] {e}")
        return {}

def delete_cache(team):
    try:
        result = session.query(Cache).filter(Cache.team == team).delete()
        # session.commit()
        return bool(result)
    except Exception as e:
        # session.rollback()
        logging.info(f"[ORM.delete_cache] {e}")
        return  False
