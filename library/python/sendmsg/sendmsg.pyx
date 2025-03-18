# cython: language_level=2

from cpython.exc cimport PyErr_SetFromErrno
from cpython.mem cimport PyMem_Malloc, PyMem_Free

from libc.string cimport memcpy, memset

from collections import namedtuple


Ancillary = namedtuple('Ancillary', ['level', 'type', 'data'])
Message = namedtuple('Message', ['data', 'ancillary', 'flags'])


cdef extern from "<sys/socket.h>" nogil:
    ctypedef unsigned int socklen_t

    struct iovec:
        void* iov_base
        size_t iov_len

    struct msghdr:
        void* msg_name
        socklen_t msg_namelen
        iovec* msg_iov
        size_t msg_iovlen
        void* msg_control
        size_t msg_controllen
        int msg_flags

    struct cmsghdr:
        size_t cmsg_len
        int cmsg_level
        int cmsg_type

    cpdef enum:
        ScmRights "SCM_RIGHTS"
        ScmCredentials "SCM_CREDENTIALS"

    ssize_t sendmsg(int sockfd, const msghdr* msg, int flags)
    ssize_t recvmsg(int sockfd, msghdr* msg, int flags)

    size_t CMSG_ALIGN(size_t length)
    size_t CMSG_SPACE(size_t length)
    size_t CMSG_LEN(size_t length)
    unsigned char* CMSG_DATA(cmsghdr* control_message)
    cmsghdr* CMSG_FIRSTHDR(msghdr* msg)
    cmsghdr* CMSG_NXTHDR(msghdr* msg, cmsghdr* control_message)


def socket_sendmsg(object sock, bytes data, list ancillary=[], int flags=0):
    cdef int ret
    cdef iovec iov[1]
    cdef msghdr msg
    cdef cmsghdr* cmsg
    cdef char* control_buf = NULL
    cdef unsigned char* cmsg_data

    iov[0].iov_base = <char*>data
    iov[0].iov_len = len(data)

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    msg.msg_flags = 0

    try:
        if ancillary:
            if not all(
                isinstance(item, tuple)
                and len(item) == 3
                and isinstance(item[0], int)
                and isinstance(item[1], int)
                and isinstance(item[2], bytes)
                for item in ancillary
            ):
                raise TypeError("ancillary data expects list of 3-element (int, int, bytes) tuples")

            ancillary_data_len = sum(int(CMSG_SPACE(len(item[2]))) for item in ancillary)

            if ancillary_data_len >= (1 << sizeof(size_t)):
                raise ValueError("ancillary data doesn't fit into size_t")

            if ancillary_data_len > 0x7fffffff:
                raise ValueError("ancillary data exceeds socklen_t size limit")

            if ancillary_data_len:
                control_buf = <char*>PyMem_Malloc(<size_t>ancillary_data_len)
                if control_buf == NULL:
                    raise MemoryError("not enough memory for ancillary data")

                memset(control_buf, 0, <size_t>ancillary_data_len)

                msg.msg_control = <void*>control_buf
                msg.msg_controllen = <size_t>ancillary_data_len

                cmsg = CMSG_FIRSTHDR(&msg)
                for msg_level, msg_type, msg_data in ancillary:
                    cmsg.cmsg_level = msg_level
                    cmsg.cmsg_type = msg_type
                    cmsg.cmsg_len = <socklen_t>CMSG_LEN(len(msg_data))

                    cmsg_data = CMSG_DATA(cmsg)
                    memcpy(cmsg_data, <char*>msg_data, len(msg_data))

                    cmsg = CMSG_NXTHDR(&msg, cmsg)

        ret = sendmsg(sock.fileno(), &msg, flags)
        if ret == -1:
            PyErr_SetFromErrno(IOError)
            return

    finally:
        if control_buf != NULL:
            PyMem_Free(control_buf)
            msg.msg_control = NULL

    return ret


def socket_recvmsg(object sock, size_t max_bytes=8192, size_t ancillary_bytes=4096, int flags=0):
    cdef ssize_t ret
    cdef msghdr msg
    cdef cmsghdr *cmsg = NULL
    cdef char* cmsgbuf = NULL
    cdef char* msg_data
    cdef iovec iov[1]
    cdef size_t cmsg_space = CMSG_SPACE(ancillary_bytes)

    if cmsg_space > 0x7fffffff:
        raise ValueError("ancillary data exceeds socklen_t size limit")

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_len = max_bytes
    iov[0].iov_base = PyMem_Malloc(max_bytes)

    if iov[0].iov_base == NULL:
        raise MemoryError("not enough memory for recvmsg")

    try:
        msg.msg_iov = iov
        msg.msg_iovlen = 1

        cmsgbuf = <char*>PyMem_Malloc(cmsg_space)
        if cmsgbuf == NULL:
            raise MemoryError("not enough memory for cmsghdr")

        memset(cmsgbuf, 0, cmsg_space)

        msg.msg_control = cmsgbuf
        msg.msg_controllen = <socklen_t>cmsg_space

        ret = recvmsg(sock.fileno(), &msg, flags)
        if ret == -1:
            PyErr_SetFromErrno(IOError)
            return

        msg_data = <char*>iov[0].iov_base

        result = Message(data=msg_data[:ret], ancillary=[], flags=msg.msg_flags)

        cmsg = CMSG_FIRSTHDR(&msg)
        while cmsg != NULL:
            if not cmsg.cmsg_level and not cmsg.cmsg_type:
                continue

            cmsg_overhead = <size_t>(<char*>CMSG_DATA(cmsg) - <char*>cmsg)

            anc = Ancillary(
                level=cmsg.cmsg_level,
                type=cmsg.cmsg_type,
                data=<bytes>(CMSG_DATA(cmsg)[:cmsg.cmsg_len - cmsg_overhead])
            )
            result.ancillary.append(anc)

            cmsg = CMSG_NXTHDR(&msg, cmsg)

        return result

    finally:
        if iov[0].iov_base != NULL:
            PyMem_Free(iov[0].iov_base)
            iov[0].iov_base = NULL

        if cmsgbuf != NULL:
            PyMem_Free(cmsgbuf)
            cmsgbuf = NULL
