import json
import os
import time
import typing
from dataclasses import dataclass
from enum import Enum

import reactor_client as r

import cloud.ai.speechkit.stt.lib.reactor as reactor_consts

LAUNCHER_PATH = '/cloud/ai/speechkit/stt/toloka-transcription-pipeline/launcher'

operation_select_records_for_markup = 'operation-fe35d658-075a-4295-bd60-af078ab09dc3'
operation_recognize_speech = 'operation-03b5b1f7-aabd-4962-9457-f7f2346faf22'
operation_cluster_references = 'operation-31d48839-358c-4301-8fe7-63b84bf7a8c9$1'
operation_stop_words = 'operation-eefa8dbd-8388-4696-8ad0-29c67a4304b7'
operation_obfuscate = 'operation-a5b8ea24-f2df-48ea-b0db-c339cd271024'
operation_generate_transcript_honeypots_from_real_tasks = 'operation-d6eaf220-d4d2-41b9-834f-c9c3414d6f6c$4$1'
operation_generate_transcript_honeypots_from_honeypots = 'operation-d6eaf220-d4d2-41b9-834f-c9c3414d6f6c$2'
operation_generate_check_transcript_honeypots = 'operation-9baaa31b-8629-4d27-a668-021acc25cd8d'
operation_upload_audio_for_toloka = 'operation-3f74dc0e-b843-424f-8824-3c92128cf9bb$5'
operation_run_markup_check_asr = 'operation-1ef7e8c8-b6f2-45f2-84bd-53d0a1a45b75$6'
operation_run_markup_transcript = 'operation-1ef7e8c8-b6f2-45f2-84bd-53d0a1a45b75'
operation_run_markup_quality_evaluation = 'operation-1ef7e8c8-b6f2-45f2-84bd-53d0a1a45b75$7'
operation_transcript_feedback_loop = 'operation-c8d813dc-8d1e-4bcb-a6cf-c891e2265703'
operation_instantiate_markup_trigger_artifact = 'instantiate_string_reactor_artifact'

two_weeks_ago = ' + " PERIOD:" + Time.dateNow().plusDays(-14).format("yyyy-MM")'


@dataclass
class Tag:
    name: str
    count: int
    window: bool


@dataclass
class GeneratedHoneypotsQuery:
    transcript_from_real_tasks: int
    transcript_from_honeypots: int
    check_transcript_per_type: typing.List[int]
    random_sample_fraction: float


class TranscriptStrategy(Enum):
    HONEYPOTS = 'honeypots'
    FEEDBACK_LOOP = 'feedback-loop'


def weigh_tags(tags, weight: float = 1):
    assert weight >= 0
    weighted_tags = []
    for tag in tags:
        weighted_tags.append(
            Tag(
                name=tag.name,
                count=int(tag.count * weight),
                window=tag.window
            )
        )
    return weighted_tags


