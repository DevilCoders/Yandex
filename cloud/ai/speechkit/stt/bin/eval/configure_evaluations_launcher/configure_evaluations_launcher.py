import json
import os
import time
import typing

import reactor_client as r

import cloud.ai.speechkit.stt.lib.reactor as reactor_consts

source_flow_id = 'd720620e-4fa0-4fb3-b6b5-d52400036b7a'
target_flow_id = '9104ea62-47c3-426b-bdba-a64676de7b5b'

operation_select_records_joins = 'operation-d2cb14e0-8f62-401a-8009-e5298f9b6462'
operation_recognize = 'operation-b5676ba2-84e5-4e96-b059-f379a2544965'
operation_generate_slices = 'operation-db4040b9-e544-40c9-93fa-7820267df8fe'
operation_cluster_references = 'operation-31d48839-358c-4301-8fe7-63b84bf7a8c9$1'
operation_stop_words = 'operation-eefa8dbd-8388-4696-8ad0-29c67a4304b7$1'
operation_calculate_metrics = 'operation-fd3c7b31-d46d-48a0-84a8-f0238d7dc7a0'

tomorrow = 'Time.dateNow().plusDays(1).format("yyyy-MM-dd\'T\'00:00:00Z")'
month_ago = 'Time.dateNow().plusMonths(-1).format("yyyy-MM-dd\'T\'HH:mm:ssZ")'
long_time_ago = 'Time.dateNow().plusMonths(-9999).format("yyyy-MM-dd\'T\'HH:mm:ssZ")'  # dirty hack to invalidate caches

ordinal = 0

default_streaming_api_model_pairs = [
    ('yandex', 'general'),
    ('yandex', 'general:rc'),
    # ('tinkoff', ''),
    ('stc', 'IvrRus'),
    ('sber', 'ivr'),
    ('sber', 'callcenter'),
    ('google', ''),
    ('google', 'phone_call'),
]

default_transcription_api_model_pairs = [
    ('yandex', 'general'),
    ('yandex', 'general:rc'),
    ('google', ''),
]

default_yandex_api_model_pairs = [
    ('yandex', 'general'),
    ('yandex', 'general:rc'),
]

default_kk_streaming_api_model_pairs = [
    ('yandex', 'general'),
    ('yandex', 'general:rc'),
    ('stc', 'IVR_Kaz'),
    ('google', '')
]


