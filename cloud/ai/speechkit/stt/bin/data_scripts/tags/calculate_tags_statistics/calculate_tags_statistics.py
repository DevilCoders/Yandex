#!/usr/bin/python3

from collections import defaultdict

from cloud.ai.speechkit.stt.lib.data.model.dao import RecordTagData, RecordTagType
from cloud.ai.speechkit.stt.lib.data.ops.queries import calculate_tags_statistics

import nirvana.job_context as nv


def validate_tag(tag, skip_tags, skip_tags_with_values):
    return (tag.type in skip_tags) or (tag.to_str() in skip_tags_with_values)

def main():
    op_ctx = nv.context()

    parameters = op_ctx.parameters
    outputs = op_ctx.outputs

    skip_tags_list = parameters['skip-tags']
    skip_tags_with_values = list(filter(lambda t: ":" in t, skip_tags_list))
    skip_tags = list(filter(lambda t: ":" not in t, skip_tags_list))
    skip_tags = list(map(lambda t: RecordTagType[t], skip_tags))


    wiki_table = [
        '#|',
        '|| **Тэги** | **Число записей** | **Часов (с текстами/всего)** | '
        '**Гистограмма длительностей** | **Перцентили длительностей** ||',
    ]
    rows = calculate_tags_statistics()

    period_to_rows = defaultdict(list)
    for row in rows:
        tags = [RecordTagData.from_str(t) for t in row.tags]
        should_skip_row = False

        for tag in tags:
            if validate_tag(tag, skip_tags, skip_tags_with_values):
                should_skip_row = True
                break

        if should_skip_row:
            continue

        period_tag = next((tag for tag in tags if tag.type == RecordTagType.PERIOD), None)
        if period_tag is None:
            print(f'WARN: tag set without PERIOD tag: {row.tags}')
            continue
        period_to_rows[period_tag.value].append(row)

    for period in sorted(period_to_rows.keys(), reverse=True):
        wiki_table.append(f'|| %%(wacko wrapper=text align=center) `{period}`%% ||')
        for row in period_to_rows[period]:
            if row.duration_hours < 0.0000001:
                continue
            duration_percentiles = []
            for percentile, value in row.duration_percentiles.items():
                duration_percentiles.append(f'P{percentile}: {value:.3f}')
            duration_percentiles = '\n'.join(duration_percentiles)
            tags = '\n'.join(row.tags)
            wiki_table.append(
                f"""
||
%%
{tags}
%%
|
{row.records_count}
|
{row.duration_with_joins_hours:.3f}/{row.duration_hours:.3f} ({(row.duration_with_joins_hours / row.duration_hours * 100):.3f}%)
|
<{{гистограмма
%%
{row.duration_hist}
%%
}}>
|
<{{перцентили
%%
{duration_percentiles}
%%
}}>
||
"""
            )
    wiki_table.append('|#')
    wiki_table = '\n'.join(wiki_table)

    with open(outputs.get('tags_statistics.txt'), 'w') as f:
        f.write(wiki_table)
