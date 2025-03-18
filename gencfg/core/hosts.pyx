# cython: nocheck=True, boundscheck=False, wraparound=False, initializedcheck=False, cdivision=True, language_level=2, infer_types=True, c_string_type=bytes

import os
import time

# cJSON stuff
cimport cyJSON

try:  # we need this for sky run on remote machine without ujson
    import ujson
except ImportError:
    try:
        import simplejson as ujson
    except ImportError:
        ujson = None

try: # we need this for sky run on remote machine without lz4
    import lz4
except ImportError:
    lz4 = None

try:  # to work with skynet
    import mmh3
except ImportError:
    pass

from core.settings import SETTINGS
from gaux.aux_utils import download_sandbox_resource

from libcpp.string cimport string

cimport cython

cdef extern from "<string>" namespace "std" nogil:
    string to_string(int val) except +
    string to_string(float val) except +
    string to_string(double val) except +

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


cdef extern from "stdlib.h":
    int atoi(char*)

cdef extern from "string.h":
    size_t strlen(const char* s);


# =================================== Serialization primitives START ===========================
cdef inline void serialize_bool(stringstream& s, bint var):
    s.write(<char*>(&var), sizeof(var));
cdef inline bint deserialize_bool(stringstream& s):
    cdef bint var;
    s.read(<char*>(&var), sizeof(var));
    return var;

cdef inline void serialize_int(stringstream& s, int var):
    s.write(<char*>(&var), sizeof(var));
cdef inline int deserialize_int(stringstream& s):
    cdef int var;
    s.read(<char*>(&var), sizeof(var));
    return var;


cdef inline void serialize_long(stringstream& s, int var):
    s.write(<char*>(&var), sizeof(var));
cdef inline long deserialize_long(stringstream& s):
    cdef long var;
    s.read(<char*>(&var), sizeof(var));
    return var;

cdef inline void serialize_float(stringstream& s, float var):
    s.write(<char*>(&var), sizeof(var));
cdef inline float deserialize_float(stringstream& s):
    cdef float var;
    s.read(<char*>(&var), sizeof(var));
    return var;

cdef inline void serialize_pchar(stringstream& s, const char* var):
    cdef int l = strlen(var);
    serialize_int(s, l);
    s.write(var, l);
cdef inline string deserialize_pchar(stringstream& s):
    cdef int l;
    s.read(<char*>(&l), sizeof(l));

    cdef string var;
    var.resize(l);
    s.read(<char*>var.data(), l);

    return var;

# =================================== Serializtion primitives FINISH ============================

cdef bytes detect_location(bytes dc):
    if dc in [b'ugra', b'ugrb', b'iva', b'fol', b'eto', b'myt']:
        return b'msk'
    else:
        return dc


cdef class FakeHost:
    """
        This class emulates <Host> in some cases, when we need something like Host but can not
        create real Host object
    """

    def __cinit__(self, bytes name, ref_host=None):
        self.name = name
        if ref_host is not None:
            self.switch_ = ref_host.switch
            self.queue = ref_host.queue
            self.dc = ref_host.dc
            self.vlans = ref_host.vlans
        else:
            self.switch_ = b'unknown'
            self.queue = b'unknown'
            self.dc = b'unknown'
            self.vlans = None
        self.ssd = 0
        self.disk = 0
        self.memory = 0
        self.flags = 0
        self.ref_host = ref_host

    @property
    def domain(self):
        if self.name.find('.') >= 0:
            return '.' + self.name.partition('.')[2]
        else:
            return ''

    @property
    def switch(self):
        return self.switch_

    @switch.setter
    def switch(self, value):
        self.switch_ = value

    @property
    def location(self):
        return self.ref_host.location

    def __getattr__(self, name):
        return getattr(self.ref_host, name)


cdef enum HostFlags:
    IS_VM_GUEST = 1 << 0 # indicates, that this host is virtual
    IS_HWADDR_GENERATED = 1 << 1 # indicates, that hwaddr is automatically generated (based on host name fqdn)
    IS_IPV6_GENERATED = 1 << 2  # indicates, that ipv6 addr for this host is automatically generated (set True to all priemka instances)

IS_VM_GUEST_CYTHON = HostFlags.IS_VM_GUEST
IS_HWADDR_GENERATED_CYTHON = HostFlags.IS_HWADDR_GENERATED
IS_IPV6_GENERATED_CYTHON = HostFlags.IS_IPV6_GENERATED

cdef class HostFieldInfo:
    """
        Host field info (for update_hosts and web interface)
    """
    cdef public bytes name
    cdef public bytes type
    cdef public object def_value
    cdef public bytes printable_name
    cdef public bytes units
    cdef public bytes group
    cdef public bint primary

    def __cinit__(self, bytes name, bytes type, object def_value, bytes printable_name, bytes units, bytes group, bint primary):
        self.name = name
        self.type = type
        self.def_value = def_value
        self.printable_name = printable_name
        self.units = units
        self.group = group
        self.primary = primary

