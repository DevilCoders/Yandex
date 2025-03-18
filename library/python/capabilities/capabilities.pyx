# cython: language_level=2
from cpython.exc cimport PyErr_SetFromErrno


cdef extern from "<sys/capability.h>" nogil:
    ctypedef void* cap_t
    ctypedef int cap_flag_t
    ctypedef int cap_flag_value_t
    ctypedef int cap_value_t

    # cap_flag_t
    ctypedef enum:
        _Effective "CAP_EFFECTIVE"
        _Permitted "CAP_PERMITTED"
        _Inheritable "CAP_INHERITABLE"

    # cap_value_t
    ctypedef enum:
        _chown "CAP_CHOWN"
        _dac_override "CAP_DAC_OVERRIDE"
        _dac_read_search "CAP_DAC_READ_SEARCH"
        _fowner "CAP_FOWNER"
        _fsetid "CAP_FSETID"
        _kill "CAP_KILL"
        _setgid "CAP_SETGID"
        _setuid "CAP_SETUID"
        _setpcap "CAP_SETPCAP"
        _linux_immutable "CAP_LINUX_IMMUTABLE"
        _net_bind_service "CAP_NET_BIND_SERVICE"
        _net_broadcast "CAP_NET_BROADCAST"
        _net_admin "CAP_NET_ADMIN"
        _net_raw "CAP_NET_RAW"
        _ipc_lock "CAP_IPC_LOCK"
        _ipc_owner "CAP_IPC_OWNER"
        _sys_module "CAP_SYS_MODULE"
        _sys_rawio "CAP_SYS_RAWIO"
        _sys_chroot "CAP_SYS_CHROOT"
        _sys_ptrace "CAP_SYS_PTRACE"
        _sys_pacct "CAP_SYS_PACCT"
        _sys_admin "CAP_SYS_ADMIN"
        _sys_boot "CAP_SYS_BOOT"
        _sys_nice "CAP_SYS_NICE"
        _sys_resource "CAP_SYS_RESOURCE"
        _sys_time "CAP_SYS_TIME"
        _sys_tty_config "CAP_SYS_TTY_CONFIG"
        _mknod "CAP_MKNOD"
        _lease "CAP_LEASE"
        _audit_write "CAP_AUDIT_WRITE"
        _audit_control "CAP_AUDIT_CONTROL"
        _setfcap "CAP_SETFCAP"
        _mac_override "CAP_MAC_OVERRIDE"
        _mac_admin "CAP_MAC_ADMIN"
        _syslog "CAP_SYSLOG"
        _wake_alarm "CAP_WAKE_ALARM"
        # _block_suspend "CAP_BLOCK_SUSPEND"
        # _audit_read "CAP_AUDIT_READ"
        _LAST_CAP "CAP_LAST_CAP"

    # cap_flag_value_t
    ctypedef enum:
        _Clear "CAP_CLEAR"
        _Set "CAP_SET"

    int cap_free(void *caps)
    int cap_clear(cap_t cap_p)
    cap_t cap_get_proc()
    int cap_set_proc(cap_t caps)

    int cap_get_flag(cap_t caps, cap_value_t value, cap_flag_t flag, cap_flag_value_t *mode)
    int cap_set_flag(cap_t caps, cap_flag_t flag, int ncaps, cap_value_t *values, cap_flag_value_t mode)

    int cap_get_bound(cap_value_t value)
    int cap_drop_bound(cap_value_t value)

    char* cap_to_name(cap_value_t cap)
    char* cap_to_text(cap_t caps, ssize_t *length_p)
    cap_t cap_from_text(const char*)


ctypedef public enum Flag:
    Effective = _Effective
    Permitted = _Permitted
    Inheritable = _Inheritable


