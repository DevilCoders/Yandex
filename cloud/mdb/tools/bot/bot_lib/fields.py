_DEFAULT_FIELDS = ["instance_number", "XXCSI_FQDN", "loc_segment2", "UNITSIZE", "CPUMODEL",
                   "NUMCPU", "PHISICALCORES", "RAMSLOTS", "RAMTYPE", "DISKSLOTS", "DISKINTERFACE", "DISKFORM_INCH"]

DEFAULT_CALC_FIELDS = ["SSD_COUNT", "SSD_TOTAL", "HDD_TOTAL", "HDD_COUNT", "SSD_LIST", "HDD_LIST", "RAM_TOTAL", "RAM_LIST"]

__ATTRS = """
SRV.SERVERS:
| ATTRIBUTE11             | VENDOR               |
| ATTRIBUTE12             | MODEL                |
| ATTRIBUTE13             | CHASSISMODEL         |
| ATTRIBUTE14             | UNITSIZE             |
| ATTRIBUTE15             | IPMI                 |
| ATTRIBUTE16             | NUMCPU               |
| ATTRIBUTE17             | CPUMODEL             |
| ATTRIBUTE18             | RAMSLOTS             |
| ATTRIBUTE19             | RAMTYPE              |
| ATTRIBUTE20             | DISKSLOTS            |
| ATTRIBUTE21             | DISKINTERFACE        |
| ATTRIBUTE22             | DISKFORM_INCH        |
| ATTRIBUTE23             | MB                   |
| ATTRIBUTE24             | CHIPSET              |
| ATTRIBUTE25             | NETWORK_CONTROLLER   |
| ATTRIBUTE26             | PHISICALCORES        |
| ATTRIBUTE27             | NUMPSU               |
| ATTRIBUTE28             | TDP_W                |
| ATTRIBUTE29             | RPS                  |
| ATTRIBUTE30             | Disk Swaptype        |
SRV.NODES:
| ATTRIBUTE11             | VENDOR               |
| ATTRIBUTE12             | MODEL                |
| ATTRIBUTE13             | CHASSISMODEL         |
| ATTRIBUTE14             | UNITSIZE             |
| ATTRIBUTE15             | UNITSIZEH            |
| ATTRIBUTE16             | IPMI                 |
| ATTRIBUTE17             | NUMCPU               |
| ATTRIBUTE18             | CPUMODEL             |
| ATTRIBUTE19             | RAMSLOTS             |
| ATTRIBUTE20             | RAMTYPE              |
| ATTRIBUTE21             | DISKSLOTS            |
| ATTRIBUTE22             | DISKINTERFACE        |
| ATTRIBUTE23             | DISKFORM_INCH        |
| ATTRIBUTE24             | MB                   |
| ATTRIBUTE25             | CHIPSET              |
| ATTRIBUTE26             | NETWORK_CONTROLLER   |
| ATTRIBUTE27             | PHISICALCORES        |
| ATTRIBUTE28             | TDP_W                |
| ATTRIBUTE29             | RPS                  |
| ATTRIBUTE30             | Disk Swaptype        |

SRV.DISKDRIVES:
| ATTRIBUTE12             | MODEL                |
| ATTRIBUTE13             | DISKFORM_INCH        |
| ATTRIBUTE14             | DISKCAPACITY_GB      |
| ATTRIBUTE15             | DISKINTERFACE        |
| ATTRIBUTE16             | DISKTYPE             |
| ATTRIBUTE17             | ROTATESPEED_RPM      |
| ATTRIBUTE18             | TDP_W                |
| ATTRIBUTE11             | VENDOR               |
| ATTRIBUTE19             | DISKPERFORMANCE      |

SRV.RAM:
| ATTRIBUTE11             | VENDOR               |
| ATTRIBUTE12             | MODEL                |
| ATTRIBUTE13             | RAMSIZE_GB           |
| ATTRIBUTE14             | RAMTYPE              |
| ATTRIBUTE15             | TRANSFERRATE_MBS     |
| ATTRIBUTE16             | TDP_W                |
-
"""

ATTRS = {}
DISK_DRIVERS = {}
SERVERS = {}
RAM = {}
NODES = {}

C = "Components"
C_TYPE_FIELD = "item_segment3"
C_DISKDRIVES = "DISKDRIVES"
C_RAM = "RAM"
C_SERVERS = "SERVERS"
C_NODES = "NODES"

SRV_HUMAN_ATTRS = {
    "instance_number": "INV",
    "XXCSI_FQDN": "FQDN",
    "loc_segment2": "DC",
    "item_segment3": "ITYPE"
}

REV_SRV_HUMAN_ATTRS = {v: k for (k, v) in SRV_HUMAN_ATTRS.items()}

DEFAULT_FIELDS = list(SRV_HUMAN_ATTRS.values()) + [f for f in _DEFAULT_FIELDS if f not in SRV_HUMAN_ATTRS.keys()]


def __load_disk_drivers(attrs):
    dds = attrs[C_DISKDRIVES]
    for k, v in dds.items():
        DISK_DRIVERS[v] = k


def __load_servers(attrs):
    servers = attrs[C_SERVERS]
    for k, v in servers.items():
        SERVERS[v] = k


def __load_ram(attrs):
    ram = attrs[C_RAM]
    for k, v in ram.items():
        RAM[v] = k


def __load_nodes(attrs):
    nodes = attrs[C_NODES]
    for k, v in nodes.items():
        NODES[v] = k


def __load_attrs():
    lst = __ATTRS.splitlines()
    prefix = "SRV."
    plen = len(prefix)
    current_key = None
    segment = None
    for i in range(0, len(lst)):
        l = lst[i]
        lo = l.strip()
        if len(lo) == 0:
            if i == len(lst) - 1:
                ATTRS[current_key] = segment
            continue
        if lo.startswith("SRV"):
            if ':' not in lo:
                raise Exception("Invalid format of attributes {}: line {}".format(i, l))
            if segment is not None:
                ATTRS[current_key] = segment
            idx = lo.index(':')
            current_key = lo[plen:idx]
            segment = {}
            continue
        if lo.startswith('|'):
            aline = lo.split('|')
            if len(aline) != 4:
                raise Exception("Invalid line {}: {}".format(i, l))
            segment[aline[1].strip().lower()] = aline[2].strip()
            continue
        if lo == "-":
            ATTRS[current_key] = segment
            break
        raise Exception("Invalid line {}: {}".format(i, l))

    __load_disk_drivers(ATTRS)
    __load_servers(ATTRS)
    __load_ram(ATTRS)
    __load_nodes(ATTRS)


__load_attrs()