cpdef list get_host_fields_info():
    """
        Return list of info on all host fields
    """

    return [
        HostFieldInfo(b'name', b'str', b'unknown', b'Host', b'', b'general', True),
        HostFieldInfo(b'domain', b'str', b'', b'Domain', b'', b'general', True),
        HostFieldInfo(b'model', b'str', b'unknown', b'CPU Model', b'', b'hardware', True),
        HostFieldInfo(b'power', b'float', 0., b'Power', b'', b'hardware', False),
        HostFieldInfo(b'ncpu', b'int', 0, b'NCPU', b'', b'hardware', False),
        HostFieldInfo(b'disk', b'int', 0, b'HDD size', b'Gb', b'hardware', True),
        HostFieldInfo(b'ssd', b'int', 0, b'SSD size', b'Gb', b'hardware', True),
        HostFieldInfo(b'memory', b'int', 0, b'Memory', b'Gb', b'hardware', True),
        HostFieldInfo(b'switch', b'str', b'unknown', b'Switch', b'', b'location', True),
        HostFieldInfo(b'queue', b'str', b'unknown', b'Queue', b'', b'location', True),
        HostFieldInfo(b'dc', b'str', b'unknown', b'Data Center', b'', b'location', True),
        HostFieldInfo(b'location', b'str', b'unknown', b'Location', b'', b'location', False),
        HostFieldInfo(b'vlan', b'int', 0, b'VLAN', b'', b'network', True),
        HostFieldInfo(b'ipmi', b'int', 0, b'IPMI', b'', b'hardware', True),
        HostFieldInfo(b'os', b'str', b'unknown', b'OS', b'', b'software', True),
        HostFieldInfo(b'n_disks', b'int', 0, b'Disks #', b'', b'hardware', True),
        HostFieldInfo(b'flags', b'int', 0, b'Flags', b'', b'misc', True),
        HostFieldInfo(b'rack', b'str', b'unknown', b'Rack', b'', b'location', True),
        HostFieldInfo(b'kernel', b'str', b'unknown', b'kernel', b'', b'software', True),
        HostFieldInfo(b'issue', b'str', b'unknown', b'Issue', b'', b'misc', True),
        HostFieldInfo(b'invnum', b'str', b'unknown', b'Inventory number', b'', b'hardware', True),
        HostFieldInfo(b'raid', b'str', b'unknown', b'Raid type', b'', b'hardware', True),
        HostFieldInfo(b'platform', b'str', b'unknown', b'Platform', b'', b'hardware', True),
        HostFieldInfo(b'netcard', b'str', b'unknown', b'Network card', b'', b'hardware', True),
        HostFieldInfo(b'ipv6addr', b'str', b'unknown', b'IPv6 addr', b'', b'software', True),
        HostFieldInfo(b'ipv4addr', b'str', b'unknown', b'IPv6 addr', b'', b'software', True),
        HostFieldInfo(b'botprj', b'str', b'unknown', b'Bot prj (from bot dump)', b'', b'misc', True),
        HostFieldInfo(b'botmem', b'int', 0, b'Memory size in Gb (by bot info)', b'', b'hardware', True),
        HostFieldInfo(b'botdisk', b'int', 0, b'Disk size in Gb (by bot info)', b'', b'hardware', True),
        HostFieldInfo(b'botssd', b'int', 0, b'SSD size in Gb (by bot info)', b'', b'hardware', True),
        HostFieldInfo(b'vlan688ip', b'str', b'unknown', b'Vlan 688 ip addr', b'', b'software', True),
        HostFieldInfo(b'shelves', b'list', [], b'Shelves, battached to machine', b'', b'hardware', True),
        HostFieldInfo(b'golemowners', b'list', [], b'Owners in golem', b'', b'software', True),
        HostFieldInfo(b'ffactor', b'str', b'unknown', b'Form factor (0.5/1/2-unit server)', b'', b'hardware', True),
        HostFieldInfo(b'hwaddr', b'str', b'unknown', b'Host mac address', b'', b'hardware', True),
        HostFieldInfo(b'lastupdate', b'str', b'unknown', b'Last Update', b'', b'misc', True),
        HostFieldInfo(b'unit', b'str', b'unknown', b'Unit in rack', b'', b'location', True),
        HostFieldInfo(b'vmfor', b'str', b'unknown', b'Host machine for virtual', b'', b'hardware', True),
        HostFieldInfo(b'l3enabled', b'bool', False, b'Whether machine in L3 or not', b'', b'hardware', True),
        HostFieldInfo(b'vlans', b'dict', {}, b'Dict with all machine vlans', b'', b'network', True),
        HostFieldInfo(b'walle_tags', b'list', [], b'List with all walle tags (RX-436)', b'', b'hardware', True),
        # storages related info
        HostFieldInfo(b'storages', b'dict', {}, b'Storages on machine', b'', b'hardware', True),
        HostFieldInfo(b'ssd_models', b'list', [], b'List of models on <ssd> partition', b'', b'hardware', False),
        HostFieldInfo(b'ssd_size', b'list', [], b'<Ssd> partition size (in Gb)', b'', b'hardware', False),
        HostFieldInfo(b'ssd_count', b'int', 0, b'<Ssd> disk count', b'', b'hardware', False),
        HostFieldInfo(b'hdd_models', b'list', [], b'List of models on <hdd> partition', b'', b'hardware', False),
        HostFieldInfo(b'hdd_size', b'list', [], b'<Hdd> partition size', b'', b'hardware', False),
        HostFieldInfo(b'hdd_count', b'int', 0, b'<Hdd> disk count', b'', b'hardware', False),
        # extra fields
        HostFieldInfo(b'mtn_fqdn_version', b'int', 0, b'Mtn fqdn generator version (RX-336)', b'', b'misc', True),
        HostFieldInfo(b'net', b'int', 1000, b'Nework io capacity (RX-430) (in MBits/sec)', b'', b'hardware', True),
        # gpu
        HostFieldInfo(b'gpu_count', b'int', 0, b'GPU count', b'', b'hardware', True),
        HostFieldInfo(b'gpu_models', b'list', [], b'GPU models', b'', b'hardware', True),
        # debug info
        HostFieldInfo(b'change_time', b'dict', {}, b'Dict with last change fields', b'', b'misc', True),
    ]

cdef class StorageInfo:
    """
        Class, describing storage info (separate partition on host)
    """

    cdef public bytes name
    cdef public bint rota
    cdef public list models
    cdef public bytes mount_point
    cdef public int size
    cdef public bytes raid_mode

    def __cinit__(self):
        self.name = b''; self.rota = True; self.models = []; self.mount_point = b'';
        self.size = 0; self.raid_mode = b''

    cdef void load_from_cjson(self, bytes name, cyJSON.cJSON* data):
        self.name = name

        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "rota"))
        if item != <cyJSON.cJSON*>0:
            if item.type == 1:
                self.rota = False
            else:
                self.rota = True
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "models"))
        if item != <cyJSON.cJSON*>0:
            subitem = <cyJSON.cJSON*>cyJSON.cJSON_GetArrayItem(item, 0)
            while subitem != <cyJSON.cJSON*>0:
                self.models.append(<bytes>subitem.valuestring)
                subitem = subitem.next
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "mount_point"))
        if item != <cyJSON.cJSON*>0:
            self.mount_point = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "size"))
        if item != <cyJSON.cJSON*>0:
            self.size = <int>item.valueint
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "raid_mode"))
        if item != <cyJSON.cJSON*>0:
            self.raid_mode = <bytes>item.valuestring


    cdef inline bytes save_to_json_string(self):
        """
            Create string with jsoned data
        """

        return <bytes>('{"name": "%s", "rota": %s, "models": %s, "mount_point": "%s", "size": %s, "raid_mode": "%s"}' % (
            self.name, "true" if self.rota else "false", "[" + ", ".join(map(lambda x: '"%s"' % x, self.models)) + "]",
            self.mount_point, self.size, self.raid_mode,))

    cdef inline dict save_to_json_object(self):
        return {
            'name': self.name,
            'rota': self.rota,
            'models': self.models,
            'mount_point': self.mount_point,
            'size': self.size,
            'raid_mode': self.raid_mode,
        }

    cdef inline void serialize(self, stringstream& io):
        cdef bytes model

        serialize_pchar(io, <char*>self.name)
        serialize_bool(io, <bint>self.rota)

        serialize_int(io, <int>(len(self.models)))
        for model in self.models:
            serialize_pchar(io, <char*>model)

        serialize_pchar(io, <char*>self.mount_point)
        serialize_int(io, <int>self.size)
        serialize_pchar(io, <char*>self.raid_mode)

    cdef inline void deserialize(self, stringstream& io):
        self.name = deserialize_pchar(io);
        self.rota = deserialize_bool(io);

        self.models = []
        cdef int l = deserialize_int(io);
        for i in xrange(l):
            self.models.append(<bytes>deserialize_pchar(io));

        self.mount_point = deserialize_pchar(io)
        self.size = deserialize_int(io)
        self.raid_mode = deserialize_pchar(io)

    # pickle function
    def __reduce__(self):
        cdef stringstream io;
        self.serialize(io)
        return (serialize_storage_info, (<bytes>(io.str()), ));

