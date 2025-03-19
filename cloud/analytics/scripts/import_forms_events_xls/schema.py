from datetime import timedelta

import pandas as pd


def _str_to_bool(s):
    s = s.str.lower()
    s = s.replace(["да", "yes"], 1)
    s = s.replace(["нет", "no"], 0)
    return s.astype(bool)


def _datetime_to_int(s):
    return (s - pd.Timestamp("1970-01-01")) // pd.Timedelta('1s')


schema = [
    {
        'name': 'event_start_time',
        'type': 'uint32',
        'normalize_func': lambda s: _datetime_to_int(pd.to_datetime(s) + timedelta(hours=9))
    },
    {
        'name': 'event_end_time',
        'type': 'uint32',
        'normalize_func': lambda s: _datetime_to_int(pd.to_datetime(s) + timedelta(hours=21))
    },
    {
        'name': 'event_name',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip()
    },
    {
        'name': 'event_type',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip(),
        'whitelist': {"conference", "meetup", "party", "webinar"}
    },
    {
        'name': 'event_link',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip()
    },
    {
        'name': 'form_id',
        'type': 'uint32',
        'normalize_func': lambda s: s.astype(int)
    },
    {
        'name': 'event_id',
        'type': 'uint32',
        'normalize_func': lambda s: s.astype(int)
    },
    {
        'name': 'event_city',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip().astype(str),
    },
    {
        'name': 'attendee_status',
        'type': 'string',
        'whitelist': {"registered", "attended", "not_attended"},
        'normalize_func': lambda s: s.str.lower()
    },
    {
        'name': 'forms_email',
        'type': 'string',
        'normalize_func': lambda s: s.str.lower().astype(str)
    },
    {
        'name': 'forms_company',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip().astype(str),
    },
    {
        'name': 'forms_last_name',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip().astype(str),
    },
    {
        'name': 'forms_first_name',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip().astype(str),
    },
    {
        'name': 'forms_position',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip().astype(str),
    },
    {
        'name': 'website',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip().astype(str),
    },
    {
        'name': 'forms_phone',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip().astype(str),
    },
    {
        'name': 'forms_agree_pers_data',
        'type': 'boolean', 'normalize_func': _str_to_bool,
        "whitelist": {True, False}
    },
    {
        'name': 'forms_mail_marketing',
        'type': 'string',
        'normalize_func': lambda s: s.str.lower()
        # "whitelist": {True, False}
    },
    {
        'name': 'has_cloud_account',
        'type': 'boolean',
        'normalize_func': _str_to_bool,
        "whitelist": {True, False}
    },
    {
        'name': 'cloud_interest_reason',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip().astype(str),
    },
    {
        'name': 'needs_call',
        'type': 'boolean',
        'normalize_func': _str_to_bool,
        "whitelist": {True, False}
    },
    {
        'name': 'grant_request_reason',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip().astype(str),
    },
    {
        'name': 'product',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip(),
        'whitelist': {"compute", "mdb", "storage", "vpc", "security", "resilience"}
    },
    {
        'name': 'forms_created_time',
        'type': 'uint32',
        'normalize_func': lambda s: _datetime_to_int(pd.to_datetime(s))
    },
    {
        'name': 'audience_type',
        'type': 'string',
        'normalize_func': lambda s: s.str.rstrip(),
        'whitelist': {"IT", "business", "any"}
    }
]


