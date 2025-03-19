# !/usr/bin/python3

import nirvana.job_context as nv
import ujson as json

from cloud.ai.lib.python.datetime import parse_nirvana_datetime
from cloud.ai.speechkit.stt.lib.data.model.dao import Mark
from cloud.ai.speechkit.stt.lib.data_pipeline.select_records_joins import select_records_joins, available_strategies


def main():
    outputs = nv.context().outputs
    params = nv.context().parameters

    tags = params.get('tags')
    if params.get('mark') == 'ALL':
        mark = None
    else:
        mark = Mark(params.get('mark'))
    bits = params.get('bits')
    join_received_after = parse_nirvana_datetime(params.get('join-received-after'))
    join_received_before = parse_nirvana_datetime(params.get('join-received-before'))
    duration_limit_minutes_per_tag = params.get('duration-limit-minutes-per-tag')
    filter_tags = params.get('filter-tags')
    compose_tags = params.get('compose-tags')
    picking_strategies = params.get('picking-strategies')

    strategies = [available_strategies[name] for name in picking_strategies]
    records_joins = select_records_joins(
        tags, mark, bits, join_received_after, join_received_before,
        duration_limit_minutes_per_tag, filter_tags, compose_tags, strategies
    )

    with open(outputs.get('records'), 'w') as f:
        json.dump(records_joins, f, ensure_ascii=False, indent=4)
