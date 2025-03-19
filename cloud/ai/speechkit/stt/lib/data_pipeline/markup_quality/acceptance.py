import time
import typing
from collections import defaultdict
from dataclasses import dataclass

import toloka.client as toloka

from cloud.ai.lib.python.log import get_logger
from cloud.ai.lib.python.serialization import YsonSerializable
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MarkupData,
    MarkupAssignment,
    SolutionHoneypotAcceptanceData,
    HoneypotsAcceptanceValidationData,
    MarkupDataVersions,
    HoneypotAcceptanceStrategy,
    MarkupStep,
    MarkupAssignmentStatus,
    MarkupSolution,
    SolutionAcceptanceResult,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.toloka import TolokaClient
from .strategies import (
    default_honeypot_acceptance_strategy,
    transcript_acceptance_strategy_v1,
    TextValidationToolkit,
)

logger = get_logger(__name__)


@dataclass
class HoneypotsQuality(YsonSerializable):
    all_assignments_accuracy_ratio: float
    accepted_assignments_accuracy_ratio: float


custom_acceptance_strategies: typing.Dict[MarkupDataVersions, typing.Tuple[
    HoneypotAcceptanceStrategy, typing.Callable[[MarkupSolution, MarkupSolution], SolutionAcceptanceResult]
]] = {
    MarkupDataVersions.TRANSCRIPT_AND_TYPE: (
        HoneypotAcceptanceStrategy.TRANSCRIPT_V1,
        transcript_acceptance_strategy_v1,
    ),
}


# Operation has to be reenterable. If Toloka API failure will lead to operation failure, acceptance cycle
# after operation restart will start from last point in time, accepting current assignments in SUMBITTED status.
# Get acceptance results will gather stats about all assignments, including assignments which were accepted/rejected
# in failed operation.
def run_pool_honeypot_acceptance(
    pool_id: str,
    markup_id: str,
    markup_step: MarkupStep,
    min_correct_solutions: int,
    text_validation_toolkit: TextValidationToolkit,
    toloka_client: TolokaClient,
    pull_interval_seconds: float = 60,
    **kwargs,
) -> typing.Tuple[typing.List[MarkupAssignment], HoneypotsQuality]:
    acceptance_cycle(pool_id, markup_id, markup_step, min_correct_solutions, text_validation_toolkit,
                     toloka_client, pull_interval_seconds, **kwargs)
    return get_acceptance_results(pool_id, markup_id, markup_step, min_correct_solutions, text_validation_toolkit,
                                  toloka_client, **kwargs)


