import nirvana.job_context as nv
import yt.wrapper as yt

import toloka.client as toloka

from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MarkupStep, MarkupAssignmentStatus, MarkupDataVersions
)
from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data_pipeline.toloka import TolokaClient
from cloud.ai.speechkit.stt.lib.experiments.libri_speech_mer_pipeline import (
    generate_tasks_check, noise_levels, generate_task_check_json, honeypots_check, MarkupCheck,
    table_markups_check_meta, table_name,
)


def main():
    op_ctx = nv.context()
    params = op_ctx.parameters

    yt.config['proxy']['url'] = 'hahn'

    recognitions = generate_tasks_check(
        params.get('tasks-count'),  # 500
        params.get('reference-length-limit'),  # 200
    )
    for r in recognitions:
        print(f'ref: {r.reference}\nhyp: {r.hypothesis}\n'
              f'noise: {noise_levels[r.noise.level_idx] if r.noise is not None else "none"}\n')

    reference_to_recognition = {r.reference: r for r in recognitions}

    toloka_client = TolokaClient(oauth_token=params.get('toloka-token'), lang='ru-RU')

    pool_id = toloka_client.create_pool(
        pool=(
            params.get('base-pool-id'),  # 20552082
            {'private_name': f'{len(recognitions)} text comparisons'},
        ),
        tasks=[generate_task_check_json(r.reference, r.hypothesis) for r in recognitions] + [generate_task_check_json(ref, hyp, ok) for ref, hyp, ok in honeypots_check],
    )

    toloka_client.await_pool_status(pool_id, toloka.Pool.Status.CLOSED)

    assignments = toloka_client.get_assignments_failsafe(
        pool_id=pool_id,
        markup_id='nevermind',
        markup_step=MarkupStep.COMPARE_TEXT_MEANING,
        pull_interval_seconds=60,
    )

    received_at = now()

    markups = []

    for assignment in assignments:
        if assignment.data.status != MarkupAssignmentStatus.ACCEPTED:
            continue
        for task in assignment.tasks:
            assert task.version == MarkupDataVersions.COMPARE_TEXT_MEANING

            if len(task.known_solutions) > 0:
                continue

            reference = task.input.reference
            hypothesis = task.input.hypothesis
            ok = task.solution.result

            recognition = reference_to_recognition[reference]

            markups.append(MarkupCheck(
                recognition_id=recognition.id,
                dataset=recognition.dataset,
                record=recognition.record,
                reference=reference,
                hypothesis=hypothesis,
                ok=ok,
                model=recognition.model,
                noise=recognition.noise,
                pool_id=pool_id,
                assignment_id=assignment.data.source_id,
                task=task,
                received_at=received_at,
            ))

    table_markups = Table(meta=table_markups_check_meta, name=table_name)
    table_markups.append_objects(markups)
