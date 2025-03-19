import os
import time
import typing
from enum import Enum
from dataclasses import dataclass
from datetime import datetime, timedelta
import logging

import toloka.client as toloka

from cloud.ai.lib.python.iterators import chunk_iterator
from cloud.ai.lib.python.log import get_logger
from cloud.ai.lib.python.datetime import parse_datetime, format_nirvana_datetime, now
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from cloud.ai.speechkit.stt.lib.data.model.dao import *
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_params import dict_merge
from cloud.ai.speechkit.stt.lib.text.re import clean as clean_text

logger = get_logger(__name__)


@dataclass
class CheckTranscriptAggregatedSolution:
    confidence: float
    solution: MarkupSolutionCheckTranscript


class TolokaEnvironment(Enum):
    SANDBOX = 'https://sandbox.toloka.yandex.com'
    PRODUCTION = 'https://toloka.yandex.com'
    YANG = 'https://yang.yandex-team.ru'


class TolokaClient:
    def __init__(
        self,
        oauth_token: str,
        lang: str,
        environment: TolokaEnvironment = TolokaEnvironment.PRODUCTION,
    ):
        self.client = toloka.TolokaClient(
            token=oauth_token,
            url=environment.value,
            retries=100,
            timeout=(60.0, 1200.0),  # (connectTimeout, readTimeout)
        )
        self.lang = lang

    def get_pool(self, pool_id: str) -> toloka.Pool:
        return self.client.get_pool(pool_id)

    def open_pool(self, pool_id: str):
        logger.debug(f'opening pool {pool_id}')
        # TODO: we make retry in loop because of TOLOKAKIT-230
        attempts = 0
        while True:
            attempts += 1
            try:
                self.client.open_pool(pool_id)
                return
            except Exception as e:
                logger.error(f'open pool failed, attempt #{attempts}, see TOLOKAKIT-230: {e}')
                if self.ensure_pool_status(pool_id, toloka.Pool.Status.OPEN, confirmation_timeout_seconds=10):
                    return
                if attempts > 30:
                    raise e
                time.sleep(60)

    def create_pool(
        self,
        pool: typing.Union[toloka.Pool, typing.Tuple[str, dict]],
        tasks: typing.List[dict],
    ) -> str:
        if isinstance(pool, toloka.Pool):
            pool = self.client.create_pool(pool)
        else:
            # deprecated way
            base_pool_id, pool_params_override = pool
            pool_params = self.client.get_pool(base_pool_id).unstructure()
            del pool_params['id']
            pool_params['will_expire'] = format_nirvana_datetime(now() + timedelta(days=365))
            dict_merge(pool_params, pool_params_override)
            pool = toloka.Pool.structure(pool_params)
            pool = self.client.create_pool(pool)

        logger.debug(f'created pool {pool.id}')

        for task in tasks:
            task['pool_id'] = pool.id

        # we upload tasks in chunks, because Toloka API is tender
        tasks = [toloka.Task.structure(task) for task in tasks]
        for tasks_chunk in chunk_iterator(tasks, 10000):
            logger.debug(f'uploading {len(tasks_chunk)} tasks to pool {pool.id}')
            op = self.client.create_tasks_async(tasks_chunk, skip_invalid_items=False)
            op = self.client.wait_operation(op, timeout=timedelta(hours=24))

            # don't know what to do with this, may be chunk were uploaded partially
            assert op.status == toloka.operations.Operation.Status.SUCCESS, f'tasks upload operation {op.id} failed'

        logger.debug(f'opening pool {pool.id}')
        self.client.open_pool(pool.id)

        return pool.id

    def ensure_pool_status(
        self,
        pool_id: str,
        status: toloka.Pool.Status,
        confirmation_timeout_seconds: float = 0,
    ):
        pool_status = self.get_pool(pool_id).status
        if pool_status == status:
            if confirmation_timeout_seconds == 0:
                return True
            time.sleep(confirmation_timeout_seconds)
            pool_status = self.get_pool(pool_id).status
            return pool_status == status
        return False

    def await_pool_status(
        self,
        pool_id: str,
        status: toloka.Pool.Status,
        pull_interval_seconds: float = 60,
        confirmation_timeout_seconds: float = 0,
    ):
        logger.debug(f'waiting for pool {pool_id} status {status}')
        while True:
            time.sleep(pull_interval_seconds)
            if self.ensure_pool_status(pool_id, status, confirmation_timeout_seconds):
                break

    # TODO (TOLOKA-15383): delete this wrapper when Toloka fixes problem with broken auto-accept/reject rules.
    # This wrapper is needed to keep markup graph from failing, when some assignments stuck in
    # SUBMITTED status after pool closing. Stuck assignments can be processed in two ways:
    #
    # 1. Go to pool and accept/reject remaining assignments manually.
    # 2. Wait for auto-accept timeout (one day in our case).
    #
    # But both ways are too long for client markup. So we just accept stuck assignments with API.
    def get_assignments_failsafe(
        self,
        pool_id: str,
        markup_id: str,
        markup_step: MarkupStep,
        pull_interval_seconds: float,
        statuses: typing.Optional[typing.List[MarkupAssignmentStatus]] = None,
        **kwargs,
    ) -> typing.List[MarkupAssignment]:
        while True:
            assignments = self.get_assignments(pool_id, markup_id, markup_step, statuses, **kwargs)
            stuck_assignments = [
                assignment
                for assignment in assignments
                if assignment.data.status not in [
                    MarkupAssignmentStatus.ACCEPTED,
                    MarkupAssignmentStatus.REJECTED,
                    MarkupAssignmentStatus.EXPIRED,
                ]
            ]
            if len(stuck_assignments) == 0:
                return assignments
            if any(assignment.data.status != MarkupAssignmentStatus.SUBMITTED for assignment in stuck_assignments):
                raise ValueError('Unexpected stuck assignment status')
            logger.warn(
                f'{len(stuck_assignments)} assignments are stuck in SUBMITTED status, accepting them with API... '
                f'(see TOLOKA-15383)',
            )
            for assignment in stuck_assignments:
                self.set_assignment_status(assignment.data.source_id, MarkupAssignmentStatus.ACCEPTED)
            time.sleep(pull_interval_seconds)  # not sure if this needed, maybe Toloka uses some caches

    def get_assignments(
        self,
        pool_id: str,
        markup_id: str,
        markup_step: MarkupStep,
        statuses: typing.Optional[typing.List[MarkupAssignmentStatus]] = None,
        **kwargs,
    ) -> typing.List[MarkupAssignment]:
        assignments: typing.List[MarkupAssignment] = []

        if statuses is None:
            statuses = [
                MarkupAssignmentStatus.ACTIVE,
                MarkupAssignmentStatus.SUBMITTED,
                MarkupAssignmentStatus.ACCEPTED,
                MarkupAssignmentStatus.REJECTED,
                MarkupAssignmentStatus.SKIPPED,
                MarkupAssignmentStatus.EXPIRED,
            ]

        received_at = now()

        logger.debug(f'fetching assignments of pool {pool_id}')
        for toloka_assignment in self.client.get_assignments(toloka.search_requests.AssignmentSearchRequest(
            status=[dao_assignment_status_to_toloka_assignment_status[status] for status in statuses],
            pool_id=pool_id,
        )):
            toloka_assignment_fields = toloka_assignment.unstructure()
            if 'solutions' not in toloka_assignment_fields:
                # it is a case for expired assignments, for example
                # TODO: handle it, creating empty solutions
                continue
            assignments.append(
                create_markup_assignment(
                    toloka_assignment_fields,
                    received_at, markup_id, markup_step, self.lang, **kwargs,
                ),
            )

        return assignments

    def set_assignment_status(
        self,
        assignment_id: str,
        status: MarkupAssignmentStatus,
        public_comment: typing.Optional[str] = None,
    ):
        if status == MarkupAssignmentStatus.ACCEPTED:
            logger.debug(f'accepting assignment {assignment_id}')
            self.client.accept_assignment(assignment_id, public_comment or '')
        elif status == MarkupAssignmentStatus.REJECTED:
            logger.debug(f'rejecting assignment {assignment_id} with comment: {public_comment}')
            self.client.reject_assignment(assignment_id, public_comment)
        else:
            raise ValueError(f'Unexpected assignment status: {status}')

    def set_task_overlap(self, task_id: str, overlap: int):
        logger.debug(f'setting overlap {overlap} for task {task_id}')
        self.client.patch_task_overlap_or_min(
            task_id=task_id,
            patch=toloka.task.TaskOverlapPatch(overlap=overlap),
        )


