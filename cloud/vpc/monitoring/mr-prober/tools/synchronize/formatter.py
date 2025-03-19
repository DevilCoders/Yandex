import textwrap
from typing import Union

from devtools import PrettyFormat

__all__ = ["PrettyFormatWithLongStringsTruncated"]


class PrettyFormatWithLongStringsTruncated(PrettyFormat):
    def __init__(self, *args, **kwargs):
        self._strings_max_length = None
        if "strings_max_length" in kwargs:
            self._strings_max_length = int(kwargs.pop("strings_max_length"))
            if self._strings_max_length <= 3:
                raise ValueError("PrettyFormatWithLongStringsTruncated.strings_max_length should be greater than 3")
        super().__init__(*args, **kwargs)

    def _format_str_bytes(self, value: Union[str, bytes], value_repr: str, indent_current: int, indent_new: int):
        if self._strings_max_length is not None:
            if isinstance(value, str):
                value = textwrap.shorten(value, self._strings_max_length)
            else:
                try:
                    value = textwrap.shorten(value.decode(), self._strings_max_length).encode()
                except UnicodeDecodeError:
                    value = value[:self._strings_max_length - 3] + b"..."

            if len(value_repr) > self._strings_max_length:
                value_repr = textwrap.shorten(value_repr, self._strings_max_length)
                if value_repr.startswith("b'") or value_repr.startswith("'"):
                    value_repr += "'"
        return super()._format_str_bytes(value, value_repr, indent_current, indent_new)
