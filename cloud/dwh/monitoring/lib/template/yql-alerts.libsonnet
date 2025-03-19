local common = import '../common.libsonnet';
local time = import '../time.libsonnet';
local telegram = import '../notification/telegram.libsonnet';

{
    notification_description:: 'Problem with Analytics Scheduled Job',

    service: error 'Scheduler job check must have service field',
    responsible: common.responsible,
    namespace: common.namespace,
    refresh_time: 10 * time.minute,
    aggregator: 'logic_or',
    aggregator_kwargs: {
        nodata_mode: 'force_crit',
    },
    notifications: [
        telegram {
            description: $.notification_description,
            template_name: 'on_status_change',
        },
    ],
}
