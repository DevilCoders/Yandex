import typing as tp


class DictObj:
    def __init__(self,  **kwargs: tp.Dict[str, tp.Any]) -> None:
        self._dict: tp.Dict[str, tp.Any] = kwargs

    def __getattr__(self, attr: str) -> tp.Any:
        return self._dict[attr]

    def keys(self) -> tp.Iterable[str]:
        return self._dict.keys()

    def values(self) -> tp.Iterable[tp.Any]:
        return self._dict.values()

__all__ = ['DictObj']
