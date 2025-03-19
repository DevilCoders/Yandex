import json

import nirvana.job_context as nv
import toloka.client as toloka

from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MarkupStep, MarkupAssignmentStatus, MarkupDataVersions, SbSChoice
)
from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data_pipeline.toloka import TolokaClient
from cloud.ai.speechkit.stt.lib.experiments.libri_speech_mer_pipeline import (
    generate_tasks_and_honeypots_sbs, generate_task_sbs_json, print_task,
    MarkupSbS, MarkupMetrics, table_markups_sbs_meta, table_name
)


def main():
    op_ctx = nv.context()
    params = op_ctx.parameters
    outputs = op_ctx.outputs

    tasks, honeypots = generate_tasks_and_honeypots_sbs(
        params.get('tasks-count'),
        params.get('honeypots-count'),
        params.get('reference-length-limit'),
    )
    print('TASKS\n')
    for t in tasks:
        print_task(t)
    print('HONEYPOTS\n')
    for t in honeypots:
        print_task(t)

    reference_to_recognition_pair = {
        reference: (less_noise_recognition, more_noise_recognition)
        for less_noise_recognition, more_noise_recognition, reference in tasks + honeypots
    }
    assert len(tasks) + len(honeypots) == len(reference_to_recognition_pair)

    toloka_client = TolokaClient(oauth_token=params.get('toloka-token'), lang='ru-RU')

    pool_id = toloka_client.create_pool(
        pool=(params.get('base-pool-id'), {'private_name': f'{len(tasks)} text comparisons'}),
        tasks=
        [generate_task_sbs_json(t, for_honeypot=False) for t in tasks] +
        [generate_task_sbs_json(t, for_honeypot=True) for t in honeypots],
    )

    toloka_client.await_pool_status(pool_id, toloka.Pool.Status.CLOSED)

    assignments = toloka_client.get_assignments_failsafe(
        pool_id=pool_id,
        markup_id='nevermind',
        markup_step=MarkupStep.COMPARE_TEXT_MEANING_SBS,
        pull_interval_seconds=60,
    )

    received_at = now()

    markups = []
    defined_prior_total = 0
    defined_prior_correct = 0
    undefined_prior_left = 0
    undefined_prior_right = 0
    honeypots_total = 0
    honeypots_correct = 0
    for assignment in assignments:
        if assignment.data.status != MarkupAssignmentStatus.ACCEPTED:
            continue
        for task in assignment.tasks:
            assert task.version == MarkupDataVersions.COMPARE_TEXT_MEANING_SBS

            reference = task.input.reference
            hypothesis_left = task.input.hypothesis_left
            choice = task.solution.choice

            less_noise_recognition, more_noise_recognition = reference_to_recognition_pair[reference]
            assert less_noise_recognition.hypothesis != more_noise_recognition.hypothesis

            # left/right position of recognitions are shuffled in toloka

            if hypothesis_left == less_noise_recognition.hypothesis:
                recognition_left = less_noise_recognition
                recognition_right = more_noise_recognition
                prior = SbSChoice.LEFT
            elif hypothesis_left == more_noise_recognition.hypothesis:
                recognition_left = more_noise_recognition
                recognition_right = less_noise_recognition
                prior = SbSChoice.RIGHT
            else:
                raise RuntimeError(f'No match for hypothesis "{hypothesis_left}" in selected recognitions')

            if len(task.known_solutions) > 0:
                honeypots_total += 1
                if choice == prior:
                    honeypots_correct += 1
                continue

            if recognition_left.noise is None and recognition_right.noise is None:
                prior = None
            elif recognition_left.noise is not None and recognition_right.noise is not None and \
                recognition_left.noise.level == recognition_right.noise.level:
                prior = None

            if prior is None:
                if choice == SbSChoice.LEFT:
                    undefined_prior_left += 1
                else:
                    undefined_prior_right += 1
            else:
                defined_prior_total += 1
                if prior == choice:
                    defined_prior_correct += 1

            markups.append(MarkupSbS(
                recognition_id_left=recognition_left.id,
                recognition_id_right=recognition_right.id,
                dataset=recognition_left.dataset,
                record=recognition_left.record,
                reference=reference,
                hypothesis_left=recognition_left.hypothesis,
                hypothesis_right=recognition_right.hypothesis,
                prior=prior,
                choice=choice,
                model_left=recognition_left.model,
                model_right=recognition_right.model,
                noise_left=recognition_left.noise,
                noise_right=recognition_right.noise,
                pool_id=pool_id,
                assignment_id=assignment.data.source_id,
                task=task,
                markup_metrics=None,  # to be filled
                received_at=received_at,
            ))

    markup_metrics = MarkupMetrics(
        defined_prior_accuracy=defined_prior_correct / max(defined_prior_total, 1),
        undefined_prior_skew=max(undefined_prior_left,
                                 undefined_prior_right) / max(min(undefined_prior_left,
                                                                  undefined_prior_right), 1),
        honeypots_accuracy=honeypots_correct / max(honeypots_total, 1),
    )

    for markup in markups:
        markup.markup_metrics = markup_metrics

    with open(outputs.get('markup_metrics.json'), 'w') as f:
        json.dump(markup_metrics.to_yson(), f, indent=4, ensure_ascii=False)

    table_markups = Table(meta=table_markups_sbs_meta, name=table_name)
    table_markups.append_objects(markups)
