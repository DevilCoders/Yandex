CREATE UNIQUE INDEX one_running_operation ON cms.instance_operations (instance_id, operation_type) WHERE (status != 'ok');
