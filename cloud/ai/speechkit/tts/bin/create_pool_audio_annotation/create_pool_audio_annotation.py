import nirvana.job_context as nv
import toloka.client as toloka
from datetime import datetime, timedelta
import json


def set_ru_users_filter(pool: toloka.Pool):
    pool.filter = (
        toloka.filter.Languages.in_('RU') &
        (
            toloka.filter.RegionByPhone.in_(225) |  # RU
            toloka.filter.RegionByPhone.in_(187) |  # UA
            toloka.filter.RegionByPhone.in_(159) |  # KZ
            toloka.filter.RegionByPhone.in_(149)  # BY
        ) &
        (
            (toloka.filter.ClientType == toloka.filter.ClientType.ClientType.BROWSER) |
            (toloka.filter.ClientType == toloka.filter.ClientType.ClientType.TOLOKA_APP)
        )
    )


def set_control_blocks(pool: toloka.Pool, min_submit_time_seconds, correct_answers_rate):
    # если слишком быстро ответил -- значит не качественно и можно отклонить (заблокировать)
    pool.quality_control.add_action(
        collector=toloka.collectors.AssignmentSubmitTime(
            fast_submit_threshold_seconds=min_submit_time_seconds,
            history_size=15,
        ),
        conditions=[toloka.conditions.TotalSubmittedCount >= 1, toloka.conditions.FastSubmittedCount >= 1],
        action=toloka.actions.RejectAllAssignments(public_comment='Мало правильных ответов')
    )
    # отклонение (блокировка), если граница по контрольным ответам не пройдена
    pool.quality_control.add_action(
        collector=toloka.collectors.GoldenSet(history_size=10),
        conditions=[toloka.conditions.TotalAnswersCount >= 1, toloka.conditions.CorrectAnswersRate <= correct_answers_rate],
        action=toloka.actions.RejectAllAssignments(public_comment='Ошибки в простых заданиях')
    )

    # ограничение на одно задание в день в этом пуле
    pool.quality_control.add_action(
        collector=toloka.collectors.Income(),
        conditions=[toloka.conditions.IncomeSumForLast24Hours > 0],
        action=toloka.actions.RestrictionV2(
            scope=toloka.user_restriction.UserRestriction.POOL,
            duration=1,
            duration_unit=toloka.user_restriction.DurationUnit.DAYS,
            private_comment="thank you, that's enough",
        )
    )

    # повторное выполнение отклоненного задания
    pool.quality_control.add_action(
        collector=toloka.collectors.AssignmentsAssessment(),
        conditions=[toloka.conditions.AssessmentEvent == toloka.conditions.AssessmentEvent.Type.REJECT],
        action=toloka.actions.ChangeOverlap(delta=1, open_pool=True),
    )


def main():
    job_context = nv.context()
    inputs = job_context.get_inputs()
    outputs = job_context.get_outputs()
    parameters = job_context.get_parameters()

    task_suites_path = inputs.get('task_suites.json')
    task_suites_with_pool_id_path = outputs.get('task_suites_with_pool_id.json')
    task_spec_id_path = outputs.get("task_spec_id.txt")

    token = parameters.get('toloka-token')
    min_submit_time_seconds = parameters.get('min-submit-time-seconds')
    correct_answers_rate = parameters.get('correct-answers-rate')
    reward_per_assignment = parameters.get('reward-per-assignment')
    assignment_max_duration_seconds = parameters.get('assignment-max-duration-seconds')
    private_name = parameters.get('private-name')

    # Create a pool
    new_pool = toloka.pool.Pool(
        project_id='49020',
        private_name=private_name,
        may_contain_adult_content=True,
        will_expire=datetime.utcnow() + timedelta(days=7),
        reward_per_assignment=reward_per_assignment,
        auto_accept_solutions=False,
        assignment_max_duration_seconds=assignment_max_duration_seconds,
        defaults=toloka.pool.Pool.Defaults(default_overlap_for_new_task_suites=1),
    )

    # рускоязычные мобильные и десктопные
    set_ru_users_filter(new_pool)

    # два типа отклонений, ограничение на одно задание и повторное выполнение после отклонения
    set_control_blocks(new_pool, min_submit_time_seconds, correct_answers_rate)

    print(new_pool)

    client = toloka.TolokaClient(
        token=token,
        environment=toloka.TolokaClient.Environment.PRODUCTION,
        retries=100,
        timeout=(60.0, 240.0),
    )
    new_pool = client.create_pool(new_pool)

    with open(task_suites_path, 'r') as f:
        task_suites = json.load(f)

    for task_suit in task_suites:
        task_suit["pool_id"] = new_pool.id

    with open(task_suites_with_pool_id_path, 'w') as f:
        json.dump(task_suites, f, indent=4, ensure_ascii=False)

    with open(task_spec_id_path, 'w') as f:
        f.write("synth-audio-annotation-v0.1")

    print(f'Created pool with id {new_pool.id}')
