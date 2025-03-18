import enum
from functools import partial

import pandas as pd

from antiadblock.tasks.tools.logger import create_logger
import antiadblock.tasks.tools.common_configs as configs

logger = create_logger('lib')


class Columns(enum.Enum):
    service_id = enum.auto()
    producttype = enum.auto()
    domain = enum.auto()
    device = enum.auto()
    date = enum.auto()
    aab = enum.auto()
    fraud = enum.auto()
    money = enum.auto()
    shows = enum.auto()
    clicks = enum.auto()

    def __str__(self):
        return str(self.name)

    def __repr__(self):
        return str(self.name)


DIMENSIONS = [Columns.date.name, Columns.service_id.name]
PREFIXES = [Columns.fraud.name, Columns.aab.name]
AGGREGATION_FIELDS = [Columns.producttype.name, Columns.device.name, Columns.domain.name]
AGGREGATIONS = [
    [Columns.producttype.name],
    [Columns.device.name],
    [Columns.domain.name],
    [Columns.producttype.name, Columns.device.name],
]
METRICS = [Columns.money.name, Columns.shows.name, Columns.clicks.name]
PARSED_NAME = 'p'
SERVICE_ID_AGG_NAME = 'TOTAL'

RATIOS = {'unblock': ('aab_' + Columns.money.name, Columns.money.name)}
for metric in METRICS:
    for prefix in ('fraud', 'bad'):
        RATIOS[f'{prefix}_{metric}_percent'] = (f'aab_{prefix}_{metric}', f'aab_{metric}')


def stat_record_parser(row, tree_fields=None, dfmt=configs.STAT_FIELDDATE_I_FMT):
    """
    >>> series = pd.Series(dict(date=pd.Timestamp('2019-12-10 00:00:00'), service_id='autoru', some_field='some_value', p=dict(a=1, b=2)))
    >>> print(stat_record_parser(series))
    {'service_id': ['autoru'], 'fielddate': '2019-12-10 00:00:00', 'a': 1, 'b': 2}
    >>> print(stat_record_parser(series, ['some_field']))
    {'service_id': ['autoru', 'some_field', 'some_value'], 'fielddate': '2019-12-10 00:00:00', 'a': 1, 'b': 2}
    >>> print(stat_record_parser(series, ['some_field'], '%Y-%m-%d'))
    {'service_id': ['autoru', 'some_field', 'some_value'], 'fielddate': '2019-12-10', 'a': 1, 'b': 2}
    """
    dims = dict(service_id=[row[Columns.service_id.name]], fielddate=row[Columns.date.name].strftime(dfmt))
    if tree_fields is not None:
        for tree_field in tree_fields:
            dims['service_id'].extend([tree_field, row[tree_field]])
    return dict(dims, **row[PARSED_NAME])


def get_stat_records(frame, tree_fields=None, total=False, dfmt=configs.STAT_FIELDDATE_I_FMT):
    if total:
        frame = frame.droplevel(level=Columns.service_id.name)
        frame[Columns.service_id.name] = SERVICE_ID_AGG_NAME
        frame.set_index(Columns.service_id.name, append=True, inplace=True)
    groups = DIMENSIONS
    if tree_fields is not None:
        groups = DIMENSIONS + tree_fields
    frame = frame.groupby(level=groups).sum().apply(dict, axis=1)
    frame.name = PARSED_NAME
    return frame.reset_index().apply(partial(stat_record_parser, tree_fields=tree_fields, dfmt=dfmt), axis=1).to_list()


def get_sensors(frame, ratios, tree_fields=None, add_labels=()):
    add_labels = dict(add_labels)

    def parser(row, metric):
        parsed = dict(
            ts=row[Columns.date.name].timestamp() - 3 * 3600,
            value=min(row[metric], 1000.),  # truncate values to max 10000%
            labels=dict(sensor=metric, service_id=row[Columns.service_id.name]),
        )
        if add_labels:
            parsed['labels'].update(add_labels)
        if tree_fields is not None:
            for tree_field in tree_fields:
                parsed['labels'][tree_field] = row[tree_field]
        return parsed

    groups = DIMENSIONS
    if tree_fields is not None:
        groups = DIMENSIONS + tree_fields
    frame = frame.groupby(level=groups).sum()
    for ratio, (col1, col2) in ratios:
        try:
            frame[ratio] = 100. * frame[col1] / frame[col2]
        except:
            continue
    frame.fillna(0, inplace=True)
    frame.reset_index(inplace=True)
    res = []
    for ratio in ratios:
        try:
            res.extend(frame.apply(partial(parser, metric=ratio[0]), axis=1))
        except:
            continue
    return res