def main():
    client = r.ReactorAPIClientV1(
        base_url='https://reactor.yandex-team.ru',
        token=os.getenv('REACTOR_TOKEN'),
    )

    create_launcher(
        client=client,
        launcher_name='kpi',
        triggers=[
            *generate_triggers(
                name='primal30',
                tags=[
                    'EXEMPLAR:primal-30',
                ],
                mode='stream',
                api_model_pairs=default_streaming_api_model_pairs,
                slices=['not-autoresponder_not-silence_v1'],
                daily=True,
            ),
            *generate_triggers(
                name='kpi2020',
                tags=[
                    'KPI:2020-07_825_clients_03-06_ui_cc',
                ],
                mode='stream',
                api_model_pairs=default_streaming_api_model_pairs,
                slices=['not-autoresponder_not-silence_v1'],
                daily=True,
            ),
            *generate_triggers(
                name='kpi2021',
                tags=[
                    'KPI:2021-02_960_clients-13_10-01_cc',
                ],
                mode='stream',
                api_model_pairs=default_streaming_api_model_pairs,
                slices=['not-autoresponder_not-silence_v1'],
                daily=True,
            ),
            *generate_triggers(
                name='cc288',
                tags=[
                    'OTHER:yandex-call-center_288_test_2020-01',
                ],
                mode='stream',
                api_model_pairs=default_streaming_api_model_pairs,
                slices=['not-autoresponder_not-silence_v1'],
                daily=True,
            )
        ]
    )

    create_launcher(
        client=client,
        launcher_name='silence',
        triggers=generate_triggers(
            name='silence',
            tags=[
                'OTHER:silence-bits_6522_2020-07-23',
            ],
            mode='stream',
            api_model_pairs=default_streaming_api_model_pairs,
            bits=True,
        )
    )

    create_launcher(
        client=client,
        launcher_name='clients_stream',
        triggers=generate_triggers(
            name='clients_stream',
            tags=[
                'CLIENT:fromtech-qnd MODE:stream LANG:ru-RU PERIOD:2021-05',
                'CLIENT:kaspi-bank-rfr MODE:stream LANG:ru-RU PERIOD:2021-05',
                'CLIENT:mtt-u16 MODE:stream LANG:ru-RU PERIOD:2021-05',
                'CLIENT:neuro-net-mjq MODE:stream LANG:ru-RU PERIOD:2021-05',
                'CLIENT:prof-it-jja MODE:stream LANG:ru-RU PERIOD:2021-03',
                'CLIENT:s7-85t MODE:stream LANG:ru-RU PERIOD:2021-05',
                'CLIENT:web-banker-ke3 MODE:stream LANG:ru-RU PERIOD:2021-05',
                'CLIENT:zvonobot-c2a MODE:stream LANG:ru-RU PERIOD:2021-05',
                'CLIENT:zvonobot-h64 MODE:stream LANG:ru-RU PERIOD:2021-05',
                'CLIENT:ats-7np MODE:stream LANG:ru-RU PERIOD:2021-07',
                'CLIENT:ats-jb9 MODE:stream LANG:ru-RU PERIOD:2021-07',
                'CLIENT:alfa-bank-lal MODE:stream LANG:ru-RU PERIOD:2021-10',
            ],
            mode='stream',
            api_model_pairs=default_streaming_api_model_pairs,
            duration_limit_minutes_per_tag=60.0,
            join_received_before=tomorrow,
        )
    )

    create_launcher(
        client=client,
        launcher_name='clients_stream_tail',
        triggers=generate_triggers(
            name='clients_stream_tail',
            tags=[
                '~CLIENT MODE:stream LANG:ru-RU',
            ],
            mode='stream',
            api_model_pairs=default_streaming_api_model_pairs,
            duration_limit_minutes_per_tag=240.0,
            join_received_after=month_ago,
            filter_tags=False,
            join_received_before=tomorrow,
        )
    )

    create_launcher(
        client=client,
        launcher_name='clients_long',
        triggers=generate_triggers(
            name='clients_long',
            tags=[
                'CLIENT:yandex-vertikali-ir1 MODE:long LANG:ru-RU PERIOD:2021-03',
                'CLIENT:qiwi-bank-f1o MODE:long LANG:ru-RU PERIOD:2021-03',
                'CLIENT:voca-gvr MODE:long LANG:ru-RU PERIOD:2021-10',
                'CLIENT:voca-5c1 MODE:long LANG:ru-RU PERIOD:2021-10',
                'CLIENT:voca-aoh MODE:long LANG:ru-RU PERIOD:2021-09',
                'CLIENT:voca-d6m MODE:long LANG:ru-RU PERIOD:2021-08',
                'CLIENT:voca-magnit-chg MODE:long LANG:ru-RU PERIOD:2021-10',
                'CLIENT:voca-73u MODE:long LANG:ru-RU PERIOD:2022-03'
            ],
            mode='long',
            api_model_pairs=default_transcription_api_model_pairs,
            duration_limit_minutes_per_tag=300.0,
            join_received_before=tomorrow,
        )
    )

    create_launcher(
        client=client,
        launcher_name='clients_long_tail',
        triggers=generate_triggers(
            name='clients_long_tail',
            tags=[
                '~CLIENT MODE:long LANG:ru-RU',
            ],
            mode='long',
            api_model_pairs=default_transcription_api_model_pairs,
            duration_limit_minutes_per_tag=600.0,
            join_received_after=month_ago,
            filter_tags=False,
            join_received_before=tomorrow,
        )
    )

    create_launcher(
        client=client,
        launcher_name='voice_recorder',
        triggers=generate_triggers(
            name='voice_recorder',
            tags=[
                'ACOUSTIC:voice-recorder LANG:ru-RU PERIOD:2022-01 DATASET:fio',
                'ACOUSTIC:voice-recorder LANG:ru-RU PERIOD:2022-01 DATASET:addresses',
                'ACOUSTIC:voice-recorder LANG:ru-RU PERIOD:2022-01 DATASET:alpha',
                'ACOUSTIC:phone LANG:ru-RU PERIOD:2022-01 DATASET:books',
                'ACOUSTIC:phone LANG:ru-RU PERIOD:2022-01 DATASET:news',
                'ACOUSTIC:phone LANG:ru-RU PERIOD:2022-01 DATASET:b1gnjigi42vcj28c6e7m/fio',
                'ACOUSTIC:phone LANG:ru-RU PERIOD:2022-02 DATASET:b1g1283fivue3pekum2p/food-menu-asian',
                'ACOUSTIC:phone LANG:ru-RU PERIOD:2022-02 DATASET:b1ge9s4vqcupb0qjkft6/materials',
            ],
            mode='stream',
            api_model_pairs=default_streaming_api_model_pairs,
            duration_limit_minutes_per_tag=60.0,
            join_received_before=tomorrow
        )
    )

    create_launcher(
        client=client,
        launcher_name='multilang_streaming',
        triggers=[
            *generate_triggers(
                name='voice_recorder',
                tags=[
                    'ACOUSTIC:phone LANG:ru-RU PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:kk-KK PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:fr-FR PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:de-DE PERIOD:2021-12',
                    'ACOUSTIC:phone LANG:fi-FI PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:sv-SV PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:tr-TR PERIOD:2022-05',
                    'ACOUSTIC:voice-recorder LANG:en-US PERIOD:2021-08',
                    'ACOUSTIC:voice-recorder LANG:fr-FR PERIOD:2021-07',
                    'ACOUSTIC:voice-recorder LANG:de-DE PERIOD:2021-08',
                    'IMPORT:yt-table_mls-english',
                    'IMPORT:yt-table_mls-french',
                    'IMPORT:yt-table_mls-german',
                    'IMPORT:yt-table_mozilla-common-voice-english',
                    'IMPORT:yt-table_mozilla-common-voice-french',
                    'IMPORT:yt-table_mozilla-common-voice-german',
                    'IMPORT:yt-table_youtube-handcrafted2-random',
                    'IMPORT:yt-table_youtube-handcrafted-random',
                    'IMPORT:yt-table_youtube-auto-random',
                    'IMPORT:yt-table_youtube-auto',
                    'IMPORT:yt-table_kp-random',
                ],
                mode='stream',
                api_model_pairs=[
                    ('yandex', 'general'),
                    ('yandex', 'general:rc'),
                    ('google', ''),
                ],
                yandex_multilang=False,
                duration_limit_minutes_per_tag=120.0
            ),
            *generate_triggers(
                name='voice_recorder_auto',
                tags=[
                    'ACOUSTIC:phone LANG:ru-RU PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:kk-KK PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:fr-FR PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:de-DE PERIOD:2021-12',
                    'ACOUSTIC:phone LANG:fi-FI PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:sv-SV PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:tr-TR PERIOD:2022-05',
                    'ACOUSTIC:voice-recorder LANG:en-US PERIOD:2021-08',
                    'ACOUSTIC:voice-recorder LANG:fr-FR PERIOD:2021-07',
                    'ACOUSTIC:voice-recorder LANG:de-DE PERIOD:2021-08',
                    'IMPORT:yt-table_mls-english',
                    'IMPORT:yt-table_mls-french',
                    'IMPORT:yt-table_mls-german',
                    'IMPORT:yt-table_mozilla-common-voice-english',
                    'IMPORT:yt-table_mozilla-common-voice-french',
                    'IMPORT:yt-table_mozilla-common-voice-german',
                    'IMPORT:yt-table_youtube-handcrafted2-random',
                    'IMPORT:yt-table_youtube-handcrafted-random',
                    'IMPORT:yt-table_youtube-auto-random',
                    'IMPORT:yt-table_youtube-auto',
                    'IMPORT:yt-table_kp-random',
                ],
                mode='stream',
                api_model_pairs=[
                    ('yandex', 'general'),
                    ('yandex', 'general:rc')
                ],
                yandex_multilang=True,
                duration_limit_minutes_per_tag=120.0
            ),
            # google phone_call model does not support german
            *generate_triggers(
                name='voice_recorder',
                tags=[
                    'ACOUSTIC:phone LANG:ru-RU PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:fr-FR PERIOD:2022-01',
                    'ACOUSTIC:voice-recorder LANG:en-US PERIOD:2021-08',
                    'ACOUSTIC:voice-recorder LANG:fr-FR PERIOD:2021-07',
                    'IMPORT:yt-table_mls-english',
                    'IMPORT:yt-table_mls-french',
                    'IMPORT:yt-table_mozilla-common-voice-english',
                    'IMPORT:yt-table_mozilla-common-voice-french',
                    'IMPORT:yt-table_youtube-handcrafted2-random',
                    'IMPORT:yt-table_youtube-handcrafted-random',
                    'IMPORT:yt-table_youtube-auto-random',
                    'IMPORT:yt-table_youtube-auto',
                    'IMPORT:yt-table_kp-random',
                ],
                mode='stream',
                api_model_pairs=[
                    ('google', 'phone_call'),
                ],
                yandex_multilang=False,
                duration_limit_minutes_per_tag=120.0
            )
        ],
        use_lemm_norm=False
    )

    create_launcher(
        client=client,
        launcher_name='multilang_transcription',
        triggers=[
            *generate_triggers(
                name='voice_recorder',
                tags=[
                    'ACOUSTIC:phone LANG:ru-RU PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:kk-KK PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:fr-FR PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:de-DE PERIOD:2021-12',
                    'ACOUSTIC:phone LANG:fi-FI PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:sv-SV PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:tr-TR PERIOD:2022-05',
                    'ACOUSTIC:voice-recorder LANG:tr-TR PERIOD:2021-08',
                    'ACOUSTIC:voice-recorder LANG:en-US PERIOD:2021-08',
                    'ACOUSTIC:voice-recorder LANG:fr-FR PERIOD:2021-07',
                    'ACOUSTIC:voice-recorder LANG:de-DE PERIOD:2021-08',
                    'IMPORT:yt-table_mls-english',
                    'IMPORT:yt-table_mls-french',
                    'IMPORT:yt-table_mls-german',
                    'IMPORT:yt-table_mozilla-common-voice-english',
                    'IMPORT:yt-table_mozilla-common-voice-french',
                    'IMPORT:yt-table_mozilla-common-voice-german',
                    'IMPORT:yt-table_youtube-handcrafted2-random',
                    'IMPORT:yt-table_youtube-handcrafted-random',
                    'IMPORT:yt-table_youtube-auto-random',
                    'IMPORT:yt-table_youtube-auto',
                    'IMPORT:yt-table_kp-random',
                ],
                mode='long',
                api_model_pairs=[
                    ('yandex', 'general'),
                    ('yandex', 'general:rc'),
                    ('google', ''),
                ],
                yandex_multilang=False,
                duration_limit_minutes_per_tag=120.0
            ),
            *generate_triggers(
                name='voice_recorder_auto',
                tags=[
                    'ACOUSTIC:phone LANG:ru-RU PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:kk-KK PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:fr-FR PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:de-DE PERIOD:2021-12',
                    'ACOUSTIC:phone LANG:fi-FI PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:sv-SV PERIOD:2022-01',
                    'ACOUSTIC:phone LANG:tr-TR PERIOD:2022-05',
                    'ACOUSTIC:voice-recorder LANG:tr-TR PERIOD:2021-08',
                    'ACOUSTIC:voice-recorder LANG:en-US PERIOD:2021-08',
                    'ACOUSTIC:voice-recorder LANG:fr-FR PERIOD:2021-07',
                    'ACOUSTIC:voice-recorder LANG:de-DE PERIOD:2021-08',
                    'IMPORT:yt-table_mls-english',
                    'IMPORT:yt-table_mls-french',
                    'IMPORT:yt-table_mls-german',
                    'IMPORT:yt-table_mozilla-common-voice-english',
                    'IMPORT:yt-table_mozilla-common-voice-french',
                    'IMPORT:yt-table_mozilla-common-voice-german',
                    'IMPORT:yt-table_youtube-handcrafted2-random',
                    'IMPORT:yt-table_youtube-handcrafted-random',
                    'IMPORT:yt-table_youtube-auto-random',
                    'IMPORT:yt-table_youtube-auto',
                    'IMPORT:yt-table_kp-random',
                ],
                mode='long',
                api_model_pairs=[
                    ('yandex', 'general'),
                    ('yandex', 'general:rc'),
                ],
                yandex_multilang=True,
                duration_limit_minutes_per_tag=120.0
            )
        ],
        use_lemm_norm=False
    )

    create_launcher(
        client=client,
        launcher_name='misc_long',
        triggers=generate_triggers(
            name='misc_long',
            tags=[
                'OTHER:yandex-call-center_288_test_2020-01',
                'IMPORT:yt-table_biovitrum_2019',
                'IMPORT:files_russia-24_2020-05',
                # 'IMPORT:files_tvc_2020-08-18',
            ],
            mode='long',
            api_model_pairs=default_transcription_api_model_pairs,
            duration_limit_minutes_per_tag=1200.0,
        )
    )

    create_launcher(
        client=client,
        launcher_name='kk_stream',
        triggers=generate_triggers(
            name='kk_stream',
            tags=[
                'CLIENT:kaspi-bank-rfr LANG:kk-KK PERIOD:2021-10',
                'ACOUSTIC:phone LANG:kk-KK PERIOD:2021-10',
                'ACOUSTIC:phone LANG:kk-KK PERIOD:2022-01',
            ],
            mode='stream',
            api_model_pairs=default_kk_streaming_api_model_pairs,
            lang='kk-KK',
            duration_limit_minutes_per_tag=300.0,
        ),
        use_lemm_norm=False
    )

    create_launcher(
        client=client,
        launcher_name='public_datasets',
        triggers=generate_triggers(
            name='public_datasets',
            tags=[
                'IMPORT:datasphere FOLDER:mlcloud DATASET:sbergolos-farfield',
                'IMPORT:datasphere FOLDER:mlcloud DATASET:sbergolos-crowd',
                'IMPORT:yt-table_mozilla-commonvoice',
            ],
            mode='stream',
            api_model_pairs=default_streaming_api_model_pairs,
            duration_limit_minutes_per_tag=300.0,
        )
    )

    create_launcher(
        client=client,
        launcher_name='khural',
        triggers=generate_triggers(
            name='khural',
            tags=[
                'IMPORT:khural',
            ],
            mode='long',
            api_model_pairs=default_yandex_api_model_pairs,
            bits=True,
        )
    )