ctypedef public enum Value:
    cap_chown = _chown
    cap_dac_override = _dac_override
    cap_dac_read_search = _dac_read_search
    cap_fowner = _fowner
    cap_fsetid = _fsetid
    cap_kill = _kill
    cap_setgid = _setgid
    cap_setuid = _setuid
    cap_setpcap = _setpcap
    cap_linux_immutable = _linux_immutable
    cap_net_bind_service = _net_bind_service
    cap_net_broadcast = _net_broadcast
    cap_net_admin = _net_admin
    cap_net_raw = _net_raw
    cap_ipc_lock = _ipc_lock
    cap_ipc_owner = _ipc_owner
    cap_sys_module = _sys_module
    cap_sys_rawio = _sys_rawio
    cap_sys_chroot = _sys_chroot
    cap_sys_ptrace = _sys_ptrace
    cap_sys_pacct = _sys_pacct
    cap_sys_admin = _sys_admin
    cap_sys_boot = _sys_boot
    cap_sys_nice = _sys_nice
    cap_sys_resource = _sys_resource
    cap_sys_time = _sys_time
    cap_sys_tty_config = _sys_tty_config
    cap_mknod = _mknod
    cap_lease = _lease
    cap_audit_write = _audit_write
    cap_audit_control = _audit_control
    cap_setfcap = _setfcap
    cap_mac_override = _mac_override
    cap_mac_admin = _mac_admin
    cap_syslog = _syslog
    cap_wake_alarm = _wake_alarm
    # cap_block_suspend = _block_suspend
    # cap_audit_read = _audit_read
    cap_last_cap = _LAST_CAP


ctypedef public enum FlagValue:
    Clear = _Clear
    Set = _Set


cdef class Capabilities:
    cdef cap_t caps

    def __cinit__(self):
        cdef cap_t caps = cap_get_proc()
        if <void*>caps == NULL:
            PyErr_SetFromErrno(OSError)
            return

        self.caps = caps

    def __dealloc__(self):
        with nogil:
            cap_free(<void*>self.caps)

    def is_set(self, Value value, Flag flag):
        cdef cap_flag_value_t res = <cap_flag_value_t><int>Clear
        ret = cap_get_flag(self.caps, value, <cap_flag_t>flag, &res)
        if ret != 0:
            PyErr_SetFromErrno(OSError)
            return

        return res == _Set

    def is_supported(self, Value value):
        return cap_get_bound(<cap_value_t>value) >= 0

    def set(self, Flag flag, Value value):
        cdef cap_value_t values[1]
        values[0] = value
        ret = cap_set_flag(self.caps, <cap_flag_t>flag, 1, values, _Set)
        if ret != 0:
            PyErr_SetFromErrno(OSError)

    def unset(self, Flag flag, Value value):
        cdef cap_value_t values[1]
        values[0] = value
        ret = cap_set_flag(self.caps, <cap_flag_t>flag, 1, values, _Clear)
        if ret != 0:
            PyErr_SetFromErrno(OSError)

    def clear(self):
        ret = cap_clear(self.caps)
        if ret != 0:
            PyErr_SetFromErrno(OSError)

    def set_current_proc(self):
        ret = cap_set_proc(self.caps)
        if ret != 0:
            PyErr_SetFromErrno(OSError)

    @staticmethod
    def from_text(bytes text):
        cdef cap_t caps = cap_from_text(<char*>text)
        if <void*>caps is NULL:
            PyErr_SetFromErrno(OSError)
            return

        cdef Capabilities ret = Capabilities.__new__(Capabilities)
        ret.caps = caps
        return ret

    @staticmethod
    def name_for(Value value):
        ret = cap_to_name(<cap_value_t>value)
        if ret == NULL:
            PyErr_SetFromErrno(OSError)
        else:
            result = <bytes>ret
            cap_free(ret)
            return result

    @staticmethod
    def drop_bound(Value value):
        ret = cap_drop_bound(<cap_value_t>value)
        if ret != 0:
            PyErr_SetFromErrno(OSError)

    def __str__(self):
        cdef ssize_t size = 0
        ret = cap_to_text(self.caps, &size)
        if ret == NULL:
            PyErr_SetFromErrno(OSError)
        else:
            result = bytes(ret[:size])
            cap_free(ret)
            return result
