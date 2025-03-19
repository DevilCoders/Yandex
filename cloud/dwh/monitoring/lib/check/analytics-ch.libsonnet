local template = import '../common.libsonnet';
local time = import '../time.libsonnet';
local telegram = import '../notification/telegram.libsonnet';
local startrek = import '../notification/startrek.libsonnet';

[
    template {
        notification_description :: 'Problem with ClickHouse replication',
        host: "cloud-analytics-ch",
        service: "replication",
        tags: ['cloud_analytics', 'clickhouse'],
        aggregator: 'logic_or',
        aggregator_kwargs+: {
            crit_desc: $.notification_description,
            ok_desc: 'ClickHouse cluster replication OK',
        },
        ttl: 10 * time.minute,
        meta: {
            urls: [
                {
                    title: "Графики в Solomon",
                    url: "https://solomon.yandex-team.ru/?project=internal-mdb&cluster=mdb_07bc5e8c-c4a7-4c26-b668-5a1503d858b9&service=mdb&sensor=ch_replication-future_parts%7Cch_replication-parts_to_check%7Cch_replication-inserts_in_queue%7Cch_replication-merges_in_queue&dc=!nodc&graph=auto&stack=false&l.shard=*&l.node=replica&host=!by_node&b=4h&e=",
                },
                {
                    title: "Панель управления кластером CH",
                    url: "https://yc.yandex-team.ru/folders/foo1ql4gh577phgchar0/managed-clickhouse/cluster/07bc5e8c-c4a7-4c26-b668-5a1503d858b9",
                },
            ],
        },
        children: [
            {
                'group_type': 'EVENTS',
                'host': 'tag=cloud_analytics&tag=replication',
                'service': 'all',
            },
        ],
        notifications: [
            telegram {
                description: $.notification_description,
                template_name: 'on_status_change',
            },
            startrek {
                description: $.notification_description,
            }
        ],
    },
]