def create_launcher(client: r.ReactorAPIClientV1, launcher_name: str, triggers: list, use_lemm_norm: bool = True):
    if len(triggers) <= 100:
        create_single_launcher(client, f'{launcher_name}', triggers, use_lemm_norm)
        return

    for i, triggers_start in enumerate(range(0, len(triggers), 100)):
        triggers_end = min(triggers_start + 100, len(triggers))
        create_single_launcher(client, f'{launcher_name}_{i}', triggers[triggers_start:triggers_end], use_lemm_norm)


def create_single_launcher(client: r.ReactorAPIClientV1, launcher_name, triggers: list, use_lemm_norm: bool):
    builder = r.reaction_builders.NirvanaReactionBuilder()
    builder.set_version(2)

    builder.set_source_graph(flow_id='d720620e-4fa0-4fb3-b6b5-d52400036b7a')
    builder.set_target_graph(flow_id='be22a1bf-7518-4c01-9949-fc8591d12ed7')
    builder.set_reaction_path(
        f'/cloud/ai/speechkit/stt/quality-evaluation/launcher_{launcher_name}',
        'ASR quality evaluation launcher',
    )

    builder.set_project(reactor_consts.project_identifier)
    builder.set_owner(reactor_consts.reaction_owner)

    set_global_params(builder)
    set_common_params(builder, use_lemm_norm)

    builder.set_dynamic_triggers(triggers)

    namespace_id = builder.operation_descriptor.namespace_desc.namespace_identifier
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
            r.r_objs.NotificationDescriptor(
                namespace_identifier=r.r_objs.NamespaceIdentifier(namespace_id=reaction.namespace_id),
                event_type=r.r_objs.NotificationEventType.REACTION_FAILED,
                transport=r.r_objs.NotificationTransportType.TELEGRAM,
                recipient='eranik'
            ),
            r.r_objs.NotificationDescriptor(
                namespace_identifier=r.r_objs.NamespaceIdentifier(namespace_id=reaction.namespace_id),
                event_type=r.r_objs.NotificationEventType.REACTION_RECALCULATION_PROBLEM,
                transport=r.r_objs.NotificationTransportType.TELEGRAM,
                recipient='eranik'
            )
        ]
    )