def create_markup_assignment(
    fields: dict,
    received_at: datetime,
    markup_id: str,
    markup_step: MarkupStep,
    lang: str,
    **kwargs,
) -> MarkupAssignment:
    results = zip(fields['tasks'], fields['solutions'])
    tasks = []
    for task, solution in results:
        tasks.append(create_markup_data(task, solution, markup_step, lang, **kwargs))

    return MarkupAssignment(
        id=generate_id(),
        markup_id=markup_id,
        markup_step=markup_step,
        pool_id=fields['pool_id'],
        data=MarkupAssignmentDataV1(
            source_id=fields['id'],
            task_suite_id=fields['task_suite_id'],
            user_id=fields['user_id'],
            status=MarkupAssignmentStatus(fields['status']),
            reward=float(fields['reward']),  # Decimal
            mixed=fields['mixed'],
            auto_merged=fields['automerged'],
            owner_id=fields['owner']['id'],
            created_at=parse_dt(fields.get('created')),
            submitted_at=parse_dt(fields.get('submitted')),
            accepted_at=parse_dt(fields.get('accepted')),
            rejected_at=parse_dt(fields.get('rejected')),
            skipped_at=parse_dt(fields.get('skipped')),
            expired_at=parse_dt(fields.get('expired')),
        ),
        tasks=tasks,
        validation_data=[],
        received_at=received_at,
        other=None,
    )


