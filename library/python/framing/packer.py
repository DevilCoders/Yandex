from . import _packer
from . import format


class Packer(object):
    def __init__(self, fd, format=format.FORMAT_PROTOSEQ):
        self._fd = fd
        self._format = format

    def add_raw(self, message):
        """Adds serialized proto message."""
        p = _packer.pack_to_string(self._format, message)
        self._fd.write(p)

    def add_proto(self, message):
        """Add protobuf message."""
        self.add_raw(message.SerializeToString())

    def flush(self):
        self._fd.flush()
