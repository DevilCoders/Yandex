# coding=utf-8

import hashlib
import imp
import logging
import os
import re
import tempfile
from collections import OrderedDict
import weakref


import gaux.aux_utils
import config
import core.cacher
import core.ctypes
import core.hosts
import core.igroups
import core.intlookups
import core.yplookups
import core.itypes
import core.settings
import core.tiers
import core.cpumodels
import core.diskmodels
import core.ipv4tunnels
import core.hbfmacroses
import core.hbfranges
import core.users
import core.dnscache
import core.slbs
import core.abcgroups
import core.staffgroups
import core.dispenser
import searcherlookup
from gaux.aux_utils import mkdirp, retry_urlopen
import gaux.aux_decorators
from svnapi import SvnRepository, NothingToCommitError, ChangesDiscardedOnMerge
from utils.standalone import get_gencfg_repo
from core.settings import SETTINGS
import simplejson

log = logging.getLogger(__name__)

class DBVersion(object):
    __slots__ = ['version']

    def __init__(self, s):
        self.version = map(lambda x: int(x), s.strip().split('.'))

    def __eq__(self, other):
        if isinstance(other, basestring):
            other = create_db_version(other)
        return self.version == other.version

    def __ne__(self, other):
        return not self == other

    def __lt__(self, other):
        if isinstance(other, basestring):
            other = create_db_version(other)
        for i in range(min(len(self.version), len(other.version))):
            if self.version[i] < other.version[i]:
                return True
            if self.version[i] > other.version[i]:
                return False
        if len(self.version) < len(other.version):
            return True
        return False

    def __le__(self, other):
        if isinstance(other, basestring):
            other = create_db_version(other)
        return self < other or self == other

    def __gt__(self, other):
        if isinstance(other, basestring):
            other = create_db_version(other)
        return other < self

    def __ge__(self, other):
        if isinstance(other, basestring):
            other = create_db_version(other)
        return other <= self

    def asstr(self):
        return ".".join(map(lambda x: str(x), self.version))

    def asint(self):
        result = 0
        for elem in self.version:
            result = result * 1000 + elem

        return result


@gaux.aux_decorators.memoize
def create_db_version(s):
    return DBVersion(s)