def serialize_storage_info(bytes data):
    cdef stringstream io;
    io.write(<char*>(data), len(data))
    io.seekg(0);

    cdef StorageInfo res = StorageInfo()
    res.deserialize(io)

    return res


cdef class Host:
    @property
    def switch(self):
        return self.switch_

    @switch.setter
    def switch(self, value):
        self.switch_ = value

    def __cinit__(self, bint init_defaults = True, object models = None):
        # initialize with default values
        self.shelves = []
        self.golemowners = []
        self.storages = {}
        self.ssd_models = []
        self.hdd_models = []
        self.gpu_models = []
        self.vlans = {}
        self.walle_tags = []
        self.change_time = {}
        # self.location = b'unknown'; self.ssd_models = []; self.ssd_size = 0; self.ssd_count = 0; self.hdd_models = [];
        # self.hdd_size = 0; self.hdd_count = 0; self.ncpu = 0; self.power = 0.0; self.storages = {};

        if init_defaults:
            self.name = b'unknown'; self.domain = b''; self.model = b'unknown'; self.power = 0.0;
            self.ncpu = 0; self.disk = 0; self.ssd = 0; self.memory = 0; self.switch_ = b'unknown';
            self.queue = b'unknown'; self.dc = b'unknown'; self.location = b'unknown'; self.vlan = 0;
            self.ipmi = 0; self.os = b'unknown'; self.n_disks = 0; self.flags = 0; self.rack = b'unknown';
            self.kernel = b'unknown'; self.issue = b'unknown'; self.invnum = b'unknown'; self.raid = b'unknown';
            self.platform = b'unknown'; self.netcard = b'unknown'; self.ipv6addr = b'unknown';
            self.ipv4addr = b'unknown'; self.botprj = b'unknown'; self.botmem = 0;  self.botdisk = 0;
            self.botssd = 0; self.vlan688ip = b'unknown'; self.shelves = []; self.golemowners = []; self.ffactor = b'unknown';
            self.hwaddr = b'unknown'; self.lastupdate = b'unknown'; self.unit = b'unknown';
            self.vmfor = b'unknown'; self.l3enabled = True; self.vlans = {}; self.storages = {}; self.ssd_models = [];
            self.ssd_size = 0; self.ssd_count = 0; self.hdd_models = []; self.hdd_size = 0;
            self.hdd_count = 0; self.mtn_fqdn_version = 0; self.net = 1000; self.walle_tags = []; self.change_time = {};
            self.gpu_count = 0; self.gpu_models = [];

            if models is not None:
                self.postinit(models)

    cdef void load_from_cjson(self, cyJSON.cJSON* data, object models):
        """
            Create host structure from cjson structure
        """

        # fill all fields
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "name"))
        if item != <cyJSON.cJSON*>0:
            self.name = <bytes>item.valuestring

        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "domain"))
        if item != <cyJSON.cJSON*>0:
            self.domain = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "model"))
        if item != <cyJSON.cJSON*>0:
            self.model = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "disk"))
        if item != <cyJSON.cJSON*>0:
            self.disk = <int>item.valueint
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "ssd"))
        if item != <cyJSON.cJSON*>0:
            self.ssd = <int>item.valueint
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "memory"))
        if item != <cyJSON.cJSON*>0:
            self.memory = <int>item.valueint
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "switch"))
        if item != <cyJSON.cJSON*>0:
            self.switch_ = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "queue"))
        if item != <cyJSON.cJSON*>0:
            self.queue = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "dc"))
        if item != <cyJSON.cJSON*>0:
            self.dc = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "vlan"))
        if item != <cyJSON.cJSON*>0:
            self.vlan = <int>item.valueint
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "ipmi"))
        if item != <cyJSON.cJSON*>0:
            self.ipmi = <int>item.valueint
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "os"))
        if item != <cyJSON.cJSON*>0:
            self.os = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "n_disks"))
        if item != <cyJSON.cJSON*>0:
            self.n_disks = <int>item.valueint
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "flags"))
        if item != <cyJSON.cJSON*>0:
            self.flags = <int>item.valueint
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "rack"))
        if item != <cyJSON.cJSON*>0:
            self.rack = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "kernel"))
        if item != <cyJSON.cJSON*>0:
            self.kernel = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "issue"))
        if item != <cyJSON.cJSON*>0:
            self.issue = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "invnum"))
        if item != <cyJSON.cJSON*>0:
            self.invnum = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "raid"))
        if item != <cyJSON.cJSON*>0:
            self.raid = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "platform"))
        if item != <cyJSON.cJSON*>0:
            self.platform = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "netcard"))
        if item != <cyJSON.cJSON*>0:
            self.netcard = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "ipv6addr"))
        if item != <cyJSON.cJSON*>0:
            self.ipv6addr = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "ipv4addr"))
        if item != <cyJSON.cJSON*>0:
            self.ipv4addr = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "botprj"))
        if item != <cyJSON.cJSON*>0:
            self.botprj = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "botmem"))
        if item != <cyJSON.cJSON*>0:
            self.botmem = <int>item.valueint
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "botdisk"))
        if item != <cyJSON.cJSON*>0:
            self.botdisk = <int>item.valueint
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "botssd"))
        if item != <cyJSON.cJSON*>0:
            self.botssd = <int>item.valueint
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "vlan688ip"))
        if item != <cyJSON.cJSON*>0:
            self.vlan688ip = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "shelves"))
        if item != <cyJSON.cJSON*>0:
            # shelves is merely a list of strings
            subitem = <cyJSON.cJSON*>cyJSON.cJSON_GetArrayItem(item, 0)
            while subitem != <cyJSON.cJSON*>0:
                self.shelves.append(<bytes>subitem.valuestring)
                subitem = subitem.next
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "golemowners"))
        if item != <cyJSON.cJSON*>0:
            # golem owners is merely a list of strings
            subitem = <cyJSON.cJSON*>cyJSON.cJSON_GetArrayItem(item, 0)
            while subitem != <cyJSON.cJSON*>0:
                self.golemowners.append(<bytes>subitem.valuestring)
                subitem = subitem.next
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "ffactor"))
        if item != <cyJSON.cJSON*>0:
            self.ffactor = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "hwaddr"))
        if item != <cyJSON.cJSON*>0:
            self.hwaddr = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "lastupdate"))
        if item != <cyJSON.cJSON*>0:
            self.lastupdate = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "unit"))
        if item != <cyJSON.cJSON*>0:
            self.unit = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "vmfor"))
        if item != <cyJSON.cJSON*>0:
            self.vmfor = <bytes>item.valuestring
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "l3enabled"))
        if item != <cyJSON.cJSON*>0:
            if item.type == 1:
                self.l3enabled = False
            else:
                self.l3enabled = True
        # SWAT-3126 start
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "vlans"))
        if item != <cyJSON.cJSON*>0:
            subitem = <cyJSON.cJSON*>cyJSON.cJSON_GetArrayItem(item, 0)
            while subitem != <cyJSON.cJSON*>0:
                self.vlans[<bytes>subitem.string] = <bytes>subitem.valuestring
                subitem = subitem.next
        # SWAT-3126 finish
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "storages"))
        if item != <cyJSON.cJSON*>0:
            subitem = <cyJSON.cJSON*>cyJSON.cJSON_GetArrayItem(item, 0)
            while subitem != <cyJSON.cJSON*>0:
                storage_info = <StorageInfo>StorageInfo()
                storage_info.load_from_cjson(<bytes>subitem.string, subitem)
                self.storages[storage_info.name] = storage_info
                subitem = subitem.next

        # =============================== RX-336 START ===================================
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "mtn_fqdn_version"))
        if item != <cyJSON.cJSON*>0:
            self.mtn_fqdn_version = <int>item.valueint
        # =============================== RX-336 FINISH ==================================


        # =============================== RX-430 START ===================================
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "net"))
        if item != <cyJSON.cJSON*>0:
            self.net = <int>item.valueint
        # =============================== RX-430 FINISH ==================================

        # =============================== RX-436 START ===================================
        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "walle_tags"))
        if item != <cyJSON.cJSON*>0:
            subitem = <cyJSON.cJSON*>cyJSON.cJSON_GetArrayItem(item, 0)
            while subitem != <cyJSON.cJSON*>0:
                self.walle_tags.append(<bytes>subitem.valuestring)
                subitem = subitem.next
        # =============================== RX-436 FINISH ===================================

        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "change_time"))
        if item != <cyJSON.cJSON*>0:
            subitem = <cyJSON.cJSON*>cyJSON.cJSON_GetArrayItem(item, 0)
            while subitem != <cyJSON.cJSON*>0:
                self.change_time[<bytes>subitem.string] = <bytes>subitem.valuestring
                subitem = subitem.next

        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "gpu_count"))
        if item != <cyJSON.cJSON*>0:
            self.gpu_count = <int>item.valueint

        item = <cyJSON.cJSON*>(cyJSON.cJSON_GetObjectItem(data, "gpu_models"))
        if item != <cyJSON.cJSON*>0:
            subitem = <cyJSON.cJSON*>cyJSON.cJSON_GetArrayItem(item, 0)
            while subitem != <cyJSON.cJSON*>0:
                self.gpu_models.append(<bytes>subitem.valuestring)
                subitem = subitem.next

        self.postinit(models)

    cpdef void postinit(self, object models):
        """
            Calculate some values, which are not stored in db, but can be easily calculated using db data
        """

        if self.name.find('.') < 0:
            self.name = <bytes>('%s%s' % (self.name, self.domain))

        self.location = detect_location(self.dc)

        self.power = models[self.model].power
        self.ncpu = models[self.model].ncpu

        new_storages = dict()
        for k in self.storages:
            v = self.storages[k]
            if isinstance(v, dict):
                new_v = StorageInfo()
                new_v.name = k
                new_v.rota = v['rota']
                new_v.models = v['models']
                new_v.size = v['size']
                new_v.mount_point = v['mount_point']
                new_v.raid_mode = v['raid_mode']
                new_storages[k] = new_v
            else:
                new_storages[k] = v
        self.storages = new_storages

        # set different storages info
        self.ssd_models = self.get_storages(sid='ssd')
        self.ssd_count = len(self.ssd_models)
        if 'ssd' in self.storages:
            self.ssd_size = self.storages['ssd'].size
        else:
            self.ssd_size = 0

        self.hdd_models = self.get_storages(sid='hdd')

        self.hdd_count = len(self.hdd_models)
        if 'hdd' in self.storages:
            self.hdd_size = self.storages['hdd'].size
        else:
            self.hdd_size = 0

    cpdef str get_short_name(self):
        return self.name.partition('.')[0]

    cpdef bytes save_to_json_string(self):
        """
            We have three function to save data. Current function is used to convert data to jsoned string
        """

        # convert name to shortname
        cdef bytes shortname = self.name.partition('.')[0]

        # convert storages to string
        cdef list storages_json_list = []
        for storage_info in self.storages.itervalues():
            storages_json_list.append('"%s": %s' % (storage_info.name, (<StorageInfo>storage_info).save_to_json_string()))
        cdef bytes storages_json = <bytes>('{%s}' % ', '.join(storages_json_list))

        # convert shelves to string
        cdef list shelves_json_list = []
        for shelf in self.shelves:
            shelves_json_list.append('"%s"' % <bytes>shelf)
        cdef bytes shelves_json = <bytes>('[%s]' % ', '.join(shelves_json_list))

        # convert golemowners to string
        cdef list golemowners_json_list = []
        for golemowner in self.golemowners:
            golemowners_json_list.append('"%s"' % <bytes>golemowner)
        cdef bytes golemowners_json = <bytes>('[%s]' % ', '.join(golemowners_json_list))

        # convert issue to string
        cdef bytes issue_json_string = self.issue.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')

        # convert botprj to string
        cdef bytes botprj_json_string = self.botprj.replace('\\', '\\\\').replace('"', '\"')

        # convert vlans to string
        cdef list vlans_list = []
        for k in sorted(self.vlans.keys()):
            v = self.vlans[k]
            vlans_list.append('"{}": "{}"'.format(k, v))
        cdef vlans_json = <bytes>('{ %s }' % ', '.join(vlans_list))

        # convert wallet tags to string
        cdef list walle_tags_json_list = []
        for walle_tag in self.walle_tags:
            walle_tags_json_list.append('"%s"' % <bytes>walle_tag)
        cdef bytes walle_tags_json = <bytes>('[%s]' % ', '.join(walle_tags_json_list))

        # convert change_time to string
        cdef list change_time_list = []
        for k in sorted(self.change_time.keys()):
            v = self.change_time[k]
            change_time_list.append('"{}": "{}"'.format(k, v))
        cdef change_time_json = <bytes>('{ %s }' % ', '.join(change_time_list))

        # convert gpu_models to string
        cdef list gpu_models_json_list = []
        for gpu_model in self.gpu_models:
            gpu_models_json_list.append('"%s"' % <bytes>gpu_model)
        cdef bytes gpu_models_json = <bytes>('[%s]' % ', '.join(gpu_models_json_list))

        cdef string cs;
        cs.reserve(5000);
        cs.append('{ "name": "'); cs.append(<char*>shortname); cs.append('", "domain": "'); cs.append(<char*>self.domain); cs.append('", "model": "');
        cs.append(<char*>self.model); cs.append('", "disk": '); cs.append(to_string(self.disk)); cs.append(', "ssd": '); cs.append(to_string(self.ssd));
        cs.append(', "memory": '); cs.append(to_string(self.memory)); cs.append(', "switch": "'); cs.append(<char*>self.switch_); cs.append('", "queue": "');
        cs.append(<char*>self.queue); cs.append('", "dc": "'); cs.append(<char*>self.dc); cs.append('", "vlan": '); cs.append(to_string(self.vlan));
        cs.append(', "ipmi": '); cs.append(to_string(self.ipmi)); cs.append(', "os": "'); cs.append(<char*>self.os); cs.append('", "n_disks": ');
        cs.append(to_string(self.n_disks)); cs.append(', "flags": '); cs.append(to_string(self.flags)); cs.append(', "rack": "'); cs.append(<char*>self.rack);
        cs.append('", "kernel": "'); cs.append(<char*>self.kernel); cs.append('", "issue": "'); cs.append(<char*>issue_json_string); cs.append('", "invnum": "');
        cs.append(<char*>self.invnum); cs.append('", "raid": "'); cs.append(<char*>self.raid); cs.append('", "platform": "'); cs.append(<char*>self.platform);
        cs.append('", "netcard": "'); cs.append(<char*>self.netcard); cs.append('", "ipv6addr": "'); cs.append(<char*>self.ipv6addr);
        cs.append('", "ipv4addr": "'); cs.append(<char*>self.ipv4addr); cs.append('", "botprj": "'); cs.append(<char*>botprj_json_string);
        cs.append('", "botmem": '); cs.append(to_string(self.botmem)); cs.append(', "botdisk": '); cs.append(to_string(self.botdisk)); cs.append(', "botssd": ');
        cs.append(to_string(self.botssd)); cs.append(', "vlan688ip": "'); cs.append(<char*>self.vlan688ip); cs.append('", "shelves": '); cs.append(<char*>shelves_json);
        cs.append(', "golemowners": ');
        cs.append(<char*>golemowners_json); cs.append(', "ffactor": "'); cs.append(<char*>self.ffactor); cs.append('", "hwaddr": "'); cs.append(<char*>self.hwaddr);
        cs.append('", "lastupdate": "'); cs.append(<char*>self.lastupdate); cs.append('", "unit": "'); cs.append(<char*>self.unit); cs.append('", "vmfor": "');
        cs.append(<char*>self.vmfor); cs.append('", "l3enabled": '); cs.append(<char*>("true" if self.l3enabled else "false")); cs.append(', "vlans": ');
        cs.append(<char*>vlans_json); cs.append(', "storages": '); cs.append(<char*>storages_json); cs.append(', "mtn_fqdn_version": ');
        cs.append(to_string(self.mtn_fqdn_version)); cs.append(', "net": '); cs.append(to_string(self.net)); cs.append(', "walle_tags": ');
        cs.append(<char*>walle_tags_json); cs.append(', "change_time": '); cs.append(<char*>change_time_json); cs.append(', "gpu_count": '); cs.append(to_string(self.gpu_count));
        cs.append(', "gpu_models": '); cs.append(<char*>gpu_models_json); cs.append(' }');

        return <bytes>cs

    cpdef dict save_to_json_object(self):
        """
            Auxiliary function (when we need presention of our object as json)
        """
        storages_info = dict()
        for k, v in self.storages.iteritems():
            storages_info[k] = (<StorageInfo>v).save_to_json_object()

        res = {
            'name': self.name.partition('.')[0], 'domain': self.domain, 'model': self.model, 'power': self.power, 'ncpu': self.ncpu,
            'disk': self.disk, 'ssd': self.ssd, 'memory': self.memory, 'switch': self.switch_, 'queue': self.queue,
            'dc': self.dc, 'location': self.location, 'vlan': self.vlan, 'ipmi': self.ipmi, 'os': self.os, 'n_disks': self.n_disks,
            'flags': self.flags, 'rack': self.rack, 'kernel': self.kernel, 'issue': self.issue, 'invnum': self.invnum,
            'raid': self.raid, 'platform': self.platform, 'netcard': self.netcard, 'ipv6addr': self.ipv6addr, 'ipv4addr': self.ipv4addr,
            'botprj': self.botprj, 'botmem': self.botmem, 'botdisk': self.botdisk, 'botssd': self.botssd, 'vlan688ip': self.vlan688ip, 'shelves': self.shelves,
            'golemowners': self.golemowners, 'ffactor': self.ffactor, 'hwaddr': self.hwaddr, 'lastupdate': self.lastupdate,
            'unit': self.unit, 'vmfor': self.vmfor, 'l3enabled': self.l3enabled, 'vlans': self.vlans, 'storages': storages_info, 'ssd_models': self.ssd_models,
            'ssd_size': self.ssd_size, 'ssd_count': self.ssd_count, 'hdd_models': self.hdd_models, 'hdd_size': self.hdd_size,
            'hdd_count': self.hdd_count, 'mtn_fqdn_version': self.mtn_fqdn_version, 'net': self.net, 'walle_tags': self.walle_tags, 'change_time': self.change_time,
            'gpu_count': self.gpu_count, 'gpu_models': self.gpu_models,
        }

        return res

    cdef inline void serialize(self, int version, stringstream& io):
        serialize_pchar(io, <char*>self.name); serialize_pchar(io, <char*>self.domain); serialize_pchar(io, <char*>self.model); serialize_float(io, <float>self.power);
        serialize_int(io, <int>self.ncpu); serialize_int(io, <int>self.disk); serialize_int(io, <int>self.ssd); serialize_int(io, <int>self.memory);
        serialize_pchar(io, <char*>self.switch_); serialize_pchar(io, <char*>self.queue); serialize_pchar(io, <char*>self.dc); serialize_pchar(io, <char*>self.location);
        serialize_int(io, <int>self.vlan); serialize_int(io, <int>self.ipmi); serialize_pchar(io, <char*>self.os); serialize_int(io, <int>self.n_disks);
        serialize_int(io, <int>self.flags); serialize_pchar(io, <char*>self.rack); serialize_pchar(io, <char*>self.kernel); serialize_pchar(io, <char*>self.issue);
        serialize_pchar(io, <char*>self.invnum); serialize_pchar(io, <char*>self.raid); serialize_pchar(io, <char*>self.platform); serialize_pchar(io, <char*>self.netcard);
        serialize_pchar(io, <char*>self.ipv6addr); serialize_pchar(io, <char*>self.ipv4addr); serialize_pchar(io, <char*>self.botprj); serialize_int(io, <int>self.botmem);
        serialize_int(io, <int>self.botdisk); serialize_int(io, <int>self.botssd); serialize_pchar(io, <char*>self.vlan688ip);

        cdef int l = len(self.shelves)
        serialize_int(io, l);
        for i in xrange(l):
            serialize_pchar(io, <char*>self.shelves[i]);

        l = len(self.golemowners)
        serialize_int(io, l);
        for i in xrange(l):
            serialize_pchar(io, <char*>self.golemowners[i]);

        serialize_pchar(io, <char*>self.ffactor); serialize_pchar(io, <char*>self.hwaddr); serialize_pchar(io, <char*>self.lastupdate); serialize_pchar(io, <char*>self.unit);
        serialize_pchar(io, <char*>self.vmfor); serialize_bool(io, <bint>self.l3enabled);

        # serialize vlans
        l = len(self.vlans)
        serialize_int(io, l)
        for k in sorted(self.vlans.keys()):
            serialize_pchar(io, <char*>k)
            serialize_pchar(io, <char*>self.vlans[k])

        l = len(self.storages);
        serialize_int(io, l);
        for k in sorted(self.storages.keys()):
            serialize_pchar(io, <char*>k);
            (<StorageInfo>self.storages[k]).serialize(io)

        l = len(self.ssd_models)
        serialize_int(io, l)
        for i in xrange(l):
            serialize_pchar(io, <char*>self.ssd_models[i]);
        serialize_int(io, <int>self.ssd_size);
        serialize_int(io, <int>self.ssd_count);

        l = len(self.hdd_models)
        serialize_int(io, l)
        for i in xrange(l):
            serialize_pchar(io, <char*>self.hdd_models[i]);
        serialize_int(io, <int>self.hdd_size)
        serialize_int(io, <int>self.hdd_count)

        if (version is not None) and (version >= 2002045):
            serialize_int(io, <int>self.mtn_fqdn_version);

        if (version is not None) and (version >= 2002046):
            serialize_int(io, <int>self.net);

        if (version is not None) and (version >= 2002050):
            l = len(self.walle_tags)
            serialize_int(io, l);
            for i in xrange(l):
                serialize_pchar(io, <char*>self.walle_tags[i]);

        l = len(self.change_time)
        serialize_int(io, l)
        for k in sorted(self.change_time.keys()):
            serialize_pchar(io, <char*>k)
            serialize_pchar(io, <char*>self.change_time[k])

        serialize_int(io, <int>self.gpu_count)

        l = len(self.gpu_models)
        serialize_int(io, l);
        for i in xrange(l):
            serialize_pchar(io, <char*>self.gpu_models[i]);

    cdef inline void deserialize(self, int version, stringstream& io):
        cdef bytes key
        cdef StorageInfo value

        self.name = deserialize_pchar(io); self.domain = deserialize_pchar(io); self.model = deserialize_pchar(io); self.power = deserialize_float(io);
        self.ncpu = deserialize_int(io); self.disk = deserialize_int(io); self.ssd = deserialize_int(io); self.memory = deserialize_int(io);
        self.switch_ = deserialize_pchar(io); self.queue = deserialize_pchar(io); self.dc = deserialize_pchar(io); self.location = deserialize_pchar(io);
        self.vlan = deserialize_int(io); self.ipmi = deserialize_int(io); self.os = deserialize_pchar(io); self.n_disks = deserialize_int(io);
        self.flags = deserialize_int(io); self.rack = deserialize_pchar(io); self.kernel = deserialize_pchar(io); self.issue = deserialize_pchar(io);
        self.invnum = deserialize_pchar(io); self.raid = deserialize_pchar(io); self.platform = deserialize_pchar(io); self.netcard = deserialize_pchar(io);
        self.ipv6addr = deserialize_pchar(io); self.ipv4addr = deserialize_pchar(io); self.botprj = deserialize_pchar(io); self.botmem = deserialize_int(io);
        self.botdisk = deserialize_int(io); self.botssd = deserialize_int(io); self.vlan688ip = deserialize_pchar(io);

        cdef int l = deserialize_int(io);
        for i in xrange(l):
            self.shelves.append(<bytes>deserialize_pchar(io));

        l = deserialize_int(io);
        for i in xrange(l):
            self.golemowners.append(deserialize_pchar(io));

        self.ffactor = deserialize_pchar(io); self.hwaddr = deserialize_pchar(io); self.lastupdate = deserialize_pchar(io); self.unit = deserialize_pchar(io);
        self.vmfor = deserialize_pchar(io); self.l3enabled = deserialize_bool(io);

        # deserialize vlans
        l = deserialize_int(io)
        for i in xrange(l):
            k = deserialize_pchar(io)
            v = deserialize_pchar(io)
            self.vlans[k] = v

        l = deserialize_int(io);
        for i in xrange(l):
            key = deserialize_pchar(io)
            value = StorageInfo()
            value.deserialize(io)
            self.storages[key] = value

        l = deserialize_int(io)
        for i in xrange(l):
            self.ssd_models.append(deserialize_pchar(io));
        self.ssd_size = deserialize_int(io);
        self.ssd_count = deserialize_int(io);

        l = deserialize_int(io)
        for i in xrange(l):
            self.hdd_models.append(deserialize_pchar(io));
        self.hdd_size = deserialize_int(io)
        self.hdd_count = deserialize_int(io)

        if (version is not None) and (version >= 2002045):
            self.mtn_fqdn_version = deserialize_int(io)

        if (version is not None) and (version >= 2002046):
            self.net = deserialize_int(io)

        if (version is not None) and (version >= 2002050):
            l = deserialize_int(io);
            for i in xrange(l):
                self.walle_tags.append(<bytes>deserialize_pchar(io));

        l = deserialize_int(io)
        for i in xrange(l):
            k = deserialize_pchar(io)
            v = deserialize_pchar(io)
            self.change_time[k] = v

        self.gpu_count = deserialize_int(io)

        l = deserialize_int(io);
        for i in xrange(l):
            self.gpu_models.append(<bytes>deserialize_pchar(io));

    cpdef bint is_vm_guest(self):
        return bool(self.flags & HostFlags.IS_VM_GUEST)

    cpdef bint is_hwaddr_generated(self):
        return bool(self.flags & HostFlags.IS_HWADDR_GENERATED)

    cpdef bint is_ipv6_generated(self):
        return bool(self.flags & HostFlags.IS_IPV6_GENERATED)

    cpdef list get_storages(self, str sid=None):
        cdef list result = []

        if sid is not None:
            if sid in self.storages:
                result.extend(self.storages[sid].models)
        else:
            for v in self.storages.itervalues():
                result.extend(v.models)

        return result

    cpdef long get_avail_memory(self):
        """
            We can not use all host memory. Some memory (3Gb) is needed for system tasks, some other
            memory (proportional to total host memory) is needed by kernel structures

            :return: memory available (in bytes)
        """

        cdef long GB = 1024 * 1024 * 1024
        cdef long result = self.memory * GB

        # 3 GB for system needs on all non-virtual machines
        # update: 5Gb because of task GENCFG-853
        if not self.is_vm_guest():
            result -= 5 * GB

        # kernel reserve some memory for its internal use. Currently this is about 1.5-2% of all host memory
        if not self.is_vm_guest(): # do we have to subtract this memory for guest hosts
            result -= long(self.memory * GB * 0.02)

        return result

    cpdef void swap_data(self, Host other):

        self.name, other.name = other.name, self.name
        self.domain, other.domain = other.domain, self.domain
        self.model, other.model = other.model, self.model
        self.power, other.power = other.power, self.power
        self.ncpu, other.ncpu = other.ncpu, self.ncpu
        self.disk, other.disk = other.disk, self.disk
        self.ssd, other.ssd = other.ssd, self.ssd
        self.memory, other.memory = other.memory, self.memory
        self.switch_, other.switch_ = other.switch_, self.switch_
        self.queue, other.queue = other.queue, self.queue
        self.dc, other.dc = other.dc, self.dc
        self.location, other.location = other.location, self.location
        self.vlan, other.vlan = other.vlan, self.vlan
        self.ipmi, other.ipmi = other.ipmi, self.ipmi
        self.os, other.os = other.os, self.os
        self.n_disks, other.n_disks = other.n_disks, self.n_disks
        self.flags, other.flags = other.flags, self.flags
        self.rack, other.rack = other.rack, self.rack
        self.kernel, other.kernel = other.kernel, self.kernel
        self.issue, other.issue = other.issue, self.issue
        self.invnum, other.invnum = other.invnum, self.invnum
        self.raid, other.raid = other.raid, self.raid
        self.platform, other.platform = other.platform, self.platform
        self.netcard, other.netcard = other.netcard, self.netcard
        self.ipv6addr, other.ipv6addr = other.ipv6addr, self.ipv6addr
        self.ipv4addr, other.ipv4addr = other.ipv4addr, self.ipv4addr
        self.botprj, other.botprj = other.botprj, self.botprj
        self.botmem, other.botmem = other.botmem, self.botmem
        self.botdisk, other.botdisk = other.botdisk, self.botdisk
        self.botssd, other.botssd = other.botssd, self.botssd
        self.vlan688ip, other.vlan688ip = other.vlan688ip, self.vlan688ip
        self.shelves, other.shelves = other.shelves, self.shelves
        self.golemowners, other.golemowners = other.golemowners, self.golemowners
        self.ffactor, other.ffactor = other.ffactor, self.ffactor
        self.hwaddr, other.hwaddr = other.hwaddr, self.hwaddr
        self.lastupdate, other.lastupdate = other.lastupdate, self.lastupdate
        self.unit, other.unit = other.unit, self.unit
        self.vmfor, other.vmfor = other.vmfor, self.vmfor
        self.l3enabled, other.l3enabled = other.l3enabled, self.l3enabled
        self.vlans, other.vlans = other.vlans, self.vlans
        self.storages, other.storages = other.storages, self.storages
        self.ssd_models, other.ssd_models = other.ssd_models, self.ssd_models
        self.ssd_size, other.ssd_size = other.ssd_size, self.ssd_size
        self.ssd_count, other.ssd_count = other.ssd_count, self.ssd_count
        self.hdd_models, other.hdd_models = other.hdd_models, self.hdd_models
        self.hdd_size, other.hdd_size = other.hdd_size, self.hdd_size
        self.hdd_count, other.hdd_count = other.hdd_count, self.hdd_count
        self.walle_tags, other.walle_tags = other.walle_tags, self.walle_tags
        self.change_time, other.change_time = other.change_time, self.change_time
        self.gpu_count, other.gpu_count = other.gpu_count, self.gpu_count
        self.gpu_models, other.gpu_models = other.gpu_models, self.gpu_models

    # pickle function
    def __reduce__(self):
        cdef stringstream io;
        self.serialize(0, io)
        return (serialize_host, (<bytes>(io.str()), ));

    def __repr__(self):
        return 'Host(' + self.name + ')'


