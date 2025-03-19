import csv
import os
from collections import defaultdict

import nirvana.job_context as nv
import ujson as json
import yt.wrapper as yt

from cloud.ai.lib.python import datetime
from cloud.ai.speechkit.stt.lib.data.model.dao import MarkupStep, MarkupPriorities
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_quality import run_pool_honeypot_acceptance
from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_params import (
    produce_pool_params,
    compare_meaning_ru_pool_params,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.toloka import TolokaClient


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs
    params = op_ctx.parameters

    snapshot_path = outputs.get("snapshot")
    pool_info = {}
    pool_id, pool_name, received_at_string, table_name = None, None, None, None
    if os.path.exists(snapshot_path):
        print("Found snapshot with pool info", flush=True)
        with open(snapshot_path, "r") as f:
            pool_info = json.load(f)[0]
            pool_id = pool_info['pool_id']
            pool_name = pool_info['pool_name']
            received_at_string = pool_info['received_at']
            table_name = pool_info['table_name']

    markup_step = MarkupStep.COMPARE_TEXT_MEANING
    toloka_token = params.get('toloka-token')

    toloka_client = TolokaClient(oauth_token=toloka_token, lang='ru-RU')

    if pool_info.get('pool_id') is None or pool_info.get('pool_id') == 0:
        received_at = datetime.now()
        table_name = f'{Table.get_name(received_at)}-{received_at.hour}-{received_at.minute}'

        with open(inputs.get('tasks.json')) as f:
            tasks = json.load(f)

        with open(inputs.get('tags.json')) as f:
            options_json = json.load(f)
            tags = options_json['tags']

        tags_string = " ".join(tags)
        pool_name = f'Compare texts meaning {table_name} {tags_string}'
        received_at_string = datetime.format_datetime(received_at)

        pool_id = toloka_client.create_pool(
            pool=(
                compare_meaning_ru_pool_params.base_pool_id,
                produce_pool_params(
                    pool_name,
                    markup_step,
                    [],
                    compare_meaning_ru_pool_params,
                    MarkupPriorities.INTERNAL,
                ),
            ),
            tasks=tasks,
        )

        with open(snapshot_path, "w") as f:
            json.dump(
                [
                    {
                        "pool_id": pool_id,
                        "pool_name": pool_name,
                        "received_at": received_at_string,
                        "table_name": table_name
                    }
                ],
                f
            )

    markups_assignments, honeypots_quality = run_pool_honeypot_acceptance(
        pool_id=pool_id,
        markup_id=pool_name,
        markup_step=markup_step,
        min_correct_solutions=compare_meaning_ru_pool_params.min_correct_honeypot_solutions,
        text_comparison_stop_words=None,
        toloka_client=toloka_client,
        pull_interval_seconds=30,
    )

    ref_rec_to_ok, ref_rec_to_all = defaultdict(int), defaultdict(int)

    for assignment in markups_assignments:
        for task in assignment.tasks:
            input = task.input
            solution = task.solution
            if solution.result:
                ref_rec_to_ok[(input.reference, input.hypothesis)] += 1
            ref_rec_to_all[(input.reference, input.hypothesis)] += 1

    result_table = []
    for pair, total in ref_rec_to_all.items():
        ok = ref_rec_to_ok[pair]
        conf = (float(ok) + 1) / (total + 2)
        ref, rec = pair
        result_table.append({"reference": ref,
                             "recognition": rec,
                             "prob": conf,
                             "ok": ok,
                             "total": total,
                             "received_at": received_at_string})

    yt.config["proxy"]["url"] = "hahn"
    yt.write_table(f'//home/mlcloud/hotckiss/mer/{table_name}', result_table)

    with open(outputs.get('output.tsv'), 'wt') as f:
        tsv_writer = csv.writer(f, delimiter='\t')
        tsv_writer.writerow(['INPUT:reference', 'INPUT:recognised', 'MV:confidence'])
        for item in result_table:
            tsv_writer.writerow([item['reference'], item['recognition'], item['prob']])
