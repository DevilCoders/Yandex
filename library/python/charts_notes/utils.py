from dateutil import parser


def remove_nulls(d):
    if not isinstance(d, dict):
        return d
    return {
        k: remove_nulls(v)
        for k, v in d.items()
        if v is not None
    }


def parse_date(d):
    return parser.parse(d) if isinstance(d, str) else d


def parse_dates(*ds):
    return tuple(parse_date(d) for d in ds)


def format_utc_time(d):
    return d.strftime('%Y-%m-%dT%H:%M:%S.000Z')
