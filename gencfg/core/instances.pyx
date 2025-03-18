# cython: nocheck=True, boundscheck=False, wraparound=False, initializedcheck=False, cdivision=True, language_level=2, infer_types=True, c_string_type=bytes

import hashlib

from core.hosts cimport FakeHost, Host

cimport cython

cdef class FakeInstance:
    cdef public FakeHost host
    cdef public int port
    cdef public float power
    cdef public bytes type
    cdef public str hbf_mtn_addr

    def __cinit__(self, FakeHost host, int port, float power=1.0):
        self.host = host
        self.port = port
        self.power = power
        self.type = b'FAKE'

    def __str__(self):
        return self.full_name()

    cpdef bytes short_name(self):
        return <bytes>('%s:%s' % (self.host.name.partition('.')[0], self.port))

    cpdef bytes name(self):
        return <bytes>('%s:%s' % (self.host.name, self.port))

    cpdef bytes full_name(self):
        return <bytes>('%s:%s:%s:%s' % (self.host.name, self.port, self.power, self.type))

    # FIXME: copy-paste from Instance code
    cpdef bytes get_url(self, int IPv, bytes lua_port_tpl=None, bytes custom_port=None, bytes source_collection=b'yandsearch', bytes proto=b'http'):
        if not custom_port:
            custom_port = str(self.port)
        if lua_port_tpl:
            lua_port_name = lua_port_tpl % {'port': custom_port}
        else:
            lua_port_name = '%s' % custom_port

        return <bytes>('%s://%s:%s/%s' % (proto, self.host.name, lua_port_name, source_collection))


cdef class Instance:
    """
        Class, describing instance. Instance is unique inside db, thus we can not find two instances on same host and same port.
    """

    def __cinit__(self, Host host, float power, int port, bytes type, int N):
        self.host = host
        self.port = port
        self.power = power
        self.type = type
        self.N = N

    def __str__(self):
        return self.full_name()

    def __hash__(self):
        return hash(self.host.name) ^ self.port

    def __richcmp__(self, Instance other, int op):
        if op == 0: # __le__ method
            if self.host.name < other.host.name:
                return True
            elif self.host.name > other.host.name:
                return False
            else:
                if self.port < other.port:
                    return True
                else:
                    return False
        elif op == 2: # __eq__ method
            return self.host.name == other.host.name and self.port == other.port
        else:
            raise NotImplementedError("Operation <%s> is not implemented yet" % op)

    cpdef bytes short_name(self):
        return <bytes>('%s:%s' % (self.host.name.partition('.')[0], self.port))

    cpdef bytes name(self):
        return <bytes>('%s:%s' % (self.host.name, self.port))

    cpdef bytes full_name(self):
        return <bytes>('%s:%s:%s:%s' % (self.host.name, self.port, self.power, self.type))

    cpdef bytes full_shortcut_name(self):
        return <bytes>('%s:%s:%.1f:%s' % (self.host.name.partition('.')[0], self.port, self.power, self.type))

    cpdef swap_data(self, Instance other):
        self.host, other.host = other.host, self.host
        self.power, other.power = other.power, self.power
        self.port, other.port = other.port, self.port
        self.type, other.type = other.type, self.type
        self.N, other.N = other.N, self.N

    cpdef copy_from(self, Instance other):
        self.host = other.host
        self.power = other.power
        self.port = other.port
        self.type = other.type
        self.N = other.N

    cpdef bytes model(self):
        return self.host.model

    cpdef bytes get_url(self, int IPv, bytes lua_port_tpl=None, bytes custom_port=None, bytes source_collection=b'yandsearch', bytes proto=b'http'):
        if not custom_port:
            custom_port = str(self.port)
        if lua_port_tpl:
            lua_port_name = lua_port_tpl % {'port': custom_port}
        else:
            lua_port_name = '%s' % custom_port

        return <bytes>('%s://%s:%s/%s' % (proto, self.get_host_name(), lua_port_name, source_collection))

    cpdef bytes get_host_name(self):
        return self.host.name

