from dataclasses import dataclass


@dataclass
class KnownEnabledApps:
    cms: bool
    meta: bool
    deploy: bool
    katan: bool
    dbm: bool
