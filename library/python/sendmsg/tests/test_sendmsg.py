import os
import array
import socket

from library.python.sendmsg import sendmsg, recvmsg, Message, Ancillary, ScmRights


def test_send_socket():
    s1, s2 = socket.socketpair()
    r, w = os.pipe()

    assert sendmsg(s1, b'123', [(socket.SOL_SOCKET, ScmRights, array.array('i', [w]).tostring())]) > 0
    result = recvmsg(s2)

    assert isinstance(result, Message)
    assert result.data == b'123'
    assert result.flags == 0
    assert len(result.ancillary) == 1

    anc = result.ancillary[0]
    assert isinstance(anc, Ancillary)
    assert anc.level == socket.SOL_SOCKET
    assert anc.type == ScmRights

    fds = array.array('i')
    fds.fromstring(anc.data)
    assert len(fds) == 1

    w2 = fds[0]

    os.write(w2, b'data')

    assert os.read(r, 4) == b'data'
