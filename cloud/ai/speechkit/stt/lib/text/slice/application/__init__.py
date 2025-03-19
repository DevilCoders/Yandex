import re
import typing

from dataclasses import dataclass

from cloud.ai.lib.python.serialization import YsonSerializable


@dataclass
class PreparedSlice(YsonSerializable):
    name: str
    predicate: str


def infer_slices(text: str, slices: typing.List[PreparedSlice]) -> typing.List[str]:
    result = []
    for slice in slices:
        try:
            ok = eval(slice.predicate, {'text': text, 're': re})
        except Exception:
            print(f'invalid predicate: {slice.predicate}')
            raise
        assert isinstance(ok, bool), 'slice predicate must return boolean'
        if ok:
            result.append(slice.name)
    return result
