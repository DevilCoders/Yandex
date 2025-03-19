CREATE OR REPLACE FUNCTION code.cluster_status_acquire_transitions()
RETURNS TABLE (
    from_status dbaas.cluster_status,
    to_status dbaas.cluster_status,
    action dbaas.action
) AS $$
SELECT from_status::dbaas.cluster_status,
       to_status::dbaas.cluster_status,
       action::dbaas.action
  FROM (
    VALUES
        ('STOPPED', 'RESTORING-OFFLINE', 'cluster-offline-resetup'),
        ('RUNNING', 'RESTORING-ONLINE', 'cluster-resetup'),
        ('RESTORE-OFFLINE-ERROR', 'RESTORING-OFFLINE', 'cluster-offline-resetup'),
        ('RESTORE-ONLINE-ERROR', 'RESTORING-ONLINE', 'cluster-resetup'),
        ('STOP-ERROR', 'STOPPING', 'cluster-stop'),
        ('START-ERROR', 'STARTING', 'cluster-start'),
        ('CREATE-ERROR', 'CREATING', 'cluster-create'),
        ('RUNNING', 'MODIFYING', 'cluster-modify'),
        ('MODIFY-ERROR', 'MODIFYING', 'cluster-modify'),
        ('DELETE-ERROR', 'DELETING', 'cluster-delete'),
        ('PURGE-ERROR', 'PURGING', 'cluster-purge'),
        ('DELETED', 'METADATA-DELETING', 'cluster-delete-metadata'),
        ('METADATA-DELETE-ERROR', 'METADATA-DELETING', 'cluster-delete-metadata'),
        -- cluster-purge tasks are delayed,
        -- so status is switched on task acquirement
        ('METADATA-DELETED', 'PURGING', 'cluster-purge'),
        ('DELETED', 'PURGING', 'cluster-purge'),
        ('RUNNING', 'MODIFYING', 'cluster-maintenance'),
        ('MODIFY-ERROR', 'MODIFYING', 'cluster-maintenance'),
        -- delayed tasks should have same status transitions
        -- so interrupted tasks can be acquired
        ('PURGING', 'PURGING', 'cluster-purge'),
        ('MODIFYING', 'MODIFYING', 'cluster-maintenance'),
        ('METADATA-DELETING', 'METADATA-DELETING', 'cluster-delete-metadata'),
        ('STOPPED', 'MAINTAINING-OFFLINE', 'cluster-maintenance'),
        -- failed offline maintenance can be restart
        ('MAINTAIN-OFFLINE-ERROR', 'MAINTAINING-OFFLINE', 'cluster-maintenance'),
        -- running maintenance cat be released and restarted by another worker
        ('MAINTAINING-OFFLINE', 'MAINTAINING-OFFLINE', 'cluster-maintenance')
       ) AS t(from_status, to_status, action);
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.cluster_status_finish_transitions()
RETURNS TABLE (
    from_status dbaas.cluster_status,
    to_status dbaas.cluster_status,
    action dbaas.action,
    result boolean
) AS $$
SELECT from_status::dbaas.cluster_status,
       to_status::dbaas.cluster_status,
       action::dbaas.action,
       result::boolean
  FROM (
    VALUES
        ('CREATING', 'RUNNING', 'cluster-create', true),
        ('CREATING', 'CREATE-ERROR', 'cluster-create', false),
        ('STOPPING', 'STOPPED', 'cluster-stop', true),
        ('STOPPING', 'STOP-ERROR', 'cluster-stop', false),
        ('STARTING', 'RUNNING', 'cluster-start', true),
        ('STARTING', 'START-ERROR', 'cluster-start', false),
        ('MODIFYING', 'RUNNING', 'cluster-modify', true),
        ('MODIFYING', 'MODIFY-ERROR', 'cluster-modify', false),
        ('RESTORING-ONLINE', 'RUNNING', 'cluster-resetup', true),
        ('RESTORING-ONLINE', 'RESTORE-ONLINE-ERROR', 'cluster-resetup', false),
        ('RESTORING-OFFLINE', 'STOPPED', 'cluster-offline-resetup', true),
        ('RESTORING-OFFLINE', 'RESTORE-OFFLINE-ERROR', 'cluster-offline-resetup', false),
        ('MODIFYING', 'RUNNING', 'cluster-maintenance', true),
        ('MODIFYING', 'MODIFY-ERROR', 'cluster-maintenance', false),
        ('DELETING', 'DELETED', 'cluster-delete', true),
        ('DELETING', 'DELETE-ERROR', 'cluster-delete', false),
        ('METADATA-DELETING', 'METADATA-DELETED', 'cluster-delete-metadata', true),
        ('METADATA-DELETING', 'METADATA-DELETE-ERROR', 'cluster-delete-metadata', false),
        ('PURGING', 'PURGED', 'cluster-purge', true),
        ('PURGING', 'PURGE-ERROR', 'cluster-purge', false),
        ('MAINTAINING-OFFLINE', 'STOPPED', 'cluster-maintenance', true),
        ('MAINTAINING-OFFLINE', 'MAINTAIN-OFFLINE-ERROR', 'cluster-maintenance', false)
) AS t(from_status, to_status, action, result);
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.cluster_status_add_transitions()
RETURNS TABLE (
    from_status dbaas.cluster_status,
    to_status dbaas.cluster_status,
    action dbaas.action
) AS $$
SELECT from_status::dbaas.cluster_status,
       to_status::dbaas.cluster_status,
       action::dbaas.action
  FROM (
    VALUES
        ('CREATING', 'CREATING', 'cluster-create'),
        ('RUNNING', 'MODIFYING', 'cluster-modify'),
        ('RUNNING', 'STOPPING', 'cluster-stop'),
        ('STOPPED', 'STARTING', 'cluster-start'),
        ('RUNNING', 'RESTORING-ONLINE', 'cluster-resetup'),
        ('STOPPED', 'RESTORING-OFFLINE', 'cluster-offline-resetup'),
        ('RUNNING', 'DELETING', 'cluster-delete'),
        ('STOPPED', 'DELETING', 'cluster-delete'),
        ('MODIFYING', 'MODIFYING', 'noop'),
        ('CREATE-ERROR', 'DELETING', 'cluster-delete'),
        ('MODIFY-ERROR', 'DELETING', 'cluster-delete'),
        ('RESTORE-ONLINE-ERROR', 'DELETING', 'cluster-delete'),
        ('RESTORE-OFFLINE-ERROR', 'DELETING', 'cluster-delete'),
        ('STOP-ERROR', 'DELETING', 'cluster-delete'),
        ('START-ERROR', 'DELETING', 'cluster-delete'),
        ('DELETING', 'DELETING', 'cluster-delete-metadata'),
        ('MAINTAIN-OFFLINE-ERROR', 'DELETING', 'cluster-delete')
) AS t(from_status, to_status, action);
$$ LANGUAGE SQL IMMUTABLE;
