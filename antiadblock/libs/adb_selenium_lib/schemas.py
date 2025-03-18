import typing as t

from pydantic import BaseModel


class BrowserInfo(BaseModel):
    name: str
    version: str

    def __hash__(self) -> int:
        return hash((self.name, self.version))

    def __eq__(self, other: 'BrowserInfo') -> bool:
        if isinstance(other, BrowserInfo):
            return self.name == other.name and self.version == other.version
        raise ValueError(f'Equality operator is not implemented between BrowserInfo and {type(other)}')


class Cookie(BaseModel):
    name: str
    value: t.Optional[str] = None
    domain: t.Optional[str] = None
    secure: bool = True
    path: t.Optional[str] = None
