local template = import '../template/scheduled-job.libsonnet';
local time = import '../time.libsonnet';

local scheduler_url_tmpl = 'http://sandbox.yandex-team.ru/scheduler/%s/tasks';

local sandbox_job = template {
    local check = self,

    scheduler_id:: error 'Sandbox job check must have scheduler_id field',
    scheduler_url :: scheduler_url_tmpl % check.scheduler_id,

    host: 'cloud-analytics-scheduled-jobs',
    tags: ['cloud_analytics', 'scheduler_id.%d' % check.scheduler_id],
    #mark: 'cloud_analytics',
    meta: {
        urls: [
            {
                title: "Шедулер в Sandbox",
                url: check.scheduler_url,
            }
        ],
    },
    aggregator_kwargs+: {
        crit_desc: '%s: %s' % [check.notification_description, check.scheduler_url],
        nodata_desc: '%s: %s' % [check.notification_description, check.scheduler_url],
    }
};
local marketo_sandbox_job = sandbox_job {
    tags+: ['marketo'],
};
local crm_sandbox_job = sandbox_job {
    tags+: ['crm']
};

[
    sandbox_job {
        scheduler_id:: 14683,

        service: 'crm-leads-to-dyn-table',
        ttl: 12 * time.hour,
    },
    sandbox_job {
        scheduler_id:: 12877,

        service: 'passport-uid-ba',
        ttl: 6 * time.hour,
    },
    sandbox_job {
        scheduler_id:: 11721,

        service: 'solomon-to-yt',
        ttl: 12 * time.hour,
    },
    sandbox_job {
        scheduler_id:: 11960,

        service: 'archive-solomon-imports',
        ttl: 2 * time.day,
    },
    sandbox_job {
        scheduler_id:: 13164,

        service: 'promo-events-from-st-to-yt',
        ttl: 2 * time.day,
    },
    sandbox_job {
        scheduler_id:: 16682,

        service: 'import-leads-to-dyn-table',
        ttl: 1.5 * time.day,
    },

    marketo_sandbox_job {
        scheduler_id:: 12212,

        service: 'marketo-export-leads',
        ttl: 2 * time.day,
    },
    marketo_sandbox_job {
        scheduler_id:: 12428,

        service: 'marketo-export-ya-visit-webpage',
        ttl: 2 * time.day,
    },
    marketo_sandbox_job {
        scheduler_id:: 17756,

        service: 'marketo-export-ya-fill-out-form',
        ttl: 2 * time.day,
    },
    crm_sandbox_job {
        scheduler_id:: 17143,

        service: 'b2b-request-to-crm',
        ttl: 4 * time.hour,
    },
    crm_sandbox_job {
        scheduler_id:: 15288,

        service: 'crm-export-mql',
        ttl: time.day + 4 * time.hour,
    },
    crm_sandbox_job {
        scheduler_id:: 11240,

        service: 'crm-export-sales',
        ttl: 2 * time.hour,
    },
    crm_sandbox_job {
        scheduler_id:: 14496,

        service: 'crm-export-consulting-requests',
        ttl: 2 * time.hour,
    },
    sandbox_job {
        scheduler_id:: 17272,

        service: 'acquisition-cube-yt-to-ch',
        ttl: 7 * time.hour,
    },
]
