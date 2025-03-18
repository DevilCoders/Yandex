cimport cyJSON

from libcpp.string cimport string

cdef extern from "<ostream>" namespace "std" nogil:
    cdef cppclass ostream:
        ostream()

cdef extern from "<istream>" namespace "std" nogil:
    cdef cppclass istream:
        istream()

cdef extern from "<sstream>" namespace "std" nogil:
    cdef cppclass stringstream:
        stringstream()

        # reading/writing strings
        ostream& write(const char* s, int n);
        istream& read(char* s, int n);

        # other stuff
        string str();
        istream& seekg(int pos);

cdef class FakeHost:
    """
        This class emulates <Host> in some cases, when we need something like Host but can not
        create real Host object
    """

    cdef public bytes name
    cdef bytes switch_
    cdef public bytes queue
    cdef public bytes dc
    cdef public int ssd
    cdef public int disk
    cdef public int memory
    cdef public int flags
    cdef public dict vlans
    cdef public Host ref_host

cdef class Host:
    cdef public bytes name
    cdef public bytes domain
    cdef public bytes model
    cdef public float power
    cdef public int ncpu
    cdef public int disk
    cdef public int ssd
    cdef public int memory
    cdef bytes switch_
    cdef public bytes queue
    cdef public bytes dc
    cdef public bytes location
    cdef public int vlan
    cdef public int ipmi
    cdef public bytes os
    cdef public int n_disks
    cdef public int flags
    cdef public bytes rack
    cdef public bytes kernel
    cdef public bytes issue
    cdef public bytes invnum
    cdef public bytes raid
    cdef public bytes platform
    cdef public bytes netcard
    cdef public bytes ipv6addr
    cdef public bytes ipv4addr
    cdef public bytes botprj
    cdef public int botmem
    cdef public int botdisk
    cdef public int botssd
    cdef public bytes vlan688ip
    cdef public list shelves
    cdef public list golemowners
    cdef public bytes ffactor
    cdef public bytes hwaddr
    cdef public bytes lastupdate
    cdef public bytes unit
    cdef public bytes vmfor
    cdef public bint l3enabled
    cdef public dict vlans
    cdef public dict storages
    cdef public list ssd_models
    cdef public int ssd_size
    cdef public int ssd_count
    cdef public list hdd_models
    cdef public int hdd_size
    cdef public int hdd_count
    cdef public int mtn_fqdn_version
    cdef public int net
    cdef public list walle_tags
    cdef public dict change_time
    cdef public int gpu_count
    cdef public list gpu_models

    cdef void load_from_cjson(self, cyJSON.cJSON* data, object models)
    cpdef void postinit(self, object models)
    cpdef str get_short_name(self)
    cpdef bytes save_to_json_string(self)
    cpdef dict save_to_json_object(self)
    cpdef bint is_vm_guest(self)
    cpdef bint is_hwaddr_generated(self)
    cpdef bint is_ipv6_generated(self)
    cpdef list get_storages(self, str sid=*)
    cpdef long get_avail_memory(self)
    cpdef void swap_data(self, Host other)
    cdef void serialize(self, int version, stringstream& io)
    cdef void deserialize(self, int version, stringstream& io)
