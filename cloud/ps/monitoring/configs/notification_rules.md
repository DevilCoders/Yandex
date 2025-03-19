#Notification rules

https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5cb88281dcc66c00751fcf27
host=yaws_filter
    | host=yaws_dataglobal
    | host=yaws_data
    | host=yaws_identity
    | host=yaws_head
    | host=yaws_network
    | host=yaws_object
    | host=yaws_compute
    | host=yaws_alpha
    | host=gp2_mfsmaster
    | host=gp2_mpi_mfs
    | host=gp2_nodes
    | host=gp2_stable_db
    | (host=cloud-analytics-scheduled-jobs & tag=call)
    | host=yaws-alpha-api.yandex.net
    | host=yaws-alpha-admin.yandex.net
    | host=yaws-alpha-iva5-api.yandex.net
