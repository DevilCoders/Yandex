from juggler_sdk import NotificationOptions

SERVICE_SCHEDULE_ABC = "@svc_antiadblock:antiadb_main"
RESERVE_SCHEDULE_ABC = "@svc_antiadblock:antiadb_reserve"

TELEGRAM = NotificationOptions(
    template_name='on_status_change',
    template_kwargs=dict(
        status={'to': 'CRIT', 'from': 'OK'},
        method='telegram',
        login=['ANTIADBMonitoring']
    ),
    description='Telegram errors notifications'
)


DAY_PHONE = NotificationOptions(
    template_name='phone_escalation',
    template_kwargs=dict(
        time_start='09:00',
        time_end='23:00',
        logins=[SERVICE_SCHEDULE_ABC, RESERVE_SCHEDULE_ABC],
        delay=120
    ),
    description='Phone errors notifications'
)


NIGHT_PHONE = NotificationOptions(
    template_name='phone_escalation',
    template_kwargs=dict(
        time_start='23:00',
        time_end='09:00',
        logins=[SERVICE_SCHEDULE_ABC, RESERVE_SCHEDULE_ABC],
        delay=120
    ),
    description='Phone errors notifications'
)
