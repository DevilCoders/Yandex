local template = import '../template/marketo-yt.libsonnet';
local time = import '../time.libsonnet';
local telegram = import '../notification/telegram.libsonnet';
local startrek = import '../notification/startrek.libsonnet';

local common_marketo_yt_task = template {
    local check = self,
    local component = 'ya-cloud-analytics-personal.dwh-marketo-sync.production.worker',

    host: 'marketo_yt',

    tags: ['cloud_analytics', 'marketo', 'dwh'],
    meta: {
        urls: [
            {
                title: 'Окружение в Qloud',
                url: 'https://qloud-ext.yandex-team.ru/projects/ya-cloud-analytics-personal/dwh-marketo-sync/production',
            },
        ],
    },
    children: [
        {
            'group_type': 'QLOUD',
            'host': component + '@type=ext',
            'service': 'agent.tasks.' + check.service,
        }
    ]
};

local critical_marketo_yt_task = common_marketo_yt_task {
    aggregator: 'logic_or',
    aggregator_kwargs: {
        nodata_mode: 'force_crit',
    },
    notifications: [
        telegram {
            description: $.notification_description,
            template_name: 'on_desc_change',
        },
        startrek {
            description: $.notification_description,
        }
    ],
};

local warning_marketo_yt_task = common_marketo_yt_task {
    aggregator: 'expr',
    aggregator_kwargs: {
        nodata_mode: 'force_warn',
        WARN: "(CRIT + WARN) > 0",
        fallback: null,
    },

    notifications: [
        startrek {
            description: $.notification_description,
        }
    ],
};


[
    critical_marketo_yt_task {
        service: 'transfer_data_in_both_directions',
        ttl: 6 * time.hour,
    },

    warning_marketo_yt_task {
        service: 'clear_finished_migrations',
        ttl: 8 * time.hour,
    },

    warning_marketo_yt_task {
        service: 'delete_obsolete_migrations',
        ttl: 7 * time.day,
    },
]