def create_markup_data(task: dict, solution: dict, markup_step: MarkupStep, lang: str, **kwargs) -> MarkupData:
    known_solutions = task.get('known_solutions', [])
    is_honeypot = len(known_solutions) > 0

    input_creator_args = {}
    solution_creator_args = {}
    if markup_step == MarkupStep.CHECK_ASR_TRANSCRIPT:
        markup_data_version = MarkupDataVersions.CHECK_TRANSCRIPT
        input_creator = create_input_audio_url_and_transcript_source_asr
        solution_creator = create_solution_check_transcript
        recognition_params = kwargs['recognition_params']
        recognition_endpoint = RecognitionEndpoint.from_yson(recognition_params)
        input_creator_args = {
            'recognition_endpoint': recognition_endpoint,
            'is_honeypot': is_honeypot,
        }
    elif markup_step == MarkupStep.CHECK_TRANSCRIPT:
        markup_data_version = MarkupDataVersions.CHECK_TRANSCRIPT
        input_creator = create_input_audio_url_and_transcript_source_markup
        solution_creator = create_solution_check_transcript
    elif markup_step == MarkupStep.TRANSCRIPT:
        markup_data_version = active_transcript_markup_version
        solution_creator_args = {'lang': lang}
        if active_transcript_markup_version == MarkupDataVersions.PLAIN_TRANSCRIPT:
            input_creator = create_input_audio_url
            solution_creator = create_solution_plain_transcript
        elif active_transcript_markup_version == MarkupDataVersions.TRANSCRIPT_AND_TYPE:
            input_creator = create_input_audio_url
            solution_creator = create_solution_transcript_and_type
        else:
            raise ValueError(f'Unexpected active markup project: {active_transcript_markup_version}')
    elif markup_step == MarkupStep.QUALITY_EVALUATION:
        markup_data_version = MarkupDataVersions.CHECK_TRANSCRIPT
        input_creator = create_input_audio_url_and_transcript_source_join
        solution_creator = create_solution_check_transcript
        bit_filename_to_join_id = kwargs['bit_filename_to_join_id']
        input_creator_args = {
            'bit_filename_to_join_id': bit_filename_to_join_id,
            'is_honeypot': is_honeypot,
        }
    elif markup_step == MarkupStep.COMPARE_TEXT_MEANING:
        markup_data_version = MarkupDataVersions.COMPARE_TEXT_MEANING
        input_creator = create_input_compare_text_meaning
        solution_creator = create_solution_compare_text_meaning
    elif markup_step == MarkupStep.COMPARE_TEXT_MEANING_SBS:
        markup_data_version = MarkupDataVersions.COMPARE_TEXT_MEANING_SBS
        input_creator = create_input_compare_text_meaning_sbs
        solution_creator = create_solution_compare_text_meaning_sbs
    elif markup_step == MarkupStep.COMPARE_AUDIO_EUPHONY:
        markup_data_version = MarkupDataVersions.COMPARE_AUDIO_EUPHONY
        input_creator = create_input_compare_audio_euphony
        solution_creator = create_solution_compare_audio_euphony
    else:
        raise ValueError(f'Unexpected markup step: {markup_step}')

    return MarkupData(
        version=markup_data_version,
        input=input_creator(task['input_values'], **input_creator_args),
        solution=solution_creator(solution['output_values'], **solution_creator_args),
        known_solutions=[
            KnownSolution(
                solution=solution_creator(s['output_values'], **solution_creator_args),
                correctness_weight=s['correctness_weight'],
            )
            for s in known_solutions
        ],
        task_id=task['id'],
        overlap=task['overlap'],
        raw_data={
            'task': task,
            'solution': solution,
        },
        created_at=parse_dt(task['created']),
    )