cdef class TIntGroup:
    """
        Class, describing one replica of fixed number of shards (ints can be added).
    """

    def __cinit__(self, list basesearchers, list intsearchers, bint check_ints=False):
        self.basesearchers = basesearchers
        self.intsearchers = intsearchers
        self.data_hash = 0
        self._reinit(check_ints=check_ints)

    @property
    def switch(self):
        return self.switch_

    @switch.setter
    def switch(self, value):
        self.switch_ = value

    cpdef reinit(self, bint check_ints=False):
        # we need this wrapper functcion to use lambdas
        self._reinit(check_ints = check_ints)

    cdef _reinit(self, bint check_ints=False):
        cdef long new_data_hash = <long>(hash((tuple([tuple(x) for x in self.basesearchers]), tuple(self.intsearchers))))
        cdef bytes instance_switch

        if self.data_hash == new_data_hash:
            return

        self.data_hash = new_data_hash
        self.power = min(map(lambda x: sum(map(lambda y: y.power, x)), self.basesearchers))
        cdef list base_instances = sum(self.basesearchers, [])

        cdef dict switch_counts = dict() # = defaultdict(int)
        for instance in base_instances:
            instance_switch = instance.host.switch
            if instance_switch not in switch_counts:
                switch_counts[instance_switch] = 0
            switch_counts[instance_switch] += 1

        if len(switch_counts):
            self.switch_ = sorted(switch_counts.items(), key = lambda x: x[1])[0][0]
            basesearcher = filter(lambda x: x.host.switch == self.switch, base_instances)[0]
            self.queue = basesearcher.host.queue
            self.dc = basesearcher.host.dc
            self.single_stoika_group = len(filter(lambda x: x.host.switch == self.switch, self.get_all_basesearchers())) / float(len(self.basesearchers))
        else:
            self.switch_ = bytes('unknown')
            self.queue = bytes('unknown')
            self.dc = bytes('unknown')
            self.single_stoika_group = 1.0

        if check_ints:
            for intsearch in self.intsearchers:
                if (<Instance>intsearch).switch_ != self.switch_:
                    raise Exception("Switch for int %s is not equal to %s" % (intsearch, self.switch_))

    cpdef int get_multiblock_id(self):
        m = hashlib.md5()
        for lst in self.basesearchers:
            for basesearch in <list>(lst):
                m.update((<Instance>basesearch).host.name)

        cdef bytes multiblock_hash = m.hexdigest()
        return int(long(hash(multiblock_hash)) % 123456789)

    cpdef list get_all_intsearchers(self):
        return self.intsearchers

    cpdef list get_all_basesearchers(self):
        return sum(self.basesearchers, [])

    cpdef list get_all_instances(self):
        return self.get_all_intsearchers() + self.get_all_basesearchers()

    cpdef long calculate_unordered_hash(self):
        cdef long m = 0
        for lst in self.basesearchers:
            for basesearch in lst:
                m ^= hash((<Instance>basesearch).host.name)
        return m

    cpdef swap(self, TIntGroup other):
        self.baseseachers, other.basesearchers = other.basesearchers, self.basesearchers
        self.intseachers, other.intsearchers = other.intsearchers, self.intsearchers
        self.power, other.power = other.power, self.power
        self.switch_, other.switch_ = other.switch_, self.switch_
        self.queue, other.queue = other.queue, self.queue
        self.dc, other.dc = other.dc, self.dc
        self.data_hash, other.data_hash = other.data_hash, self.data_hash


cdef class TMultishardGroup:
    """
        Class, describing all replicas of several shards. Consists of array of TIntGroup (every int group considered as a replica,
        thus every N-th basesearch in int groups corresponds to same shard
    """

    def __init__(self):
        self.brigades = []
        self.weight = 0

    cpdef has_intsearch(self, Instance intsearch):
        """
            Auxiliarily function
        """

        for brigade in self.brigades:
            if intsearch in <list>(brigade.intsearchers):
                return True
        return False


cdef class TIntl2Group:
    """
        Top-tier structure, consistinif of array of TMultishardGroup. MultishardGroups describe disjoint set of shards, e. g. first
        MultishardGroup describes shards [0, N), second - [N, 2 * N), ... . In old-style intlookups, we have only one TIntL2Group.
    """

    def __cinit__(self, list multishards=None, list intl2searchers=None):
        self.intl2searchers = []
        if intl2searchers is not None:
            self.intl2searchers = intl2searchers

        self.multishards = []
        if multishards is not None:
            self.multishards = multishards