def main():
    client = r.ReactorAPIClientV1(
        base_url='https://reactor.yandex-team.ru',
        token=os.getenv('REACTOR_TOKEN'),
    )

    builder = r.reaction_builders.NirvanaReactionBuilder()

    builder.set_version(2)

    builder.set_source_graph(flow_id='47eb6ad4-3c29-4ebe-91ce-850d2ef7c4f4')
    builder.set_target_graph(flow_id='22ba1ef4-8bf6-4354-b057-a8ceede514dc')
    builder.set_reaction_path(
        LAUNCHER_PATH,
        'Audio transcription pipeline launcher',
    )

    builder.set_project(reactor_consts.project_identifier)
    builder.set_owner(reactor_consts.reaction_owner)

    builder.set_retry_policy(
        retries=5,
        time_param=10 * 60 * 1000,  # milliseconds
        retry_policy_descriptor=r.r_objs.RetryPolicyDescriptor.UNIFORM,
        result_cloning_policy=r.r_objs.NirvanaResultCloningPolicy.RECURSIVE,
    )

    # TODO: add notifications

    set_global_params(builder)
    set_common_params(builder)

    triggers = generate_concurrent_triggers_and_artifacts(
        client=client,
        artifact_base_name='ru-RU',
        lang='ru-RU',
        tags=weigh_tags(tags=[
            Tag(name='CLIENT:ats-7np MODE:stream LANG:ru-RU PERIOD:2022-04', count=450, window=False),
            Tag(name='CLIENT:ats-jb9 MODE:stream LANG:ru-RU PERIOD:2022-04', count=450, window=False),
            Tag(name='CLIENT:fromtech-qnd MODE:stream LANG:ru-RU PERIOD:2021-08', count=150, window=False),
            Tag(name='CLIENT:kaspi-bank-rfr MODE:stream LANG:ru-RU PERIOD:2021-10', count=300, window=False),
            Tag(name='CLIENT:mtt-u16 MODE:stream LANG:ru-RU PERIOD:2022-04', count=150, window=False),
            Tag(name='CLIENT:neuro-net-mjq MODE:stream LANG:ru-RU PERIOD:2021-08', count=150, window=False),
            Tag(name='CLIENT:neuro-net-upn MODE:stream LANG:ru-RU PERIOD:2021-08', count=150, window=False),
            Tag(name='CLIENT:prof-it-jja MODE:stream LANG:ru-RU PERIOD:2021-11', count=150, window=False),
            Tag(name='CLIENT:qiwi-bank-f1o MODE:long LANG:ru-RU PERIOD:2022-04', count=30, window=False),
            Tag(name='CLIENT:robovoice-tmm MODE:stream LANG:ru-RU PERIOD:2021-09', count=150, window=False),
            Tag(name='CLIENT:s7-85t MODE:stream LANG:ru-RU PERIOD:2022-04', count=150, window=False),
            Tag(name='CLIENT:web-banker-ke3 MODE:stream LANG:ru-RU PERIOD:2022-04', count=150, window=False),
            Tag(name='CLIENT:zvonobot-c2a MODE:stream LANG:ru-RU PERIOD:2022-04', count=150, window=False),
            Tag(name='CLIENT:zvonobot-h64 MODE:stream LANG:ru-RU PERIOD:2022-04', count=150, window=False),
            Tag(name='CLIENT:alfa-bank-lal MODE:stream LANG:ru-RU PERIOD:2022-04', count=300, window=False),
            Tag(name='CLIENT:ariadna-o2i MODE:stream LANG:ru-RU PERIOD:2022-06', count=300, window=False),
        ], weight=0.1) + weigh_tags(tags=[
            Tag(name='~CLIENT MODE:long LANG:ru-RU', count=150, window=True),
            Tag(name='~CLIENT MODE:stream LANG:ru-RU', count=600, window=True),
            Tag(name='IMPORT:yandex-call-center LANG:ru-RU', count=90, window=True),
        ], weight=0.1) + weigh_tags(tags=[
            Tag(name='CLIENT:voca-gvr MODE:long LANG:ru-RU PERIOD:2022-04', count=50, window=False),
            Tag(name='CLIENT:voca-5c1 MODE:long LANG:ru-RU PERIOD:2021-10', count=50, window=False),
            Tag(name='CLIENT:voca-aoh MODE:long LANG:ru-RU PERIOD:2021-09', count=50, window=False),
            Tag(name='CLIENT:voca-d6m MODE:long LANG:ru-RU PERIOD:2021-10', count=50, window=False),
            Tag(name='CLIENT:voca-magnit-chg MODE:long LANG:ru-RU PERIOD:2021-10', count=50, window=False),
            Tag(name='CLIENT:voca-73u MODE:long LANG:ru-RU PERIOD:2022-03', count=50, window=False),
        ], weight=0.1),
        recognition_mode='short',
        recognition_model='general',
        honeypots_query=GeneratedHoneypotsQuery(
            transcript_from_real_tasks=0,
            transcript_from_honeypots=0,
            check_transcript_per_type=[800, 150, 300],
            random_sample_fraction=0.3,
        ),
        concurrent_sessions=2,
    ) + generate_concurrent_triggers_and_artifacts(
        client=client,
        artifact_base_name='kk-KK',
        lang='kk-KK',
        tags=[
            # Tag(name='IMPORT:files_mogo_2020-12-28_2021-03-26', count=30, window=False),
            # Tag(name='IMPORT:taxi-p2p LANG:kk-KK', count=30, window=True),
            Tag(name='CLIENT:kaspi-bank-rfr MODE:stream LANG:kk-KK PERIOD:2021-10', count=200, window=False),
        ],
        # TODO: currently only stream recognition works for kk-KK, and only general:rc model is available
        recognition_mode='stream',
        recognition_model='general',
        honeypots_query=GeneratedHoneypotsQuery(
            transcript_from_real_tasks=0,
            transcript_from_honeypots=0,
            check_transcript_per_type=[640, 120, 80],
            random_sample_fraction=1.,
        ),
        concurrent_sessions=2,
    ) + generate_concurrent_triggers_and_artifacts(
        client=client,
        artifact_base_name='khural',
        lang='ru-RU',
        tags=[
            Tag(name='IMPORT:khural', count=1, window=False),
        ],
        recognition_mode='short',
        recognition_model='general',
        honeypots_query=GeneratedHoneypotsQuery(
            transcript_from_real_tasks=1,
            transcript_from_honeypots=1,
            check_transcript_per_type=[0, 0, 0],
            random_sample_fraction=0.1,
        ),
        toloka_environment='YANG',
        transcript_strategy=TranscriptStrategy.HONEYPOTS,
        check_asr_disabled=True,
        quality_evaluation_disabled=True,
        transcript_pool_params_override={
            'public_description':
                'Расшифровка хуралов, '
                'https://wiki.yandex-team.ru/cloud/ai/misc/khural-transcribation/toloka-instruction/',
            'project_id': '11914',
            'filter': {
                'and': [
                    {'or':
                        [
                            {
                                'operator': 'EQ',
                                'value': 'BROWSER',
                                'key': 'client_type',
                                'category': 'computed',
                            },
                            {
                                'operator': 'EQ',
                                'value': 'TOLOKA_APP',
                                'key': 'client_type',
                                'category': 'computed',
                            },
                        ],
                    },
                    {'or':
                        [
                            {
                                'key': '36183',
                                'operator': 'GT',
                                'value': 0.0,
                                'category': 'skill',
                            },
                        ],
                    },
                ],
            },
            'mixer_config': {
                'real_tasks_count': 4,
                'golden_tasks_count': 0,
                'training_tasks_count': 0,
                'force_last_assignment': True,
            },
            'quality_control': {
                'training_requirement': None,
                'configs': [],
            },
            'reward_per_assignment': 32.,  # rubles, because Yang
            'assignment_max_duration_seconds': 4800,
        },
        upload_toloka_audio_to_mds=True,
        obfuscate=False,
    )

    builder.set_dynamic_triggers(triggers)

    namespace_id = r.r_objs.NamespaceIdentifier(namespace_path=LAUNCHER_PATH)
    reaction_id = r.r_objs.OperationIdentifier(namespace_identifier=namespace_id)

    replacement_kind = None
    if client.reaction.check_exists(reaction_id):
        replacement_kind = r.r_objs.ReactionReplacementKind.REPLACE_AND_DELETE_OLD_VERSION
        triggers_ids = [
            r.r_objs.DynamicTriggerIdentifier(name=trigger.name)
            for trigger in client.dynamic_trigger.list(reaction_id)
            if trigger.status == r.r_objs.TriggerStatus.ACTIVE
        ]
        if triggers_ids:
            client.dynamic_trigger.update(reaction_id, triggers_ids, r.r_objs.DynamicTriggerAction.DEACTIVATE)
            time.sleep(10)

    reaction = client.reaction.create(
        builder.operation_descriptor,
        create_if_not_exist=True,
        replacement_kind=replacement_kind,
        old_version_for_replacement=r.r_objs.OperationIdentifier(namespace_identifier=namespace_id)
    )
    reaction_id = r.r_objs.OperationIdentifier(operation_id=reaction.reaction_id)

    triggers_ids = [
        r.r_objs.DynamicTriggerIdentifier(name=trigger.name) for trigger in client.dynamic_trigger.list(reaction_id)
    ]
    client.dynamic_trigger.update(reaction_id, triggers_ids, r.r_objs.DynamicTriggerAction.ACTIVATE)

    notification_descriptors = client.namespace_notification.list(
        namespace_identifier=r.r_objs.NamespaceIdentifier(namespace_id=reaction.namespace_id),
    )
    client.namespace_notification.change(
        delete_id_list=[
            desc.id_ for desc in notification_descriptors
        ],
        notification_descriptor_list=[
            *generate_notifications(reaction.namespace_id, 'eranik'),
            *generate_notifications(reaction.namespace_id, 'o-gulyaev')
        ]
    )