def create_input_audio_url(fields: dict) -> AudioURLInput:
    assert isinstance(fields['url'], str)
    return AudioURLInput(
        audio_s3_obj=S3Object.from_https_url(fields['url']),
    )


def create_input_audio_url_and_transcript_source_asr(
    fields: dict,
    recognition_endpoint: RecognitionEndpoint,
    is_honeypot: bool,
) -> AudioURLAndTranscriptInput:
    assert isinstance(fields['text'], str)
    assert isinstance(fields['url'], str)
    if is_honeypot:
        text_source = TranscriptSourceHoneypot()
    else:
        text_source = TranscriptSourceASR(
            recognition=RecognitionPlainTranscript(text=fields['text']),
            recognition_endpoint=recognition_endpoint,
        )
    return AudioURLAndTranscriptInput(
        audio_s3_obj=S3Object.from_https_url(fields['url']),
        text=fields['text'],
        text_source=text_source,
    )


def create_input_audio_url_and_transcript_source_join(
    fields: dict,
    bit_filename_to_join_id: typing.Dict[str, str],
    is_honeypot: bool,
) -> AudioURLAndTranscriptInput:
    assert isinstance(fields['text'], str)
    assert isinstance(fields['url'], str)
    s3_obj = S3Object.from_https_url(fields['url'])
    if is_honeypot:
        text_source = TranscriptSourceHoneypot()
    else:
        bit_filename = os.path.basename(s3_obj.key)
        record_join_id = bit_filename_to_join_id[bit_filename]
        text_source = TranscriptSourceJoin(
            record_join_id=record_join_id,
        )

    return AudioURLAndTranscriptInput(
        audio_s3_obj=s3_obj,
        text=fields['text'],
        text_source=text_source,
    )


def create_input_audio_url_and_transcript_source_markup(
    fields: dict,
) -> AudioURLAndTranscriptInput:
    assert isinstance(fields['text'], str)
    assert isinstance(fields['url'], str)
    return AudioURLAndTranscriptInput(
        audio_s3_obj=S3Object.from_https_url(fields['url']),
        text=fields['text'],
        text_source=TranscriptSourceMarkup(),
    )