def acceptance_cycle(
    pool_id: str,
    markup_id: str,
    markup_step: MarkupStep,
    min_correct_solutions: int,
    text_validation_toolkit: TextValidationToolkit,
    toloka_client: TolokaClient,
    pull_interval_seconds: float = 60,
    **kwargs,
):
    while True:
        # set task overlap only works with closed pool
        toloka_client.await_pool_status(pool_id, toloka.Pool.Status.CLOSED, pull_interval_seconds)

        logger.debug(f'getting pool {pool_id} assignments')

        assignments = toloka_client.get_assignments(
            pool_id=pool_id,
            markup_id=markup_id,
            markup_step=markup_step,
            **kwargs,
        )

        logger.debug(f'{len(assignments)} assignments received')

        if all(
            assignment.data.status in [
                MarkupAssignmentStatus.ACCEPTED,
                MarkupAssignmentStatus.REJECTED,
                MarkupAssignmentStatus.EXPIRED,
            ] for assignment in assignments
        ):
            break

        submitted_assignments = [assignment for assignment in assignments
                                 if assignment.data.status == MarkupAssignmentStatus.SUBMITTED]

        logger.debug(f'{len(submitted_assignments)} assignments are submitted from last acceptance iteration')

        task_id_to_current_overlap = {}
        for assignment in submitted_assignments:
            for task in assignment.tasks:
                if task_id_to_current_overlap.get(task.task_id, task.overlap) != task.overlap:
                    logger.warn(f'task {task.task_id} has different overlap values in different assignments within '
                                f'same pool {pool_id}')
                task_id_to_current_overlap[task.task_id] = task.overlap

        started_at = time.time()
        need_reopen_pool = False
        task_id_to_overlap_increase = defaultdict(int)
        for assignment in submitted_assignments:
            validation_data = assignment_honeypot_acceptance(
                assignment,
                min_correct_solutions,
                toloka_client.lang,
                text_validation_toolkit,
            )
            status = MarkupAssignmentStatus.ACCEPTED if validation_data.accepted else MarkupAssignmentStatus.REJECTED
            if status == MarkupAssignmentStatus.REJECTED:
                # TODO: block user
                # TODO: protection against eternal overlap increment (i.e. assignment with invalid honeypots)
                need_reopen_pool = True
                for task in assignment.tasks:
                    if len(task.known_solutions) == 0:
                        task_id_to_overlap_increase[task.task_id] += 1
                        logger.debug(f'overlap for task {task.task_id} will be increased by '
                                     f'{task_id_to_overlap_increase[task.task_id]}')

            toloka_client.set_assignment_status(assignment.data.source_id, status, validation_data.rejection_comment)

        logger.debug(f'increasing overlap for {len(task_id_to_overlap_increase)} tasks')
        for task_id, overlap_increase in task_id_to_overlap_increase.items():
            new_overlap = task_id_to_current_overlap[task_id] + overlap_increase
            toloka_client.set_task_overlap(task_id, new_overlap)

        if need_reopen_pool:
            toloka_client.open_pool(pool_id)
            toloka_client.await_pool_status(pool_id, toloka.Pool.Status.OPEN,
                                            pull_interval_seconds)  # just in case, if guarantee is not kept

        if len(submitted_assignments) > 0:
            logger.info(f'acceptance for {len(submitted_assignments)} assignments finished, '
                        f'elapsed time: {time.time() - started_at} seconds')


def get_acceptance_results(
    pool_id: str,
    markup_id: str,
    markup_step: MarkupStep,
    min_correct_solutions: int,
    text_validation_toolkit: TextValidationToolkit,
    toloka_client: TolokaClient,
    **kwargs,
) -> typing.Tuple[typing.List[MarkupAssignment], typing.Optional[HoneypotsQuality]]:
    assignments = toloka_client.get_assignments(
        pool_id=pool_id,
        markup_id=markup_id,
        markup_step=markup_step,
        **kwargs,
    )

    all_assignments_solutions = 0
    all_assignments_correct_solutions = 0
    accepted_assignments_solutions = 0
    accepted_assignments_correct_solutions = 0

    for assignment in assignments:
        validation_data = assignment_honeypot_acceptance(
            assignment,
            min_correct_solutions,
            toloka_client.lang,
            text_validation_toolkit,
        )
        assignment.validation_data.append(validation_data)

        total_solutions_count = len(validation_data.correct_solutions) + len(validation_data.incorrect_solutions)
        correct_solutions_count = len(validation_data.correct_solutions)

        all_assignments_solutions += total_solutions_count
        all_assignments_correct_solutions += correct_solutions_count

        if validation_data.accepted:
            accepted_assignments_solutions += total_solutions_count
            accepted_assignments_correct_solutions += correct_solutions_count

    if all_assignments_solutions == 0:
        # no honeypots in assignments at all
        return assignments, None

    all_assignments_accuracy_ratio = all_assignments_correct_solutions / all_assignments_solutions

    if accepted_assignments_solutions == 0:
        accepted_assignments_accuracy_ratio = 0.
    else:
        accepted_assignments_accuracy_ratio = accepted_assignments_correct_solutions / accepted_assignments_solutions

    return assignments, HoneypotsQuality(all_assignments_accuracy_ratio, accepted_assignments_accuracy_ratio)


