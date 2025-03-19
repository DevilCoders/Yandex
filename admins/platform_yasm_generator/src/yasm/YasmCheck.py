import json
from dataclasses import dataclass, field
from typing import Dict, Union, List


class CheckEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, (JugglerCheck, YasmCheck)):
            return o.__dict__

        if isinstance(o, Interval):
            return [o.from_, o.to]

        if isinstance(o, ValueModify):
            return {'type': o.type_, 'window': o.window}

        return json.JSONEncoder.default(self, o)


@dataclass
class Interval:
    from_: Union[int, float] = field(default=None)
    to: Union[int, float] = field(default=None)


@dataclass
class ValueModify:
    type_: str = field(default=None)
    window: int = field(default=None)


@dataclass
class JugglerCheck:
    namespace: str
    host: str
    service: str
    tags: List[str]
    aggregator: str = field(default="logic_or")
    aggregator_kwargs: Dict = field(default_factory=lambda: {
        "unreach_service": [
            {
                "check": "yasm_alert:virtual-meta"
            }
        ],
        "nodata_mode": "force_ok",
        "unreach_mode": "force_ok",
    })
    ttl: int = field(default=900)
    refresh_time: int = field(default=5)


@dataclass
class YasmCheck:
    name: str
    signal: str
    juggler_check: JugglerCheck

    tags: Dict[str, list] = field(default_factory=lambda: {'itype': ['qloud']})
    mgroups: list = field(default_factory=lambda: ["QLOUD"])
    value_modify: ValueModify = field(default_factory=lambda: ValueModify())
    warn: Interval = field(default_factory=lambda: Interval())
    crit: Interval = field(default_factory=lambda: Interval())

    def toJson(self):
        return json.dumps(self, cls=CheckEncoder)


if __name__ == '__main__':
    import json

    jg = JugglerCheck(namespace='kinopoisk', host='my_host', service='my_svc', tags=['tag1', 'tag2'])
    ch = YasmCheck(name="blah",
                   signal='blah',
                   juggler_check=jg,
                   )
    ch.tags['prj'] = ['myprj']
    ch.warn.from_ = 99
    print(ch.toJson())