class DB(object):
    """
    Загружает базу по указанному пути.
    База – это не только данные (groups, intlookups, etc), но и код.
    Код загружается из модулей (пакетов) {self.PATH}/base_funcs и {self.PATH}/dbconstants.
    Если в одном процессе загрузить базы из разных путей, загрузятся разные версии кода.
    """
    # Components order is important. E.g. intlookups should go in prior of groups.
    _COMPONENTS = OrderedDict([
        ('cpumodels', core.cpumodels.TCpuModelsInfo),
        ('diskmodels', core.diskmodels.TDiskModelsInfo),
        ('hosts', core.hosts.HostsInfo),
        ('intlookups', core.intlookups.Intlookups),
        ('yplookups', core.yplookups.YpLookups),
        ('groups', core.igroups.IGroups),
        ('tiers', core.tiers.Tiers),
        ('itypes', core.itypes.ITypes),
        ('ctypes', core.ctypes.CTypes),
        ('cacher', core.cacher.TCacher),
        ('ipv4tunnels', core.ipv4tunnels.TIpv4Tunnels),
        ('hbfmacroses', core.hbfmacroses.HbfMacroses),
        ('users', core.users.Users),
        ('dnscache', core.dnscache.TDnsCache),
        ('slbs', core.slbs.TSlbs),
        ('abcgroups', core.abcgroups.TAbcGroups),
        ('staffgroups', core.staffgroups.TStaffGroups),
        ('settings', core.settings.TDbSettings),
        ('hbfranges', core.hbfranges.HbfRanges),
        ('dispenser', core.dispenser.Dispenser)
    ])

    def __init__(self, path, temporary=False):
        self.__temporary = temporary
        self.enable_smart_update = True
        self.cached_searcherlookup = None
        self.readonly = False
        self.__dbdir_initialized = False

        if isinstance(path, get_gencfg_repo.DbDirHolder):
            assert ((self.__temporary and path.temporary) is False)
            self.PATH = path.path
            self.__db_dir_holder = path
        else:
            self.PATH = path

        # some cached stuff
        self.cached_repo = None

    def get_path(self):
        return os.path.realpath(self.PATH)

    def get_cache_dir_path(self):
        return self.cache_dir_path

    def __init_cache_dir(self):
        self.cache_dir_path = None

        default_cache_dir_path = os.path.join(self.get_path(), "cache")

        if os.path.exists(default_cache_dir_path):
            self.cache_dir_path = default_cache_dir_path
            return

        try:
            mkdirp(default_cache_dir_path)
            self.cache_dir_path = default_cache_dir_path
        except IOError:
            # no permission?
            pass
        if not self.cache_dir_path:
            # just to be able to write somewhere
            self.cache_dir_path = tempfile.mkdtemp()
        assert self.cache_dir_path

    # FIXME : does not work with multithread
    def __init_db_dir(self):
        if self.PATH.startswith("tag@") or self.PATH.startswith("commit@") or self.PATH == 'trunk@':  # FIXME: check if tag@... specifies tag
            self.__temporary = False

            if self.PATH.startswith("tag@"):
                dbholder = get_gencfg_repo.api_main(repo_type='db', tag=self.PATH.partition("@")[2], temporary=True, load_db_cache=True)
            elif self.PATH.startswith("commit@"):
                dbholder = get_gencfg_repo.api_main(repo_type='db', revision=int(self.PATH.partition("@")[2]),
                                                    temporary=True, load_db_cache=True)
            elif self.PATH == 'trunk@':
                url = '{}/trunk/description'.format(SETTINGS.backend.api.url)
                jsoned = simplejson.loads(retry_urlopen(3, url))
                revision = int(re.match('\(trunk, rev. (\d+)\)', jsoned['description']).group(1))
                dbholder = get_gencfg_repo.api_main(repo_type='db', revision=revision, temporary=True, load_db_cache=True)
            else:
                raise Exception("OOPS")

            self.PATH = dbholder.path
            self.__db_dir_holder = dbholder

        self.__dbdir_initialized = True

        self.load_from_path(self.PATH)
        self.version = _get_db_version(self.PATH)
        self.__closed = False
        self.base_funcs = None

        if self.version >= "2.2.8":
            # add dbconstants to components
            module_name = 'dbconstants'
            hashed_module_name = hashlib.sha224(module_name).hexdigest()
            src = imp.load_source(hashed_module_name, os.path.join(self.PATH, '%s.py' % module_name))
            self.constants = src

    def close(self):
        if not self.__dbdir_initialized or self.__closed:
            return

        if self.__temporary:
            gaux.aux_utils.rm_tree(self.PATH)

        self.__closed = True

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        del exc_type, exc_val, exc_tb
        self.close()

    def __del__(self):
        self.close()

    @staticmethod
    def load_local():
        global CURDB
        return CURDB

    def get_repo(self, verbose=False):
        if not self.__dbdir_initialized:
            self.__init_db_dir()
            self.__init_cache_dir()

        if self.cached_repo is None:
            self.cached_repo = SvnRepository(self.PATH, verbose=verbose)

        return self.cached_repo

    def load_from_path(self, path):
        for k in DB._COMPONENTS:
            if k in self.__dict__:
                delattr(self, k)

        self.PATH = os.path.realpath(os.path.abspath(path))
        self.GROUPS_DIR = os.path.join(self.PATH, 'groups')
        self.INTLOOKUP_DIR = os.path.join(self.PATH, 'intlookups')
        self.YPLOOKUP_DIR = os.path.join(self.PATH, 'yplookups')
        self.CONFIG_DIR = os.path.join(self.PATH, 'configs')
        self.HDATA_DIR = os.path.join(self.PATH, 'hardware_data')
        self.GROUPS_DIR = os.path.join(self.PATH, 'groups')
        self.SCHEMES_DIR = os.path.join(self.PATH, 'schemes')
        self.CACHE_DIR = os.path.join(self.PATH, 'cache')

    def __getattr__(self, name):
        if not self.__dbdir_initialized:
            self.__init_db_dir()
            self.__init_cache_dir()

        if name not in DB._COMPONENTS:
            if name not in self.__dict__:
                raise AttributeError("Db component <%s> not found" % name)
            else:
                return getattr(self, name)
        setattr(self, name, DB._COMPONENTS[name](weakref.proxy(self)))

        return getattr(self, name)

    def precalc_caches(self, no_nanny=False, quiet=False):
        for name in DB._COMPONENTS:
            if name == 'histdb':
                continue
            if name == 'nanny_services' and no_nanny:
                continue

            if not quiet:
                import time
                start_time = time.time()

            getattr(self, name)

            if not quiet:
                end_time = time.time()
                print (name, start_time, int(end_time - start_time))


    def build_searcherlookup(self):
        if not self.cached_searcherlookup:
            self.cached_searcherlookup = searcherlookup.Searcherlookup(self)
            self.cached_searcherlookup.generate_searcherlookup()
            assert self.cached_searcherlookup
        return self.cached_searcherlookup

    def reset_searcherlookup(self):
        self.cached_searcherlookup = None

    # TODO: remove in favor of self.save
    def update(self, smart=False):
        if not self.enable_smart_update:
            smart = False

        for k in DB._COMPONENTS:
            component = getattr(self, k, None)
            if component is not None:
                component.update(smart=smart)

        self.enable_smart_update = True

    def save(self, smart=True):
        self.update(smart)

    def sync(self, commit=None):
        """
            Syncs the database with repository. Returns True if the database has been changed.

            :type commit: int

            :param commit: commit to update DB to (if None, update to last commit)
            :return None: all changes are applied in-place
        """

        repo = self.get_repo()
        changed, commits_discarded = False, False

        try:
            changed = repo.sync(commit=commit)
        except ChangesDiscardedOnMerge:
            log.error("Some changes to DB has been discarded on merge.")
            changed, commits_discarded = True, True

        return changed, commits_discarded

    def set_readonly(self, value):
        assert (value in [True, False])
        self.readonly = value

    def is_readonly(self):
        return self.readonly

    def get_commits_count(self):
        return self.db.get_repo().get_commits_count()

    def commit(self, msg, author=None, reset_changes_on_fail=False):
        """Commits all the changes and pushes them to the remote."""

        if self.readonly:
            raise Exception("Trying to commit in readonly repo at %s" % self.get_path())

        repo = self.get_repo()
        changed, commits_discarded = False, False

        try:
            changed = repo.commit(msg, author=author, reset_changes_on_fail=reset_changes_on_fail)
        except NothingToCommitError:
            return changed, commits_discarded
        except ChangesDiscardedOnMerge:
            changed, commits_discarded = True, True

        return changed, commits_discarded

    # TODO: cached/memoized
    # TODO: private self.base_funcs
    def get_base_funcs(self):
        if not self.base_funcs:
            module_name = 'base_funcs'
            hashed_module_name = module_name + '_' + hashlib.sha224(module_name).hexdigest()[:8]
            f, filename, description = imp.find_module(module_name, [self.PATH])
            src = imp.load_module(hashed_module_name, f, filename, description)

            base_funcs = getattr(src, 'IBaseFuncs', None)
            if not base_funcs:
                raise Exception('Could not find IBaseFuncs in module ' + module_name)
            self.base_funcs = base_funcs

        return self.base_funcs

    def purge(self):
        """Purge current loaded to db state"""
        for k in DB._COMPONENTS:
            if k in self.__dict__:
                delattr(self, k)

    def ping_repo(self):
        return self.get_repo().ping()

    def fast_check(self):
        max_check_time = 0.5
        for name in DB._COMPONENTS:
            # TEMPORARY
            if name in ['histdb', 'cacher']:
                continue
            getattr(self, name).fast_check(max_check_time)


