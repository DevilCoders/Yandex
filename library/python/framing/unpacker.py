from . import _unpacker
from . import format

from google.protobuf.message import DecodeError


class Unpacker(object):
    """
    Class to process encoded data frames from given input byte-string
    """
    def __init__(self, data, format=format.FORMAT_PROTOSEQ):
        self._data = data
        self._pos = 0
        self._format = format

    def tail(self):
        return self._data[self._pos:]

    def exhausted(self):
        return self._pos >= len(self._data)

    def _fail_action(self, orig_pos):
        self._pos = len(self._data)
        return None, self._data[orig_pos:]

    def next_frame(self):
        """
        Gets next frame of data encoded using protoseq or varlen format
        :return: 2-tuple containing:
            1) str object with extracted data frame in case if next frame was found, else None
            2) str object with part of original data which was skipped, None if nothing was skipped
        """
        if self.exhausted():
            return None, None

        consumed, frame, skipped = _unpacker.unpack_from_string(self._format, self._data, self._pos)
        if frame is not None:
            self._pos += consumed
            return frame, skipped
        return self._fail_action(self._pos)

    def next_frame_proto(self, message):
        """
        Gets next frame of data and sets it to given protobuf message
        if retrieved frame cannot be decoded as protobuf of given schema, skips this frame and tries to decode next and so on
        :param message: protobuf message to be set
        :return: 2-tuple containing:
            1) param message (filled with extracted data) if protobuf with required schema was found, else None
            2) str object with part of original data which was skipped, None if nothing was skipped
        """
        orig_pos = self._pos
        while not self.exhausted():
            frame_start = self._pos
            frame, _ = self.next_frame()
            if frame is None:
                return self._fail_action(orig_pos)

            try:
                message.ParseFromString(frame)
                return message, self._data[orig_pos:frame_start - orig_pos] or None
            except DecodeError:
                pass

        return self._fail_action(orig_pos)
