let mins =  group_lines(
    'min',
    {
        project='<< project_id >>',
        cluster='<< backup_service.get("cluster") >>',
        service='<< backup_service.get("service") >>',
        sensor='mdb_backup_scheduler_exit_code',
        cluster_type='<< backup_service_cluster_type_val >>',
        action='<< backup_scheduler_action >>'
    }
);

alarm_if(min(mins) > 0);