def create_input_compare_text_meaning(fields: dict) -> TextMeaningComparisonInput:
    assert isinstance(fields['reference'], str)
    assert isinstance(fields['recognised'], str)
    return TextMeaningComparisonInput(
        reference=fields['reference'],
        hypothesis=fields['recognised'],
    )


def create_input_compare_text_meaning_sbs(fields: dict) -> TextMeaningSbSComparisonInput:
    assert isinstance(fields['orig_phrase'], str)
    assert isinstance(fields['left_phrase'], str)
    assert isinstance(fields['right_phrase'], str)
    return TextMeaningSbSComparisonInput(
        reference=fields['orig_phrase'],
        hypothesis_left=fields['left_phrase'],
        hypothesis_right=fields['right_phrase'],
    )


def create_input_compare_audio_euphony(fields: dict) -> AudioEuphonyComparisonInput:
    assert isinstance(fields['left_audio_url'], str)
    assert isinstance(fields['right_audio_url'], str)
    return AudioEuphonyComparisonInput(
        left_audio_url=fields['left_audio_url'],
        right_audio_url=fields['right_audio_url'],
    )


def create_solution_plain_transcript(fields: dict, lang: str) -> MarkupSolutionPlainTranscript:
    assert isinstance(fields['output'], str)
    return MarkupSolutionPlainTranscript(
        text=sanitize_toloka_solution_text(fields.get('output', ''), lang),
        misheard=fields.get('misheard', False),
    )


def create_solution_transcript_and_type(fields: dict, lang: str) -> MarkupSolutionTranscriptAndType:
    assert isinstance(fields['text'], str)
    assert isinstance(fields['cls'], str)
    return MarkupSolutionTranscriptAndType(
        text=sanitize_toloka_solution_text(fields['text'], lang),
        type=MarkupTranscriptType.from_toloka_output(fields['cls']),
    )


def create_solution_check_transcript(fields: dict) -> MarkupSolutionCheckTranscript:
    assert isinstance(fields['ok'], bool)
    assert isinstance(fields['cls'], str)
    return MarkupSolutionCheckTranscript(
        ok=fields['ok'],
        type=MarkupTranscriptType.from_toloka_output(fields['cls']),
    )


def create_solution_compare_text_meaning(fields: dict) -> TextMeaningComparisonSolution:
    assert isinstance(fields['result'], str)
    return TextMeaningComparisonSolution(
        result=fields['result'] == 'OK',
    )


def create_solution_compare_text_meaning_sbs(fields: dict) -> TextMeaningSbSComparisonSolution:
    assert isinstance(fields['result'], str)
    return TextMeaningSbSComparisonSolution(
        choice=SbSChoice(fields['result']),
    )


def create_solution_compare_audio_euphony(fields: dict) -> AudioEuphonyComparisonSolution:
    assert isinstance(fields['result'], str)
    return AudioEuphonyComparisonSolution(
        chosen_audio_url=fields['result'],
    )


def parse_dt(dt: typing.Optional[str]) -> typing.Optional[datetime]:
    if dt is None:
        return None
    # Toloka uses UTC timezone
    return parse_datetime(dt + 'Z')


def sanitize_toloka_solution_text(text: str, lang: str) -> str:
    cleaned_text, _ = clean_text(text, lang, toloka=True)
    if text != cleaned_text:
        logger.warn(f'solution text was not clean: {text}')
    return cleaned_text


dao_assignment_status_to_toloka_assignment_status = {
    MarkupAssignmentStatus.ACTIVE: toloka.Assignment.ACTIVE,
    MarkupAssignmentStatus.SUBMITTED: toloka.Assignment.SUBMITTED,
    MarkupAssignmentStatus.ACCEPTED: toloka.Assignment.ACCEPTED,
    MarkupAssignmentStatus.REJECTED: toloka.Assignment.REJECTED,
    MarkupAssignmentStatus.SKIPPED: toloka.Assignment.SKIPPED,
    MarkupAssignmentStatus.EXPIRED: toloka.Assignment.EXPIRED,
}