# unpickle function
def serialize_host(bytes data):
    cdef stringstream io;
    io.write(<char*>(data), len(data))
    io.seekg(0);

    cdef Host res = Host()
    res.deserialize(0, io)

    return res


cdef class HostsInfo:
    cdef public object db
    cdef public object hostsfile
    cdef public object configfile
    cdef public object statusfile
    cdef bint modified

    cdef public dict hosts
    cdef list host_names
    cdef public dict fqdn_mapping

    # some cached data
    cdef list _all_locations
    cdef list _all_dcs

    # base_funcs.py backward compability
    cdef public object cpu_models

    def __cinit__(self, object db):
        self.db = db
        self.hostsfile = os.path.join(self.db.HDATA_DIR, 'hosts_data')
        self.configfile = os.path.join(self.db.HDATA_DIR, 'hosts_data.url')  # file with sandbox resource id
        self.statusfile = os.path.join(self.db.HDATA_DIR, 'hosts_data.status')  # status of current hosts_data
        self.modified = False
        self.hosts = dict()
        self.host_names = list()
        self.fqdn_mapping = dict()
        self._all_locations = None
        self._all_dcs = None
        self.cpu_models = self.db.cpumodels.models

        self.load()

    cdef update_from_sandbox(self):
        """Download new hosts_data if needed (RX-142)"""
        # find new resource id
        jsoned = ujson.loads(open(self.configfile).read())
        new_resource_id = jsoned['resource_id']

        # find current resource id
        if (not os.path.exists(self.hostsfile)) or (not os.path.exists(self.statusfile)):
            current_resource_id = None
            hosts_data_modified = False
        else:
            jsoned = ujson.loads(open(self.statusfile).read())
            current_resource_id = jsoned['resource_id']
            hosts_data_modified = (jsoned['resource_hash'] != mmh3.hash(open(self.hostsfile).read()))

        # update resource
        if new_resource_id != current_resource_id:
            if hosts_data_modified:
                raise Exception('Can not switch resource from {} to {}: hosts_data is modified (remove db/hardware_data/hosts_data to drop changes)'.format(
                                current_resource_id, new_resource_id))

            download_sandbox_resource(new_resource_id, self.hostsfile)
            with open(self.statusfile, 'w') as f:
                f.write(ujson.dumps(dict(resource_id=new_resource_id, resource_hash=mmh3.hash(open(self.hostsfile).read()))))

    cpdef bint is_hosts_data_modified(self):
        """Check if hosts_data modified"""
        current_hash = mmh3.hash(open(self.hostsfile).read())
        cached_hash = ujson.loads(open(self.statusfile).read())['resource_hash']

        return current_hash != cached_hash

    cdef load(self):
        cdef cyJSON.cJSON *root = <cyJSON.cJSON*>0
        cdef cyJSON.cJSON * elem = <cyJSON.cJSON*>0
        cdef bytes cached_data = b""
