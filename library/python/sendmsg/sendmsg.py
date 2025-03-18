from collections import namedtuple


Ancillary = namedtuple('Ancillary', ['level', 'type', 'data'])
Message = namedtuple('Message', ['data', 'ancillary', 'flags'])

ScmRights = 1
ScmCredentials = 2


def socket_sendmsg(*args, **kwargs):
    raise NotImplementedError("sendmsg is not available on this platform")


def socket_recvmsg(*args, **kwargs):
    raise NotImplementedError("recvmsg is not available on this platform")
