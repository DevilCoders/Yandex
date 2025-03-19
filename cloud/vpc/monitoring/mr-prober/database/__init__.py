from typing import Dict, Any, Optional

from sqlalchemy import create_engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker

Base = declarative_base()

engine = None
session_maker = sessionmaker(autocommit=False, autoflush=False)


def connect(database_url: str, connect_args: Optional[Dict[str, Any]] = None, **kwargs):
    global engine
    if connect_args is None:
        connect_args = {}
    engine = create_engine(database_url, connect_args=connect_args, **kwargs)
    session_maker.configure(bind=engine)
