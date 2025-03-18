"""
CBB application uses two groups of databases, codenamed "main" and "history"
"main" holds most tables, and "history" holds historical tables (duh).

The reason for this division is historical tables tend to grow very large (one
of them, anyway - HistoryIPV4), and this can cause all manner of problems.
With this design if history database fails, main functionality should remain
unaffected (and history data is collected temporarily in main database).

Furthermore, each database group consists of one master some slaves. Throughout
the code every database-using section declares what database group it needs,
and also if it needs master or slave. (Without such declaration, there is no
database access at all).

Usage:

from antirobot.cbb.cbb_django.cbb.library import db

with db.main.use_slave():
    # Select some stuff from main db

or

@db.history.use_master()
def add_history_entry():
    <...>
"""
import logging
import random
import time
from functools import wraps

import psycopg2
from django.conf import settings
from django.core.cache import caches
from sqlalchemy import MetaData, create_engine, exc, orm
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import Session
from sqlalchemy.util import ThreadLocalRegistry

logger = logging.getLogger(__name__)

ENGINE_MANAGER_CACHE_TIMEOUT = 60

# Constants used to select engines
NONE = 0
MASTER = 1
SLAVES = 2
ANY = MASTER | SLAVES


def engine_from_config(config, hosts, master=True):
    def connect():
        connect_str = " ".join([
            f"host={hosts}",
            f"port={config['PORT']}",
            f"sslmode={config['SSLMODE']}" if config["SSLMODE"] else "",
            f"dbname={config['NAME']}",
            f"user={config['USER']}",
            f"password={config['PASSWORD']}",
            f"sslrootcert={settings.PG_SSLROOTCERT}" if settings.PG_SSLROOTCERT else "",
            "target_session_attrs=read-write" if master else "target_session_attrs=any",
        ])
        if settings.DEBUG_SQL:
            logging.info(connect_str)

        return psycopg2.connect(connect_str)

    return create_engine(
        "postgresql+psycopg2://",
        creator=connect,
        pool_size=5,
        pool_recycle=3600,
        echo=bool(settings.DEBUG_SQL),
    )


def check_engine(engine):
    """
    Return True if database appears to be alive, False otherwise.
    """
    try:
        with engine.begin() as conn:
            conn.execute("SELECT 1;")
    except exc.OperationalError:
        return False
    return True


class DatabaseManagementError(Exception):
    pass


class DatabaseNotConfigured(DatabaseManagementError):
    pass


class DatabaseNotAvailable(DatabaseManagementError):
    pass


class SessionCommitPending(DatabaseManagementError):
    pass


def _localgetter(name):
    def _getter(self):
        return getattr(self.local, name)

    return property(_getter)


class struct(object):
    """ Simple attribute holder object """
    pass


def check_config(conf):
    assert "HOSTS" in conf
    assert "NAME" in conf
    assert "USER" in conf
    assert "SCHEMA" in conf
    # assert "SSLMODE" in conf


def get_hosts(conf):
    return filter(lambda x: x.strip(), conf.get("HOSTS", "").split(","))