def generate_notifications(namespace_id: int, recipient: str):
    return [
        r.r_objs.NotificationDescriptor(
            namespace_identifier=r.r_objs.NamespaceIdentifier(namespace_id=namespace_id),
            event_type=r.r_objs.NotificationEventType.REACTION_FAILED,
            transport=r.r_objs.NotificationTransportType.TELEGRAM,
            recipient=recipient
        ),
        r.r_objs.NotificationDescriptor(
            namespace_identifier=r.r_objs.NamespaceIdentifier(namespace_id=namespace_id),
            event_type=r.r_objs.NotificationEventType.REACTION_RECALCULATION_PROBLEM,
            transport=r.r_objs.NotificationTransportType.TELEGRAM,
            recipient=recipient
        )
    ]


def set_global_params(builder: r.reaction_builders.NirvanaReactionBuilder):
    builder.set_global_param_to_expression_var('lang', r.r_objs.ExpressionVariable(var_name='lang'))
    builder.set_global_param_to_expression_var(
        'toloka-environment',
        r.r_objs.ExpressionVariable(var_name='toloka_environment'),
    )
    builder.set_global_param_to_expression_var(
        'toloka-token',
        r.r_objs.ExpressionVariable(var_name='toloka_token'),
    )
    builder.set_global_param_to_expression_var(
        'transcript-strategy',
        r.r_objs.ExpressionVariable(var_name='transcript_strategy'),
    )