def generate_triggers(
    name: str,
    tags: typing.List[str],
    mode: str,
    api_model_pairs: typing.List[typing.Tuple[str, str]],
    mark: str = 'TEST',
    slices: typing.List[str] = [],
    lang: str = 'ru-RU',
    yandex_multilang: bool = False,
    bits: bool = False,
    filter_tags: bool = True,
    duration_limit_minutes_per_tag: float = 0.0,
    join_received_before: str = '',
    join_received_after: str = '',
    daily: bool = False
) -> typing.List[r.r_objs.DynamicTrigger]:
    def bool_to_str(b: bool):
        return 'true' if b else 'false'

    def str_list_repr(l: typing.List[str]):
        return ', '.join(f'"{x}"' for x in l)

    def get_date(d: str):
        return d or '""'

    global ordinal

    result = []
    for api, model in api_model_pairs:
        result.append(
            r.r_objs.DynamicTrigger(
                trigger_name=f'{name}_{api}_{model.replace(":", "_") or "default"}',
                expression=r.r_objs.Expression('\n'.join([
                    f'global tags = [{str_list_repr(tags)}].toStringList();',
                    f'global mark = Datum.nirvanaEnum("{mark}");',
                    f'global api = Datum.nirvanaEnum("{api}");',
                    f'global mode = Datum.nirvanaEnum("{mode}");',
                    f'global model = "{model}";',
                    f'global slices = [{str_list_repr(slices)}].toStringList();',
                    f'global lang = "{lang}";',
                    f'global yandex_multilang = {"true" if (api == "yandex" and yandex_multilang) else "false"};',
                    # TODO: use only 'lang' variable when we will have corresponding text resources (i.e. kk-KK)
                    f'global text_resources_lang = "ru-RU";',
                    f'global bits = {bool_to_str(bits)};',
                    f'global filter_tags = {bool_to_str(filter_tags)};',
                    f'global duration_limit_minutes_per_tag = {duration_limit_minutes_per_tag};',
                    f'global join_received_before = {get_date(join_received_before)};',
                    f'global join_received_after = {get_date(join_received_after)};',
                    f'global stream_count = {get_stream_count(api, model)};',
                ])),
                cron_trigger=r.r_objs.CronTrigger(cron_expression=generate_cron_expression(api, daily)),
            ),
        )
        ordinal += 1

    return result


