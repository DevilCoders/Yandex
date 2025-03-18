import tempfile
from library.python.framing.ut.proto_example.test_message_pb2 import TestMessage as Message
from library.python.framing.format import FORMAT_PROTOSEQ, FORMAT_LENVAL
from library.python.framing.packer import Packer
from library.python.framing.unpacker import Unpacker

import pytest


@pytest.mark.parametrize("format", [FORMAT_PROTOSEQ, FORMAT_LENVAL])
def test_single_message(format):
    msg = Message(
        MessageId=12345,
        Key=2 ** 45 + 123,
        Ratio=0.33,
        Service='Validator',
        Info='This is test\nPlease, pass',
        Name='The Name',
        Tags=['one', 'two', 'three']
    )
    msg_to_test = Message()
    with tempfile.NamedTemporaryFile() as tmp:
        f = open(tmp.name, "wb")
        packer = Packer(f, format)
        packer.add_proto(msg)
        packer.flush()
        f.close()

        unpacker = Unpacker(open(tmp.name, "rb").read(), format)

    msg_to_test, skip_data = unpacker.next_frame_proto(msg_to_test)
    assert skip_data is None
    assert unpacker.exhausted()
    assert msg.SerializeToString() == msg_to_test.SerializeToString()


@pytest.mark.parametrize("format", [FORMAT_PROTOSEQ, FORMAT_LENVAL])
def test_multiple_messages(format):
    serv_list = ['Cs', 'Validator', 'Bstr']
    msg_list = [
        Message(
            MessageId=12345,
            Key=2 ** 45 + 123,
            Ratio=0.33,
            Service='Validator',
            Info='This is test\nPlease, pass',
            Name='The Name',
            Tags=['one', 'two', 'three']
        ),
        Message(
            MessageId=-5,
            Key=2 ** 32 - 1,
            Ratio=0.7,
            Service='Cs',
            Info='This is test\nPlease, pass',
            Name='Second message',
            Tags=['test_tag']
        )
    ] + [
        Message(
            MessageId=x,
            Key=abs(x) * 123,
            Ratio=3 * x / 737.0,
            Service=serv_list[x % len(serv_list)],
            Info='some info' * (x % 5)
        ) for x in (1, 17, 23, 99999)
    ]

    with tempfile.NamedTemporaryFile() as tmp:
        f = open(tmp.name, "wb")
        packer = Packer(f, format)
        for msg in msg_list:
            packer.add_proto(msg)
        packer.flush()
        f.close()

        unpacker = Unpacker(open(tmp.name, "rb").read(), format)

    tmp_msg = Message()
    for msg in msg_list:
        tmp_msg, skip_data = unpacker.next_frame_proto(tmp_msg)
        assert skip_data is None
        assert msg.SerializeToString() == tmp_msg.SerializeToString()
    assert unpacker.exhausted()