def set_common_params(builder: r.reaction_builders.NirvanaReactionBuilder):
    builder.set_block_param_to_value_with_hints(
        operation_select_records_for_markup, 'min-duration', 1, r.r_objs.VariableTypes.FLOAT)
    builder.set_block_param_to_value_with_hints(
        operation_select_records_for_markup, 'max-duration', 7200, r.r_objs.VariableTypes.FLOAT)
    builder.set_block_param_to_expression_var(
        operation_select_records_for_markup,
        'tags',
        r.r_objs.ExpressionVariable(var_name='tags'),
    )
    builder.set_block_param_to_expression_var(
        operation_select_records_for_markup,
        'tags-limits',
        r.r_objs.ExpressionVariable(var_name='tags_limits'),
    )
    builder.set_block_param_to_expression_var(
        operation_select_records_for_markup,
        'concurrent-session',
        r.r_objs.ExpressionVariable(var_name='concurrent_session'),
    )
    builder.set_block_param_to_expression_var(
        operation_obfuscate,
        'min_pitch',
        r.r_objs.ExpressionVariable(var_name='obfuscation_min_pitch'),
    )
    builder.set_block_param_to_expression_var(
        operation_obfuscate,
        'max_pitch',
        r.r_objs.ExpressionVariable(var_name='obfuscation_max_pitch'),
    )

    builder.set_block_param_to_expression_var(
        operation_recognize_speech,
        'mode',
        r.r_objs.ExpressionVariable(var_name='recognition_mode'),
    )
    builder.set_block_param_to_expression_var(
        operation_recognize_speech,
        'model',
        r.r_objs.ExpressionVariable(var_name='recognition_model'),
    )

    builder.set_block_param_to_expression_var(
        operation_generate_transcript_honeypots_from_real_tasks,
        'honeypots-with-speech-count',
        r.r_objs.ExpressionVariable(var_name='honeypots_transcript_from_real_tasks'),
    )
    builder.set_block_param_to_expression_var(
        operation_generate_transcript_honeypots_from_honeypots,
        'honeypots-with-speech-count',
        r.r_objs.ExpressionVariable(var_name='honeypots_transcript_from_honeypots'),
    )
    builder.set_block_param_to_expression_var(
        operation_generate_check_transcript_honeypots,
        'solution-types-counts',
        r.r_objs.ExpressionVariable(var_name='honeypots_check_transcript_per_type'),
    )
    builder.set_block_param_to_expression_var(
        operation_generate_check_transcript_honeypots,
        'random-sample-fraction',
        r.r_objs.ExpressionVariable(var_name='generate_check_transcript_honeypots_random_sample_fraction'),
    )

    builder.set_block_param_to_expression_var(
        operation_upload_audio_for_toloka,
        'upload-to-mds',
        r.r_objs.ExpressionVariable(var_name='upload_toloka_audio_to_mds'),
    )

    builder.set_block_param_to_expression_var(
        operation_run_markup_check_asr,
        'disabled',
        r.r_objs.ExpressionVariable(var_name='check_asr_disabled'),
    )
    builder.set_block_param_to_expression_var(
        operation_run_markup_quality_evaluation,
        'disabled',
        r.r_objs.ExpressionVariable(var_name='quality_evaluation_disabled'),
    )

    builder.set_block_param_to_expression_var(
        operation_run_markup_check_asr,
        'pool-params-override',
        r.r_objs.ExpressionVariable(var_name='check_asr_pool_params_override'),
    )
    builder.set_block_param_to_expression_var(
        operation_run_markup_transcript,
        'pool-params-override',
        r.r_objs.ExpressionVariable(var_name='transcript_pool_params_override'),
    )
    builder.set_block_param_to_expression_var(
        operation_run_markup_quality_evaluation,
        'pool-params-override',
        r.r_objs.ExpressionVariable(var_name='quality_evaluation_pool_params_override'),
    )

    builder.set_block_param_to_expression_var(
        operation_transcript_feedback_loop,
        'disabled',
        r.r_objs.ExpressionVariable(var_name='transcript_feedback_loop_disabled'),
    )

    builder.set_block_param_to_expression_var(
        operation_instantiate_markup_trigger_artifact,
        'artifact-path',
        r.r_objs.ExpressionVariable(var_name='artifact'),
    )

    # TODO: temporary, while we don't have text resources for other languages, re-link to global param after fix
    builder.set_block_param_to_value(operation_cluster_references, 'lang', 'ru-RU')
    builder.set_block_param_to_value(operation_stop_words, 'lang', 'ru-RU')


