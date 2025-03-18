from datetime import datetime as dt

import antiadblock.tasks.tools.common_configs as common_configs


def chuncker(data, chunk_size=5000):
    for i in range(0, len(data), chunk_size):
        yield data[i:i+chunk_size]


def reformat_df_fielddate(frame, new_format, current_format=common_configs.STAT_FIELDDATE_I_FMT):
    new_frame = frame.copy()
    new_frame.fielddate = new_frame.fielddate.map(lambda t: dt.strptime(t, current_format).strftime(new_format))
    return new_frame


def get_datetime_for_incident_start():
    return dt.utcnow().strftime('%Y-%m-%dT%H:%M')
