from sqlalchemy import create_engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker
from sqlalchemy.pool import NullPool

from app.utils.config import Config


class Database:
    engine = create_engine(f'postgresql://{Config.db_user}:{Config.db_pass}@{Config.db_host}:{Config.db_port}/{Config.db_name}',
    poolclass=NullPool,
    isolation_level="AUTOCOMMIT")
    Session = sessionmaker(bind=engine, autocommit=True)
    Base = declarative_base()