def generate_artifact_for_trigger(client: r.ReactorAPIClientV1, name: str):
    client.artifact.create(
        artifact_type_identifier=r.r_objs.ArtifactTypeIdentifier(artifact_type_key='STRING'),
        artifact_identifier=reactor_consts.get_markup_artifact_identifier_for_trigger(name),
        description='Артефакт, версия которого обновляются по завершении инстанса графа разметки, нужен для '
                    'последовательных цикличных запусков графа',
        permissions=r.r_objs.NamespacePermissions(roles={
            reactor_consts.reaction_owner: r.r_objs.NamespaceRole.RESPONSIBLE,
            'o-gulyaev': r.r_objs.NamespaceRole.RESPONSIBLE,
            'a-kondratenko': r.r_objs.NamespaceRole.RESPONSIBLE,
            'eranik': r.r_objs.NamespaceRole.RESPONSIBLE,
        }),
        project_identifier=reactor_consts.project_identifier,
        create_if_not_exist=True,
    )


def generate_trigger(
    lang: str,
    tags: typing.List[Tag],
    recognition_mode: str,
    recognition_model: str,
    honeypots_query: GeneratedHoneypotsQuery,
    artifact: str,
    transcript_strategy: TranscriptStrategy = TranscriptStrategy.FEEDBACK_LOOP,
    check_asr_disabled: bool = False,
    quality_evaluation_disabled: bool = False,
    check_asr_pool_params_override: dict = {},  # noqa
    transcript_pool_params_override: dict = {},  # noqa
    quality_evaluation_pool_params_override: dict = {},  # noqa
    toloka_environment: str = 'PRODUCTION',
    upload_toloka_audio_to_mds: bool = False,
    obfuscate: bool = True,
    concurrent_session: str = '',
) -> r.r_objs.DynamicTrigger:
    def tags_list_repr():
        return ', '.join(f'"{tag.name}"{two_weeks_ago if tag.window else ""}' for tag in tags)

    def tags_counts_list_repr():
        return ', '.join(str(tag.count) for tag in tags)

    def int_list_repr(l: typing.List[int]):
        return ', '.join(str(x) for x in l)

    def bool_to_str(b: bool):
        return 'true' if b else 'false'

    def get_toloka_token():
        if toloka_environment == 'PRODUCTION':
            return 'cloud-ai-toloka-token'
        elif toloka_environment == 'YANG':
            return 'cloud-ai-yang-token'
        elif toloka_environment == 'SANDBOX':
            return 'cloud-ai-toloka-sandbox-token'
        else:
            raise ValueError(f'unexpected toloka environment: {toloka_environment}')

    def dict_to_str(d: dict) -> str:
        return json.dumps(d).replace('"', '\\"')

    obfuscation_min_pitch = 0
    obfuscation_max_pitch = 0
    if obfuscate:
        obfuscation_min_pitch = 200
        obfuscation_max_pitch = 400

    transcript_feedback_loop_disabled = False
    if transcript_strategy != TranscriptStrategy.FEEDBACK_LOOP:
        transcript_feedback_loop_disabled = True

    return r.r_objs.DynamicTrigger(
        trigger_name=artifact.replace('-', '_'),
        expression=r.r_objs.Expression('\n'.join([
            f'global lang = "{lang}";',
            f'global tags = [{tags_list_repr()}].toStringList();',
            f'global tags_limits = [{tags_counts_list_repr()}].toIntList();',
            f'global obfuscation_min_pitch = {obfuscation_min_pitch};',
            f'global obfuscation_max_pitch = {obfuscation_max_pitch};',
            f'global recognition_mode = Datum.nirvanaEnum("{recognition_mode}");',
            f'global recognition_model = "{recognition_model}";',
            f'global honeypots_transcript_from_real_tasks = {honeypots_query.transcript_from_real_tasks};',
            f'global honeypots_transcript_from_honeypots = {honeypots_query.transcript_from_honeypots};',
            f'global honeypots_check_transcript_per_type = [{int_list_repr(honeypots_query.check_transcript_per_type)}].toIntList();',
            f'global generate_check_transcript_honeypots_random_sample_fraction = {honeypots_query.random_sample_fraction};',
            f'global artifact = "{reactor_consts.get_markup_artifact_path(artifact)}";',
            f'global transcript_strategy = "{transcript_strategy.value}";',
            f'global transcript_feedback_loop_disabled = {bool_to_str(transcript_feedback_loop_disabled)};',
            f'global check_asr_disabled = {bool_to_str(check_asr_disabled)};',
            f'global quality_evaluation_disabled = {bool_to_str(quality_evaluation_disabled)};',
            f'global check_asr_pool_params_override = "{dict_to_str(check_asr_pool_params_override)}";',
            f'global transcript_pool_params_override = "{dict_to_str(transcript_pool_params_override)}";',
            f'global quality_evaluation_pool_params_override = "{dict_to_str(quality_evaluation_pool_params_override)}";',
            f'global toloka_environment = Datum.nirvanaEnum("{toloka_environment}");',
            f'global toloka_token = Datum.nirvanaSecret("{get_toloka_token()}");',
            f'global upload_toloka_audio_to_mds = {bool_to_str(upload_toloka_audio_to_mds)};',
            f'global concurrent_session = "{concurrent_session}";',
        ])),
        artifact_trigger=r.r_objs.DynamicArtifactTrigger(triggers=[
            reactor_consts.get_markup_artifact_reference_for_trigger(artifact),
        ]),
        # cron_trigger=r.r_objs.CronTrigger(cron_expression='0 0 1 ? * MON-FRI'),
    )


def generate_concurrent_triggers_and_artifacts(
    client: r.ReactorAPIClientV1,
    artifact_base_name: str,
    concurrent_sessions: int = 1,
    **kwargs,
) -> typing.List[r.r_objs.DynamicTrigger]:
    triggers = []
    for session in range(concurrent_sessions):
        artifact = f'{artifact_base_name}-{session + 1}'
        concurrent_session = f'{session + 1}/{concurrent_sessions}' if concurrent_sessions > 1 else ''

        generate_artifact_for_trigger(client, artifact)
        triggers.append(generate_trigger(concurrent_session=concurrent_session, artifact=artifact, **kwargs))

    return triggers