def consume_dataframe(frame, scale, calculate_ratios=tuple(RATIOS.items())):
    stat_data = []
    sensors = []
    logger.info(f'Generate data for Stat & Solomon, scale: {scale}')
    for calculate_total in (True, False):
        scaled_df = frame
        if scale != configs.Scales.minute:
            scaled_df = frame.reset_index(Columns.date.name)
            scale_kwargs = {configs.Scales.hour: dict(minute=0), configs.Scales.day: dict(hour=0, minute=0)}
            scaled_df[Columns.date.name] = scaled_df[Columns.date.name].apply(lambda fd: fd.replace(**scale_kwargs[scale]))
            scaled_df.set_index(Columns.date.name, append=True, inplace=True)
        logger.info('-- Generate Stat data --')
        stat_data.extend(get_stat_records(scaled_df, total=calculate_total, dfmt=configs.STAT_FIELDDATE_FMT[scale]))
        for agg_key in AGGREGATIONS:
            stat_data.extend(get_stat_records(scaled_df, tree_fields=agg_key, total=calculate_total, dfmt=configs.STAT_FIELDDATE_FMT[scale]))
        logger.info('-- Generate Solomon data --')
        if not calculate_total:
            sensors.extend(get_sensors(scaled_df, calculate_ratios, add_labels=[(Columns.producttype.name, '_all'), (Columns.device.name, '_all')]))
            sensors.extend(get_sensors(scaled_df, calculate_ratios, tree_fields=[Columns.producttype.name], add_labels=[(Columns.device.name, '_all')]))
            sensors.extend(get_sensors(scaled_df, calculate_ratios, tree_fields=[Columns.device.name], add_labels=[(Columns.producttype.name, '_all')]))
    return stat_data, sensors


def get_dataframe(bsdsp_df, bschevent_only_blocks_df, eventbad_df):
    frames = []
    if bschevent_only_blocks_df is not None:
        bsdsp_df = bschevent_only_blocks_df.set_index(DIMENSIONS + AGGREGATION_FIELDS + PREFIXES).combine_first(bsdsp_df.set_index(DIMENSIONS + AGGREGATION_FIELDS + PREFIXES)).reset_index()
    for df in [bsdsp_df, eventbad_df]:
        if df is not None:
            df.loc[:, Columns.date.name] = pd.to_datetime(df[Columns.date.name])
            for col in PREFIXES:
                if col in df.columns:
                    df[col] = df[col].apply(bool)
                else:
                    df[col] = ''
            df.set_index(DIMENSIONS + AGGREGATION_FIELDS + PREFIXES, inplace=True)
            frames.append(df)
            logger.info(f'Query result shape={df.shape}, head:\n{df.head()}\n\n')

    data_parts = []
    logger.info(f'Separate not frauded {[Columns.money.name]}:')
    df = frames[0][[Columns.money.name]].xs(False, level=Columns.fraud.name).groupby(level=DIMENSIONS + AGGREGATION_FIELDS).sum()
    data_parts.append(df)
    logger.info(f'shape: {data_parts[-1].shape}\n{data_parts[-1].head()}\n\n')
    df = frames[0][[Columns.money.name]].xs((False, True), level=[Columns.fraud.name, Columns.aab.name])
    df.columns = [f'aab_{metric}' for metric in df.columns]
    data_parts.append(df)
    logger.info(f'shape: {data_parts[-1].shape}\n{data_parts[-1].head()}\n\n')

    logger.info(f'Separate total (fraud included) {[Columns.shows.name, Columns.clicks.name]}')
    df = frames[0][[Columns.shows.name, Columns.clicks.name]].groupby(level=DIMENSIONS + AGGREGATION_FIELDS).sum()
    data_parts.append(df)
    logger.info(f'shape: {data_parts[-1].shape}\n{data_parts[-1].head()}\n\n')
    df = frames[0][[Columns.shows.name, Columns.clicks.name]].xs(True, level=Columns.aab.name).groupby(level=DIMENSIONS + AGGREGATION_FIELDS).sum()
    df.columns = [f'aab_{metric}' for metric in df.columns]
    data_parts.append(df)
    logger.info(f'shape: {data_parts[-1].shape}\n{data_parts[-1].head()}\n\n')

    logger.info(f'Separate fraud total {METRICS}')
    df = frames[0].xs(True, level=Columns.fraud.name).groupby(level=DIMENSIONS + AGGREGATION_FIELDS).sum()
    df.columns = [f'fraud_{metric}' for metric in df.columns]
    data_parts.append(df)
    logger.info(f'shape: {data_parts[-1].shape}\n{data_parts[-1].head()}\n\n')

    logger.info(f'Separate aab fraud {METRICS}')
    df = frames[0].xs((True, True), level=[Columns.fraud.name, Columns.aab.name])
    df.columns = [f'aab_fraud_{metric}' for metric in df.columns]
    data_parts.append(df)
    logger.info(f'shape: {data_parts[-1].shape}\n{data_parts[-1].head()}\n\n')

    if eventbad_df is not None:
        logger.info(f'Separate bad {METRICS}')
        df = frames[1].groupby(level=DIMENSIONS + AGGREGATION_FIELDS + [Columns.aab.name]).sum().unstack(Columns.aab.name).fillna(0)
        df.columns = [f'{"aab_" if aab else ""}bad_{metric}' for metric, aab in df.columns]
        data_parts.append(df)
        logger.info(f'shape: {data_parts[-1].shape}\n{data_parts[-1].head()}\n\n')

    logger.info('Join final dataframe')
    return data_parts[0].join(data_parts[1:], how='left').fillna(0)