def _get_db_version(db_path):
    version_path = os.path.join(db_path, "version")
    if os.path.exists(version_path):
        return DBVersion(open(version_path).read())

    # Available versions:
    #  0.1 - stable-53/r1+ (the firest one scheme)
    #  0.2 - stable-53/r3+ (scheme with fixed msweb power func)
    #  0.2.5 - stable-54/r1 (scheme with sasweb changed)
    #  0.3 - stable-55/r1 (scheme with not limit production instances)
    #  0.5 - stable-55/r11+ (hosts data as text, cpu models as text)
    #  0.6 - stable-57/r1+ (added extra field to hosts_data)
    #  0.6.5 - stable-58/r1+ (changed syntax of prestable_in_prestable_tags)
    #  0.7 - stable-58/r10+ (added extra field to hosts_data)
    #  0.8 - stable-58/r20+ (changed models format)
    #  0.9 - stalbe-58/r23+ (added extra field to hosts_data)
    #  0.9.5 - stable-58/r26+ (some issues with SAS_KIWICALC_NEW_VM)
    #  2.0 - stable-59/r1+ (hosts data and cpu models as yaml)
    #  2.0.1 - stable-59/r3+ (tiny fix with base_funcs)
    #  2.0.2 - stable-59/r15 (changed imtub power)
    #  2.1 - stable-60/r1+ (schemes moved to different location)
    #  2.1.1 - ?
    #  2.1.2 - ?
    #  2.2 - stable-65/r2 (moved base_funcs to data)
    #  2.2.1 - stable-66/r1 (added raid information to hostsdata)
    #  2.2.2 - stable-66/r8 (moved to fqdn instead of short names)
    #  2.2.3 - stable-67/r1 (bases older than 2.2.3 use legacy base_funcs now)
    #  2.2.5 - stable-73/r1 (added ethernet controller model)
    #  2.2.6 - stable-73/r2 (added a_metaprj_<something> to group card)
    #  2.2.7 - stable-76/r11 (use scheme for tiers)
    #  2.2.8 - stable-77/r20 (added dbconstants.py to db dir)
    #  2.2.9 - stable-78/r2 (nothing changed)
    #  2.2.10 - stable-79/r3 (all intlookups converted to json format)
    #  2.2.11 - stable-80/r1 (fixed instance_count function for SAS_WEB_BASE)
    #  2.2.13 - stable-83/r6 (small changes in tiers properties.shardid_format behaviour)
    #  2.2.14 - stable-84/r40 (changes in ByteSize description (old groups cache can not be loaded))
    #  2.2.15 - stable-85/r2 (small refactoring, invalidating cache)
    #  2.2.16 - stable-85/r14 (added ffactor paramater to every host)
    #  2.2.17 - stable-85/r16 (added hwaddr for virtual machines)
    #  2.2.18 - stable-85/r24 (added doc.md with separate documentation of group scheme fields)
    #  2.2.19 - stable-85/r35 (changed intlookups structure: added intl2 section)
    #  2.2.20 - stable-87/r49 (Added checking itypes/ctypes/metaprjs)
    #  2.2.21 - stable-87/r66 (Removed group field automatic)
    #  2.2.22 - stable-87/r67 (Field <memory> in group card replaced by <memory_limit> and <memory_guarantee>)
    #  2.2.23 - stable-90/r45 (Group does not inherit CardNode anymore)
    #  2.2.24 - ??? (Add settings.yaml to db)
    #  2.2.25 - stable-92/r103 (Added <vmfor> in hosts_data)
    #  2.2.26 - stable-92/r108 (Added <l3enabled> in hosts_data)
    #  2.2.32 - stable-93/r28 (hosts_data now in archive instead of raw json)
    #  2.2.33 - ???
    #  2.2.34 - ???
    #  2.2.35 - removed <domain> property from host properties
    #  2.2.37 - added structure with ipv4 tunnels storage
    #  2.2.38 - added structure with hbf macroses
    #  2.2.39 - added structure with users
    #  2.2.40 - added structure with dnscache for virtual machines
    #  2.2.41 - Use hosts_data from sandbox
    #  2.2.42 - Added cache of racktables slbs
    #  2.2.44 - Moved to json group cards (RX-364)
    #  2.2.45 - Added host field <mtn_fqdn_hostname>
    #  2.2.46 - Added host field <net> (RX-430)
    #  2.2.47 - Changed format for ipv4tunnels to support multiple pools (GENCFG-2240)
    #  2.2.48 - Added staffgroups (RX-474)
    #  2.2.49 - Added abcgroups (RX-447)
    #  2.2.50 - Added walle tags to host (RX-436)
    #  2.2.51 - Switch to json files hbfmacroses.yaml and tiers.yaml
    #  2.2.57 – New type of objects: yplookups

    tag_name = SvnRepository(db_path).get_current_tag()
    if tag_name is None:
        raise Exception("DB at <%s> is not tagged." % db_path)

    tag_match = re.search(r"^stable-(\d+)/r(\d+)$", tag_name)
    if not tag_match:
        raise Exception("Invalid tag '{}' for DB at '{}'.".format(tag_name, db_path))

    branch, tag = int(tag_match.group(1)), int(tag_match.group(2))

    if (branch, tag) >= (60, 1):
        version = DBVersion('2.1')
    elif (branch, tag) >= (59, 15):
        version = DBVersion('2.0.2')
    elif (branch, tag) >= (59, 3):
        version = DBVersion('2.0.1')
    elif (branch, tag) >= (58, 35):
        version = DBVersion('2.0')
    elif (branch, tag) >= (58, 26):
        version = DBVersion('0.9.5')
    elif (branch, tag) >= (58, 23):
        version = DBVersion('0.9')
    elif (branch, tag) >= (58, 20):
        version = DBVersion('0.8')
    elif (branch, tag) >= (58, 10):
        version = DBVersion('0.7')
    elif (branch, tag) >= (58, 1):
        version = DBVersion('0.6.5')
    elif (branch, tag) >= (57, 1):
        version = DBVersion('0.6')
    elif (branch, tag) >= (55, 11):
        version = DBVersion('0.5')
    elif (branch, tag) >= (55, 1):
        version = DBVersion('0.3')
    elif (branch, tag) >= (54, 1):
        version = DBVersion('0.2.5')
    elif (branch, tag) >= (53, 3):
        version = DBVersion('0.2')
    elif (branch, tag) >= (53, 1):
        version = DBVersion('0.1')
    else:
        raise Exception("Too old tag '{}' for DB at '{}'.".format(tag_name, db_path))

    return version


_MAINDB = None
CURDB = None
has_werkzeug = None
local = None


def init_CURDB():
    global CURDB
    global has_werkzeug
    global local
    global _MAINDB

    try:
        import werkzeug
        has_werkzeug = True
    except ImportError:
        has_werkzeug = False

    # initial, see below
    _MAINDB = DB(config.MAIN_DB_DIR)
    if has_werkzeug:
        # make CURDB thread bounded, each thread will have it is own instance
        if local is None:
            local = werkzeug.Local()

        local.db = _MAINDB

        CURDB = werkzeug.LocalProxy(local, 'db')
    else:
        CURDB = _MAINDB


init_CURDB()


def set_current_thread_CURDB(db):
    global local
    global has_werkzeug
    global CURDB
    if not has_werkzeug:
        raise Exception("Cannot use thread local db: werkzeug is not installed")

    if not hasattr(local, 'db'):
        local.db = db
    elif db != CURDB:
        local.db = db


def get_real_db_object():
    global _MAINDB
    return _MAINDB