class DatabaseManager(object):
    """
    Manager for a single database group: master + some slaves.
    Holds all the stuff typically used by single-db sqlalchemy application:
    session, metadata, base class.

    Provides .use_* decorators/context managers to select if you need a slave or
    a master:

    @manager.use_master()
    @other_manager.use_slave()
    def my_view(request):
       ...

    with session.use_slave():
        ...
    """

    @property
    def local(self):
        return self.registry()

    # Shortcuts

    engines = _localgetter("engines")
    cache = _localgetter("cache")
    selected = _localgetter("selected")
    stack = _localgetter("stack")

    def __init__(self, name, config):
        self.name = name
        self.config = config
        self.meta = MetaData(schema=config["SCHEMA"])
        self.session = orm.scoped_session(
            orm.sessionmaker(
                class_=CbbSession,
                manager=self
            )
        )

        # Base class for models
        class _Base(object):
            query = self.session.query_property()

        self.Base = declarative_base(metadata=self.meta, cls=_Base)

        # This ensures every thread gets its own .local with all attributes
        # using the same mechanism that is used to implement scoped_session.
        # If you need another kind of scoping, you can use
        # sqlalchemy.util.ScopedRegistry too.
        self.registry = ThreadLocalRegistry(self._init_local)

    def _init_local(self):
        local = struct()
        local.engines = engines = {}

        check_config(self.config)
        engines["master"] = engine_from_config(self.config, self.config["HOSTS"], master=True)

        #
        # In PgaaS we don"t know which host is a master.
        # So we just use all of the supplied hosts as slaves
        #
        for i, host in enumerate(get_hosts(self.config)):
            engines["slave_%d" % i] = engine_from_config(self.config, host, master=False)

        local.cache = caches["engine-manager"]

        local.session = orm.scoped_session(orm.sessionmaker(
            class_=CbbSession,
            manager=self))
        local.stack = []

        # Currently selected engine
        local.selected = None

        return local

    def push_stack(self, new_selected):
        self.local.stack.append(self.selected)
        self.local.selected = new_selected

    def pop_stack(self):
        self.local.selected = self.stack.pop()

    def use_db(self, mask, commit_on_exit=False):
        return EngineSelectorContext(manager=self, mask=mask, commit_on_exit=commit_on_exit)

    def use_master(self, commit_on_exit=False):
        return self.use_db(MASTER, commit_on_exit)

    def use_slave(self, commit_on_exit=False):
        return self.use_db(ANY, commit_on_exit)

    def use_only_slaves(self, commit_on_exit=False):
        return self.use_db(SLAVES, commit_on_exit)

    @property
    def master(self):
        return self.engines["master"]

    @property
    def slaves(self):
        return [eng for (name, eng) in self.engines.items() if name != "master"]

    def __getitem__(self, item):
        return self.engines[item]

    #
    # def have_master(self, refresh=False):
    #     """ Tell whether master is available. """
    #     return self.db_statuses(refresh)["master"]

    def db_statuses(self, refresh=False):
        """
        Get database statuses, with cache. If refresh=True, ignore cached values
        and rewrite cache.
        """
        key = "DB_STATUS:%s" % (self.name)

        cached = None
        if not refresh:
            cached = self.cache.get(key)

        if cached is not None:
            return cached["statuses"]

        statuses = self._db_statuses()
        self.cache.set(key, dict(statuses=statuses, ts=int(time.time())), ENGINE_MANAGER_CACHE_TIMEOUT)
        return statuses

    def _db_statuses(self):
        """
        Database statuses (alive or not), no caching
        """
        return {name: check_engine(engine) for name, engine in self.engines.items()}

    def select_engine(self, mask=ANY, ignore_cache=False):
        """
        Return a name of appropriate and available engine.
        """
        if mask == NONE:
            return

        _statuses = self.db_statuses(refresh=ignore_cache)
        st_master = {"master": _statuses.pop("master")}
        st_slaves = _statuses
        del _statuses

        statuses = {}
        if mask & MASTER:
            statuses.update(st_master)
        if mask & SLAVES:
            statuses.update(st_slaves)

        available = [name for (name, is_available) in statuses.items() if is_available]
        if not available:
            # No suitable engine is available at all, we have nothing to lose
            # Let"s try again, ignoring cache
            if not ignore_cache:
                return self.select_engine(mask, ignore_cache=True)
            else:
                message = "Could not establish any connection with {0.name} database.".format(self)
                logger.error(message)
                raise DatabaseNotAvailable(message)

        engine_name = random.choice(available)
        # Let"s check again - if this engine status is cached as ok, but it
        # became unusable, repeat the whole song and dance.
        if not ignore_cache and not check_engine(self[engine_name]):
            return self.select_engine(mask, ignore_cache=True)

        return engine_name


class SessionRoutingError(Exception):
    pass


class CbbSession(Session):
    """
    Session that knows about a manager and selects engines appropriately.
    """

    def __init__(self, manager, *args, **kwargs):
        self.manager = manager
        super(CbbSession, self).__init__(*args, **kwargs)

    def check_meta(self, mapper, clause):
        """
        Check if session is used with correct tables (history tables should be
        used with history session, others with main session).
        """
        metadatas = set()
        if mapper is not None:
            metadatas.add(mapper.class_.metadata)

        if clause is not None:
            from sqlalchemy.sql.util import find_tables

            metadatas |= {tbl.metadata for tbl in find_tables(clause, include_crud=True)}

        if metadatas != {self.manager.meta}:
            raise SessionRoutingError(
                "Wrong metadata for this session - did you add the object to "
                "the right session? History tables go to history session, "
                "others to main session. Session session session session."
            )

    def get_bind(self, mapper=None, clause=None):

        if not self.manager.selected:
            raise DatabaseNotConfigured(
                "No database has been configured. "
                "Please configure what database to use with `session.use_*` "
                "functions."
            )

        # TODO: do we actually need this part?
        self.check_meta(mapper, clause)

        return self.manager[self.manager.selected]


class EngineSelectorContext(object):
    """
    This thing serves as context manager/decorator returned by use_* functions
    on the manager. The only data it holds is the mask, which is used when
    managed code block is entered.
    """

    def __init__(self, manager, mask, commit_on_exit):
        self.manager = manager
        self.mask = mask  # What engines to use
        self.commit_on_exit = commit_on_exit

    def start(self):
        # This can raise DatabaseNotAvailable, but this is ok, because stack
        # is not yet pushed.
        try:
            engine_name = self.manager.select_engine(self.mask)
        except DatabaseNotAvailable:
            raise

        self.manager.push_stack(engine_name)
        logger.debug("DB: selected %s engine: %s", self.manager.name, engine_name)

    def stop(self):
        self.manager.pop_stack()

    def __enter__(self):
        self.start()

    def __exit__(self, exc_type, exc_val, exc_tb):
        if exc_type:
            self.manager.session.rollback()
        elif self.commit_on_exit:
            self.manager.session.commit()
        self.stop()

    def __call__(self, func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            with self:
                return func(*args, **kwargs)

        return wrapper


main = DatabaseManager("main", settings.MAIN_DATABASE)
history = DatabaseManager("history", settings.HISTORY_DATABASE)