#        cdef list cached_data_list = []
        cdef int l
        cdef stringstream io

        # ============================================ RX-142 START ============================================
        if self.db.version >= '2.2.41':
            self.update_from_sandbox()
        # ============================================ RX-142 FINISH ===========================================

        if self.db.version <= '1.0':
            for line in open(self.hostsfile, 'r').readlines():
                if self.db.version <= '0.5':
                    name, domain, model, disk, ssd, memory, switch, queue, dc, ipmi, host_os, n_disks = line.strip().split(
                        '\t')
                    vlan = 'unknown'
                    domain = '.' + domain.partition('.')[2]
                elif self.db.version <= '0.6.5':
                    name, domain, model, disk, ssd, memory, switch, queue, dc, vlan, ipmi, host_os, n_disks = line.strip().split(
                        '\t')
                elif self.db.version <= '0.8':
                    name, domain, model, disk, ssd, memory, switch, queue, dc, vlan, ipmi, host_os, n_disks, _ = line.strip().split(
                        '\t')
                else:
                    name, domain, model, disk, ssd, memory, switch, queue, dc, vlan, ipmi, host_os, n_disks, _, _2 = line.strip().split(
                        '\t')
                disk = int(disk)
                ssd = int(ssd)

                if self.db.version <= '0.7':
                    memory = int(memory) / 972
                else:
                    memory = int(memory)

                if n_disks == 'unknown':
                    n_disks = 1
                else:
                    n_disks = int(n_disks)

                power = float(self.cpu_models[model].power)

                host = <Host>Host()
                host.name = <bytes>('%s%s' % (name, domain)); host.domain = domain; self.model = model; self.disk = disk; self.ssd = ssd;
                self.memory = memory; self.switch_ = switch; self.queue = queue; self.dc = dc; self.vlan = vlan;
                self.os = host_os; self.n_disks = n_disks; self.power = power
                host.postinit(self.cpu_models)

                self.hosts[host.name] = host
                self.host_names.append(host.name)
        else:
            cached_data = self.db.cacher.try_load([self.hostsfile])
            db_version_int = self.db.version.asint()
            if cached_data is not None:
                cached_data = lz4.loads(cached_data)

                io.write(<char*>(cached_data), len(cached_data))
                io.seekg(0);

                l = deserialize_int(io)
                for i in xrange(l):
                    host = Host(init_defaults = False)
                    host.deserialize(db_version_int, io)

                    self.hosts[host.name] = host
                    self.host_names.append(host.name)
            else:
                # ======================== GENCFG-667 START =================================
                content = open(self.hostsfile, 'r').read()
                if self.db.version >= '2.2.32':
                    content = lz4.loads(content)
                # ======================== GENCFG-667 FINISH ================================

                # load using cJSON
                root = cyJSON.cJSON_Parse(content)

                serialize_int(io, cyJSON.cJSON_GetArraySize(root));

                elem = cyJSON.cJSON_GetArrayItem(root, 0)
                while elem != <cyJSON.cJSON*>0:
                    host = <Host>Host()
                    host.load_from_cjson(elem, self.cpu_models)

                    self.hosts[host.name] = host
                    self.host_names.append(host.name)
                    host.serialize(db_version_int, io)

                    elem = elem.next

                # save cache
                cached_data = lz4.dumps(<bytes>io.str())
                self.db.cacher.save([self.hostsfile], cached_data)
                self.db.cacher.update()

        self.host_names.sort()  # host_names should be sorted for bisect operations
        for hostname in self.hosts:
            self.fqdn_mapping[<str>hostname.partition('.')[0]] = hostname

    cdef void save(self):
        cdef string cs
        cs.reserve(200 * 1024 * 1024)

        cs.append("[")
        for host_name in self.host_names:
            host = <Host>self.hosts[host_name]
            cs.append(host.save_to_json_string())
            cs.append(",\n")
        cs.resize(cs.size() - 2)
        cs.append("]")

        cdef bytes content
        # ======================== GENCFG-667 START =================================
        if self.db.version >= '2.2.32':
            content = lz4.dumps(<bytes>cs)
        else:
            content = <bytes>cs
        # ======================== GENCFG-667 FINISH ================================

        with open(self.hostsfile, 'w') as f:
            f.write(content)
            f.close()

    cpdef void update(self, bint smart=False):
        if (not smart) or self.modified:
            self.save()

    cpdef list get_all_hosts(self):
        return self.hosts.values()

    cpdef list get_hosts(self):
        return self.hosts.values()

    # ========================== GENCFG-1720 START ====================================
    cpdef list get_vm_hosts(self):
        """Get all portovm hosts"""
        from gaux.aux_portovm import guest_instance

        result = []
        for group in self.db.groups.get_groups():
            if group.has_portovm_guest_group():
                group_vm_hosts = []
                for instance in group.get_kinda_busy_instances():
                    group_vm_hosts.append(guest_instance(instance, db=self.db).host)
                result.extend(group_vm_hosts)

        return result
    # ========================== GENCFG-1720 FINISH ===================================

    # ========================== GENCFG-1852 START ====================================
    cpdef list get_samogon_vm_hosts(self):
        """Get samogon vm hosts"""
        from gaux.aux_portovm import guest_instance

        result = []
        for group in self.db.groups.get_groups():
            if ('samogon' in group.card.tags.prj) and not (group.has_portovm_guest_group()):
                group_vm_hosts = []
                for instance in group.get_kinda_busy_instances():
                    group_vm_hosts.append(guest_instance(instance, db=self.db, raise_non_portovm=False).host)
                result.extend(group_vm_hosts)

        return result
    # ========================== GENCFG-1852 FINISH ===================================

    cpdef list get_hosts_by_name(self, list names):
        cdef list result = []
        for name in names:
            result.append(self.hosts[name])
        return result

    cpdef Host get_host_by_name(self, str name):
        return self.hosts[name]

    cpdef bytes resolve_short_name(self, str shortname):
        if shortname in self.hosts:
            return <bytes>shortname
        return <bytes>(self.fqdn_mapping[shortname])

    cpdef bint has_host(self, str name):
        return name in self.hosts

    cpdef list get_host_names(self):
        return self.host_names[:]

    cpdef void add_host(self, Host host):
        if host.name in self.hosts:
            raise Exception("Trying to add host <%s>, that already in hostlist" % host.name)
        if host.name.lower() != host.name:
            raise Exception(("Cannot add host %s. " % host.name) + "Currently only lower case hosts are supported, " + "frontend requires update for non-lowercase hosts")
        self.hosts[host.name] = host
        self.host_names.append(host.name)

        self.modified = True

    cpdef void remove_host(self, Host host):
        if host.name not in self.hosts:
            raise Exception("Trying to remove non-existing host %s" % host.name)
        self.hosts.pop(host.name)
        self.host_names.pop(self.host_names.index(host.name))

        self.modified = True

    cpdef void rename_host(self, Host host, bytes newname):
        oldname = host.name

        self.remove_host(host)

        host.name = newname
        if newname.find('.') >= 0:
            host.domain = '.' + newname.partition('.')[2]
        else:
            host.domain = b''

        self.add_host(host)

        # rename in ghi
        self.db.groups.ghi.rename_host(oldname, newname)

    cpdef list get_all_locations(self):
        """
            Some people need list of all locations. We do not want to calculate it every time,
            so calculate once and use cached value. This approach can easily fail (if added host
            in new location), but this is almost impossable.
        """
        cdef set all_locations = set()

        if self._all_locations is None:
            for host in self.get_hosts():
                all_locations.add(host.location)
            self._all_locations = list(all_locations)

        return self._all_locations

    cpdef list get_all_dcs(self):
        """
            Some people need list of all dcs. We do not want to calculate it every time,
            so calculate once and use cached value. This approach can easily fail (if added host
            in new location), but this is almost impossable.
        """
        cdef set all_dcs = set()

        if self._all_dcs is None:
            for host in self.get_hosts():
                all_dcs.add(host.dc)
            self._all_dcs = list(all_dcs)

        return self._all_dcs

    cpdef void fast_check(self, int timeout):
        # if we were created => we are ok
        # but, as noone check logic, let's try to get real objects
        self.get_hosts()

    cpdef void mark_as_modified(self):
        self.modified = True
