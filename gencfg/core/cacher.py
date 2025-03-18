#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), 'contrib')))

import gc
import time
import cPickle
import cStringIO

try:  # to work with skynet
    import mmh3
except ImportError:
    pass


def fast_pickle_dumps(obj):
    """
        Function for fast pickle. Enables some options to make it as fast as possible

        :type obj: T

        :param obj: arbitrary python obj WITHOUT CYCLE references

        :return str: pickled object
    """
    buff = cStringIO.StringIO()
    pickler = cPickle.Pickler(buff, cPickle.HIGHEST_PROTOCOL)
    pickler.fast = 1
    pickler.dump(obj)
    buff.flush()

    return buff.getvalue()


def fast_pickle_loads(s):
    """
        Function for fast unpickle.

        :type s: str

        :param s: pickled object

        :return (python obj): unpickled object
    """

    gc.disable()  # disabling gc can slightly speed up loading
    try:
        ret = cPickle.loads(s)
    finally:
        gc.enable()

    return ret


class TCacheEntry(object):
    """
        Class, which stores information object hash. Can be constructed from either cached data or object and list of files
    """

    __slots__ = ['parent', 'fnames', 'cachefile', 'hashsum', 'dbversion', 'pickled', 'last_update']

    def __init__(self, parent, cachefile, fnames, obj=None):
        """
            Init function

            :type parent: core.cacher.TCacher
            :type cachefile: str
            :type fnames: list[str]
            :type obj: list[str]

            :param parent: parent class (storage of TCacheEntry)
            :param cachefile: file with cached data. Path is relative to db root
            :param fnames: list of files, which are used to construct hashed object. Pathes are relative to db root
            :param obj: obj to be cached
        """

        self.parent = parent
        self.cachefile = cachefile
        self.fnames = fnames

        if obj is not None:
            self.construct_from_obj(obj, fnames)
        else:
            self.construct_from_cache_entry(cachefile)

    def construct_from_obj(self, obj, fnames):
        self.fnames = fnames
        self.hashsum = self.calculate_hash(self.fnames)
        self.dbversion = self.parent.db.version.asstr()
        self.pickled = fast_pickle_dumps(obj)

        self.last_update = int(time.time())

    def construct_from_cache_entry(self, cachefile):
        data = fast_pickle_loads(open(os.path.join(self.parent.db.PATH, cachefile)).read())

        for slotname in self.__slots__:
            if slotname != 'parent':
                setattr(self, slotname, data[slotname])

    def check_and_reset(self):
        cur_hashsum = self.calculate_hash(self.fnames)
        cur_version = self.parent.db.version.asstr()
        if self.hashsum != cur_hashsum or self.dbversion != cur_version:  # something changed in db
            self.pickled = None
            self.last_update = None

    def update(self, obj, write_to_disk=True):
        assert obj is not None

        self.hashsum = self.calculate_hash(self.fnames)
        self.dbversion = self.parent.db.version.asstr()
        self.pickled = fast_pickle_dumps(obj)
        self.last_update = int(time.time())

        if write_to_disk:
            self.write_to_disk()

    def try_load(self):
        if self.pickled is not None:
            return fast_pickle_loads(self.pickled)
        else:
            return None

    def calculate_hash(self, fnames):
        hashsum = 0
        for fname in fnames:
            fname = os.path.join(self.parent.db.get_path(), fname)
            if not os.path.exists(fname):
                raise Exception("File <%s> not found, while calculating hash sum" % fname)
            hashsum |= mmh3.hash128(open(fname).read())
        return hashsum

    def write_to_disk(self):
        d = dict()
        for slotname in self.__slots__:
            if slotname != 'parent':
                d[slotname] = getattr(self, slotname)
        with open(os.path.join(self.parent.db.PATH, self.cachefile), 'w') as f:
            f.write(cPickle.dumps(d))


class TCacher(object):
    def __init__(self, db):
        self.db = db

        cachedir = os.path.join(self.db.CACHE_DIR, 'all.cache')
        if not os.path.exists(cachedir):
            os.makedirs(cachedir)
        self.cachedir = self.normalize_fnames([cachedir])[0]

        self.cachedata = dict()

    def save(self, fnames, obj):
        fnames = self.normalize_fnames(fnames)
        cachefile = self.get_cache_file_name(fnames)
        entry = TCacheEntry(self, cachefile, fnames, obj=obj)
        entry.write_to_disk()
        self.cachedata[cachefile] = entry

    def try_load(self, fnames, objfunc=None):
        fnames = self.normalize_fnames(fnames)
        cachefile = self.get_cache_file_name(fnames)

        if os.path.exists(os.path.join(self.db.PATH, cachefile)):
            entry = TCacheEntry(self, cachefile, fnames)
            entry.check_and_reset()

            if objfunc is not None:
                entry.update(objfunc())
                entry.write_to_disk()

            if entry.pickled is not None:
                self.cachedata[cachefile] = entry
        elif objfunc is not None:
            entry = TCacheEntry(self, cachefile, fnames, obj=objfunc())
            if entry.pickled is not None:
                entry.write_to_disk()
                self.cachedata[cachefile] = entry
        else:
            return None

        return entry.try_load()

    def update(self, smart=False):
        pass  # nothing to do

    def asstr(self):
        iob = cStringIO.StringIO()
        for entry in self.cachedata.itervalues():
            iob.write("Entry %s:\n" % ",".join(entry.fnames))
            iob.write("    cache file: %s\n" % entry.cachefile)
            iob.write("    datahash: %s\n" % entry.hashsum)
            iob.write("    version: %s\n" % entry.dbversion)
            if entry.pickled is not None:
                iob.write("    pickled (%d bytes): %s\n" % (len(entry.pickled), "NotNone"))
                iob.write(
                    "    last_update: %s\n" % (time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(entry.last_update))))
            else:
                iob.write("    pickled: None\n")
                iob.write("    last_update: None\n")

        return iob.getvalue()

    def normalize_fnames(self, fnames):
        result = []
        for fname in fnames:
            apath = os.path.realpath(os.path.abspath(fname))
            if not apath.startswith(self.db.get_path()):
                raise Exception("File <%s> not in directory <%s>" % (apath, self.db.get_path()))
            result.append(apath[len(self.db.get_path()) + 1:])
        return tuple(sorted(result))

    def get_cache_file_name(self, fnames):
        result = 0
        for hashval in map(lambda x: hash(x), fnames):
            result |= hashval

        return os.path.join(self.cachedir, str(result))
