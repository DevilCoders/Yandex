local template = import '../common.libsonnet';
local time = import '../time.libsonnet';
local telegram = import '../notification/telegram.libsonnet';
local startrek = import '../notification/startrek.libsonnet';

local common_elastic_http_status(list) = template {
    refresh_time: time.minute,
    ttl: time.minute * 10,
    tags: ['cloud-crm', 'cloud-crm-elastic'],

    service: 'http_' + std.join('_', list),

    active: 'http',
    active_kwargs: {
        disable_ipv4: 'true',
        path: '/_cat/health?h=st',
        regexp: '(' + std.join('|', list) + ')',
        port: '9200',
        timeout: time.minute
    },
    flaps: {
        stable: time.minute,
        critical: time.minute * 5,
        boost: 0
    }
};

local nodes = ['ela1', 'ela2', 'ela3'];
local services = ['http_green', 'http_yellow_green'];

local common_elastic_http_aggr = template {
    local notification_description = 'Problem with crm-elastic',

    host: 'crm_elastic',

    aggregator: 'more_than_limit_is_problem',
    aggregator_kwargs: {
        mode: 'normal',
        crit_limit: '5',
        warn_limit: '1',
        nodata_mode: 'force_crit'
    },

    notifications: [
        telegram {
            description: notification_description,
            template_name: 'on_desc_change',
        },
        startrek {
            description: notification_description,
        }
    ],

    tags: ['cloud-crm', 'cloud-crm-elastic', 'call'],
    meta: {
        urls: [
            {
                title: 'Окружение в Qloud',
                url: 'https://platform.yandex-team.ru/projects/ya-cloud-crm/prod/elastic',
            },
        ],
    },
    children: [
        {
            host: node + '.elastic.prod.ya-cloud-crm.stable.qloud-d.yandex.net',
            service: svc
        } for node in nodes
        for svc in services
    ]
};

std.flattenArrays(
    [
        [
            common_elastic_http_aggr {
                service: 'elastic'
            }
        ],
        [
            common_elastic_http_status(['yellow', 'green']) {
                host: node + '.elastic.prod.ya-cloud-crm.stable.qloud-d.yandex.net'
            } for node in nodes
        ],
        [
            common_elastic_http_status(['green']) {
                host: node + '.elastic.prod.ya-cloud-crm.stable.qloud-d.yandex.net'
            } for node in nodes
        ]
    ]
)
