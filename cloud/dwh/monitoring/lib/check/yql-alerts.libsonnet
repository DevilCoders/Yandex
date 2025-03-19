local template = import '../template/yql-alerts.libsonnet';
local time = import '../time.libsonnet';

local yql_alert = template {
    local check = self,
    local scheduler_url =  "http://sandbox.yandex-team.ru/scheduler/18004/tasks",

    host: 'cloud-analytics-yql-alerts',
    tags: ['cloud_analytics', 'scheduler_id.18004'],
    ttl: 1 * time.day + 2 * time.hour,
    meta: {
        urls: [
            {
                title: "Шедулер в Sandbox",
                url: scheduler_url,
            }
        ],
    },
    aggregator_kwargs+: {
        crit_desc: '%s: %s' % [check.notification_description, scheduler_url],
        nodata_desc: '%s: %s' % [check.notification_description, scheduler_url],
    }
};

[
    yql_alert { service: s }
    for s in [
        'lost_before_crm',
        'lost_exported_leads',
        'lost_in_dyn_table',
        'broken_email_indx',
        'crm_id_dup',
        'broken_crm_indx',
        'broken_passport_indx',
        'lost_before_marketo',
        'lost_in_marketo',
        'not_become_mql',
        'empty_email',
        'empty_individual',
        'empty_phone',
    ]
]
