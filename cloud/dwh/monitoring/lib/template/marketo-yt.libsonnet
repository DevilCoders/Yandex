local common = import '../common.libsonnet';
local time = import '../time.libsonnet';

{
    notification_description:: 'Problem with marketo_yt task',

    service: error 'Сheck must have service field',
    responsible: common.responsible,
    namespace: common.namespace,
    refresh_time: 5 * time.minute,
}