def get_stream_count(api: str, model: str) -> int:
    if api in ('stc', 'sber'):
        return 5
    return 50


def generate_cron_expression(api: str, daily=False) -> str:
    # 1) arrange launches at 15-minute intervals
    # 2) to save money, tinkoff launches only one a week, google and stc launches only one a month

    date_part = '? * SUN'
    if daily:
        date_part = '* * ?'
    if api == 'tinkoff' or api == 'sber':
        date_part = '? * SUN'
    if api == 'google' or api == 'stc':
        date_part = '? * SUN#3'

    hour = (ordinal * 15) // 60
    minute = (ordinal * 15) % 60

    return f'0 {minute} {hour} {date_part}'


def set_global_params(builder: r.reaction_builders.NirvanaReactionBuilder):
    builder.set_global_param_to_value_with_hints('enable-storing', True, r.r_objs.VariableTypes.BOOL)
    builder.set_global_param_to_value('yql-token', 'cloud-ai-yql-token')
    builder.set_global_param_to_value('yt-proxy', 'hahn')
    builder.set_global_param_to_value('yt-token', 'cloud-ai-yt-token')
    builder.set_global_param_to_expression_var('yandex-multilang', r.r_objs.ExpressionVariable(var_name='yandex_multilang'))


def set_common_params(builder: r.reaction_builders.NirvanaReactionBuilder, use_lemm_norm: bool):
    builder.set_block_param_to_value(operation_select_records_joins, 'compose-tags', True)
    builder.set_block_param_to_expression_var(
        operation_select_records_joins,
        'tags',
        r.r_objs.ExpressionVariable(var_name='tags'),
    )
    builder.set_block_param_to_expression_var(
        operation_select_records_joins,
        'mark',
        r.r_objs.ExpressionVariable(var_name='mark'),
    )
    builder.set_block_param_to_expression_var(
        operation_select_records_joins,
        'filter-tags',
        r.r_objs.ExpressionVariable(var_name='filter_tags'),
    )
    builder.set_block_param_to_expression_var(
        operation_select_records_joins,
        'bits',
        r.r_objs.ExpressionVariable(var_name='bits'),
    )
    builder.set_block_param_to_expression_var(
        operation_select_records_joins,
        'duration-limit-minutes-per-tag',
        r.r_objs.ExpressionVariable(var_name='duration_limit_minutes_per_tag'),
    )
    builder.set_block_param_to_expression_var(
        operation_select_records_joins,
        'join-received-before',
        r.r_objs.ExpressionVariable(var_name='join_received_before'),
    )
    builder.set_block_param_to_expression_var(
        operation_select_records_joins,
        'join-received-after',
        r.r_objs.ExpressionVariable(var_name='join_received_after'),
    )

    builder.set_block_param_to_value(operation_recognize, 'port', 443)
    builder.set_block_param_to_value(operation_recognize, 'raw-results', True)
    builder.set_block_param_to_value(operation_recognize, 'sensitivity-reduction', False)
    builder.set_block_param_to_value(operation_recognize, 'connection-count', 2)
    builder.set_block_param_to_value(operation_recognize, 'enable-ssl', True)
    builder.set_block_param_to_value(operation_recognize, 'environment', 'prod')
    builder.set_block_param_to_value(operation_recognize, 'flags', '')
    builder.set_block_param_to_value(operation_recognize, 'host', '')
    builder.set_block_param_to_expression_var(
        operation_recognize,
        'stream-count',
        r.r_objs.ExpressionVariable(var_name='stream_count'),
    )
    builder.set_block_param_to_expression_var(
        operation_recognize,
        'api',
        r.r_objs.ExpressionVariable(var_name='api'),
    )
    builder.set_block_param_to_expression_var(
        operation_recognize,
        'mode',
        r.r_objs.ExpressionVariable(var_name='mode'),
    )
    builder.set_block_param_to_expression_var(
        operation_recognize,
        'model',
        r.r_objs.ExpressionVariable(var_name='model'),
    )

    builder.set_block_param_to_value_with_hints(
        operation_generate_slices,
        'custom',
        False,
        r.r_objs.VariableTypes.BOOL,
    )
    builder.set_block_param_to_value_with_hints(
        operation_generate_slices,
        'predicates',
        [],
        r.r_objs.VariableTypes.LIST_STR,
    )
    builder.set_block_param_to_expression_var(
        operation_generate_slices,
        'names',
        r.r_objs.ExpressionVariable(var_name='slices'),
    )

    builder.set_block_param_to_value(operation_cluster_references, 'merge-strategy', 'skip')
    builder.set_block_param_to_value_with_hints(
        operation_cluster_references,
        'revisions',
        [8082542, 8082542, 8082542, 8082542],
        r.r_objs.VariableTypes.LIST_INT,
    )
    builder.set_block_param_to_value_with_hints(
        operation_cluster_references,
        'topics',
        ['common', 'cars', 'abbreviations', 'legacy'],
        r.r_objs.VariableTypes.LIST_STR,
    )
    builder.set_block_param_to_expression_var(
        operation_cluster_references,
        'lang',
        r.r_objs.ExpressionVariable(var_name='text_resources_lang'),
    )

    builder.set_block_param_to_value(operation_stop_words, 'revision', 8082542)
    builder.set_block_param_to_value(operation_stop_words, 'topic', 'general')
    builder.set_block_param_to_expression_var(
        operation_stop_words,
        'lang',
        r.r_objs.ExpressionVariable(var_name='text_resources_lang'),
    )

    builder.set_block_param_to_value(operation_calculate_metrics, 'input-version', 'v2')

    metrics_config = {
        'WER': [
            [],
            ['rem-punct']
        ]
    }
    if use_lemm_norm:
        metrics_config['WER'].extend([['norm'], ['rem-punct', 'norm']])
        metrics_config['WER'].extend([['lemm'], ['rem-punct', 'lemm']])
        metrics_config['WER'].extend([['norm', 'lemm'], ['rem-punct', 'norm', 'lemm']])

    builder.set_block_param_to_value(
        operation_calculate_metrics,
        'metrics-config',
        json.dumps(metrics_config, indent=4),
    )
