import logging

from sqlalchemy.exc import SQLAlchemyError

from app.database.db import Database as db
from app.database.models import Components

session = db.Session()

def get_all_components(**kwargs):
    if not kwargs:
        try:
            _cache = session.query(Components).all()
            data = [
                [component.component_name, component.component_admin, component.rotate_time,
                component.custom_notify, component.custom_dict] for component in _cache
                ]
            return data
        except Exception as e:
            session.rollback()
            logging.info(f"[ORM.get_all_components] {e}")
            return []
    else:
        try:
            _cache = session.query(Components).filter_by(**kwargs)
            data = [
                [component.component_name, component.component_admin, component.rotate_time,
                component.custom_notify, component.custom_dict] for component in _cache
                ]
            return data
        except Exception as e:
            session.rollback()
            logging.info(f"[ORM.get_all_components {kwargs}] {e}")
            return []

def get_component(name):
    try:
        _component = session.query(Components).filter(Components.component_name == name).first()
        data = _component.__repr__()
        if data == 'None':
            return {}
        return data
    except Exception as e:
        logging.info(f"[ORM.get_component] {e}")
        session.rollback()
        data = {}
        return data

def update_component(name, **kwargs):
    try:
        session.query(Components).filter(Components.name == name).update(kwargs)
        session.commit()
        return get_component(name)
    except Exception as e:
        session.rollback()
        logging.info(f"[ORM.update_component] {e}")
        return {}

def add_component(**kwargs):
    try:
        _component = Components(**kwargs)
        session.add(_component)
        session.commit()
        return get_component(_component.component_name)
    except Exception as e:
        session.rollback()
        logging.info(f"[ORM.add_component] {e}")
        return {}

def delete_component(name):
    try:
        result = session.query(Components).filter(Components.component_name == name).delete()
        session.commit()
        return bool(result)
    except Exception as e:
        session.rollback()
        logging.info(f"[ORM.delete_component] {e}")
        return  False
