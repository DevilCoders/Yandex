import ctypes

MCL_CURRENT = 1
MCL_FUTURE  = 2

libc = ctypes.CDLL('libc.so.6', use_errno=True)

def mlockall(flags: int = MCL_CURRENT | MCL_FUTURE):
    result = libc.mlockall(flags)
    if result != 0:
        raise Exception("cannot lock memmory, errno=%s" % ctypes.get_errno())

def munlockall():
    result = libc.munlockall()
    if result != 0:
        raise Exception("cannot lock memmory, errno=%s" % ctypes.get_errno())