# There are multiple honeypots in assignment, each honeypot can have multiple known solutions.
def assignment_honeypot_acceptance(
    markup_assignment: MarkupAssignment,
    min_correct_solutions: int,
    lang: str,
    text_validation_toolkit: TextValidationToolkit = None,
) -> HoneypotsAcceptanceValidationData:
    task_id_to_index = {task.task_id: i for i, task in enumerate(markup_assignment.tasks)}

    honeypots = [task for task in markup_assignment.tasks if len(task.known_solutions) > 0]
    if len(honeypots) == 0:
        return HoneypotsAcceptanceValidationData(
            accepted=True,
            correct_solutions=[],
            incorrect_solutions=[],
            min_correct_solutions=0,
            acceptance_strategy=HoneypotAcceptanceStrategy.DEFAULT,
            rejection_comment=None,
        )

    assert min_correct_solutions <= len(honeypots), 'min correct solutions can\'t be greater than total honeypots count'

    custom_strategy_tuple = custom_acceptance_strategies.get(honeypots[0].version)
    if custom_strategy_tuple is None:
        strategy = HoneypotAcceptanceStrategy.DEFAULT
        strategy_func = default_honeypot_acceptance_strategy
    else:
        strategy, strategy_func = custom_strategy_tuple

    correct_solutions = []
    incorrect_solutions = []
    incorrect_solutions_indexes = []
    for honeypot in honeypots:
        expected_solutions, actual_solution = validate_and_get_honeypot_markup_data(honeypot)
        known_solutions = []
        for expected_solution in expected_solutions:
            if strategy == HoneypotAcceptanceStrategy.DEFAULT:
                acceptance_result = strategy_func(expected_solution, actual_solution)
            elif strategy == HoneypotAcceptanceStrategy.TRANSCRIPT_V1:
                acceptance_result = strategy_func(expected_solution, actual_solution, text_validation_toolkit)
            else:
                raise ValueError(f'Unexpected strategy: {strategy}')
            known_solutions.append(acceptance_result)

        res = SolutionHoneypotAcceptanceData(
            task_id=honeypot.task_id,
            known_solutions=known_solutions,
        )
        accepted = any(known_solution.accepted for known_solution in known_solutions)

        if accepted:
            correct_solutions.append(res)
        else:
            incorrect_solutions.append(res)
            incorrect_solutions_indexes.append(task_id_to_index[honeypot.task_id])

    accepted = len(correct_solutions) >= min_correct_solutions

    return HoneypotsAcceptanceValidationData(
        accepted=accepted,
        correct_solutions=correct_solutions,
        incorrect_solutions=incorrect_solutions,
        min_correct_solutions=min_correct_solutions,
        acceptance_strategy=strategy,
        rejection_comment=generate_assignment_rejection_comment(accepted, lang, incorrect_solutions_indexes),
    )


def validate_and_get_honeypot_markup_data(
    markup_data: MarkupData,
) -> typing.Tuple[typing.List[MarkupSolution], MarkupSolution]:
    actual_solution = markup_data.solution
    expected_solutions = []
    for known_solution in markup_data.known_solutions:
        expected_solution = known_solution.solution
        assert type(expected_solution) == type(actual_solution), 'solution types must be equal'
        expected_solutions.append(expected_solution)
    return expected_solutions, actual_solution


def generate_assignment_rejection_comment(
    accepted: bool,
    lang: str,
    incorrect_solutions_indexes: typing.List[int],
) -> typing.Optional[str]:
    if accepted:
        return None
    incorrect_solutions_str = ', '.join(str(i + 1) for i in incorrect_solutions_indexes)
    if lang == 'ru-RU':
        return f'Проверьте задания под номерами: {incorrect_solutions_str}. Если вы не нашли в них ошибок, мы ' \
                'рассмотрим вашу апелляцию. Подробнее в разделе “Проверка задания” инструкции проекта.'
    return f'Tasks completed incorrectly: {incorrect_solutions_str}.'
