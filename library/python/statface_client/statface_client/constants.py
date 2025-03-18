# coding: utf8

from __future__ import division, absolute_import, print_function, unicode_literals

import itertools


DAILY_SCALE = 'd'
WEEKLY_SCALE = 'w'
MONTHLY_SCALE = 'm'
QUARTERLY_SCALE = 'q'
YEARLY_SCALE = 'y'
HOURLY_SCALE = 'h'
MINUTELY_SCALE = 'i'
CONTINUAL_SCALE = 's'

STATFACE_SCALES = frozenset((
    DAILY_SCALE,
    WEEKLY_SCALE,
    MONTHLY_SCALE,
    QUARTERLY_SCALE,
    YEARLY_SCALE,
    HOURLY_SCALE,
    MINUTELY_SCALE,
    CONTINUAL_SCALE
))

STATFACE_ORDERED_SCALES = (
    YEARLY_SCALE,
    QUARTERLY_SCALE,
    MONTHLY_SCALE,
    WEEKLY_SCALE,
    DAILY_SCALE,
    HOURLY_SCALE,
    MINUTELY_SCALE,
    CONTINUAL_SCALE,
)

# human to statface (longscale to shortscale)
SCALE_PARAMS = {
    'daily': 'd',
    'weekly': 'w',
    'monthly': 'm',
    'quarterly': 'q',
    'yearly': 'y',
    'hourly': 'h',
    'minutely': 'i',
    'continual': 's',
}
# shortscale -> shortscale too (for validation):
SCALE_PARAMS.update(list((short_scale, short_scale) for short_scale in SCALE_PARAMS.values()))

DOWNLOAD_SCALE_PARAMS = SCALE_PARAMS.copy()
DOWNLOAD_SCALE_PARAMS.update(
    # No mapping going on, basically just validation of the aggregate-scales' names.
    (scale, scale)
    for res_scale, source_scale in itertools.combinations(STATFACE_ORDERED_SCALES, 2)
    for method in ('sum', 'avg')
    # e.g. 'd_by_h_sum'
    for scale in ['{}_by_{}_{}'.format(res_scale, source_scale, method)]
)


JSON_DATA_FORMAT = 'json'
TSKV_DATA_FORMAT = 'tskv'
CSV_DATA_FORMAT = 'csv'
TSV_DATA_FORMAT = 'tsv'
JSONGRAPH_DATA_FORMAT = 'jsongraph2'  # DO NOT USE, for internal Traf needs only. @feriat
# Format 'python structures, hide the (de)serialization'
PY_DATA_FORMAT = 'py'

STATFACE_DATA_FORMATS = frozenset((
    PY_DATA_FORMAT,
    JSON_DATA_FORMAT,
    TSV_DATA_FORMAT,
    CSV_DATA_FORMAT,
    TSKV_DATA_FORMAT,
    JSONGRAPH_DATA_FORMAT,
))


# format identifier -> request data parameter name
UPLOAD_FORMAT_PARAMS = {
    PY_DATA_FORMAT: 'data',
    JSON_DATA_FORMAT: 'json_data',
    TSKV_DATA_FORMAT: 'tskv_data',
    TSV_DATA_FORMAT: 'tsv_data',
    CSV_DATA_FORMAT: 'csv_data',
}


STATFACE_BETA = 'upload.stat-beta.yandex-team.ru'
STATFACE_PRODUCTION = 'upload.stat.yandex-team.ru'
STATFACE_BETA_FRONTEND = 'stat-beta.yandex-team.ru'
STATFACE_PRODUCTION_FRONTEND = 'stat.yandex-team.ru'
STATFACE_PRODUCTION_UPLOAD = 'upload.stat.yandex-team.ru'

JUST_ONE_CHECK = 'just_one_check'
DO_NOTHING = 'do_nothing'
ENDLESS_WAIT_FOR_FINISH = 'endless_wait_for_finish'

MAX_UPLOAD_BYTES = 209715200  # 200 MB
EMPTY_STR_SIZE = 37

TOO_MANY_REQUESTS_STATUS = 429

RETRIABLE_STATUSES = set(list(range(500, 600)) + [TOO_MANY_REQUESTS_STATUS])
UNRETRIABLE_STATUSES = set(range(400, 500)) - set([TOO_MANY_REQUESTS_STATUS])

RECOMMENDED_YT_UPLOAD_SIZE_BYTES = 128 * 2**20
RECOMMENDED_YT_UPLOAD_SIZE_ROWS = 300000
