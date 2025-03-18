from core.hosts cimport Host

cdef class Instance:
    """
        Class, describing instance. Instance is unique inside db, thus we can not find two instances on same host and same port.
    """

    cdef public Host host
    cdef public int port
    cdef public float power
    cdef public bytes type
    cdef public int N

    cpdef bytes short_name(self)
    cpdef bytes name(self)
    cpdef bytes full_name(self)
    cpdef bytes full_shortcut_name(self)
    cpdef swap_data(self, Instance other)
    cpdef copy_from(self, Instance other)
    cpdef bytes model(self)
    cpdef bytes get_url(self, int IPv, bytes lua_port_tpl=*, bytes custom_port=*, bytes source_collection=*, bytes proto=*)
    cpdef bytes get_host_name(self)

cdef class TIntGroup:
    """
        Class, describing one replica of fixed number of shards (ints can be added).
    """

    cdef public list basesearchers
    cdef public list intsearchers
    cdef public float power
    cdef public bytes switch_
    cdef public bytes queue
    cdef public bytes dc
    cdef public long data_hash
    cdef public float single_stoika_group

    cpdef reinit(self, bint check_ints=*)
    cdef _reinit(self, bint check_ints=*)
    cpdef int get_multiblock_id(self)
    cpdef list get_all_intsearchers(self)
    cpdef list get_all_basesearchers(self)
    cpdef list get_all_instances(self)
    cpdef long calculate_unordered_hash(self)
    cpdef swap(self, TIntGroup other)

cdef class TMultishardGroup:
    """
        Class, describing all replicas of several shards. Consists of array of TIntGroup (every int group considered as a replica,
        thus every N-th basesearch in int groups corresponds to same shard
    """

    cdef public list brigades
    cdef public float weight

    cpdef has_intsearch(self, Instance intsearch)

cdef class TIntl2Group:
    """
        Top-tier structure, consistinif of array of TMultishardGroup. MultishardGroups describe disjoint set of shards, e. g. first
        MultishardGroup describes shards [0, N), second - [N, 2 * N), ... . In old-style intlookups, we have only one TIntL2Group.
    """

    cdef public list multishards
    cdef public list intl2searchers
