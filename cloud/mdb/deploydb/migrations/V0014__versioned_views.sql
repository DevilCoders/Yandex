CREATE OR REPLACE VIEW deploy.groups_v1 (group_id, name) AS
  SELECT group_id, name
    FROM deploy.groups;

CREATE OR REPLACE VIEW deploy.masters_v1 (master_id, group_id, fqdn, is_open, description, created_at, updated_at, alive_check_at, is_alive) AS
  SELECT master_id, group_id, fqdn, is_open, description, created_at, updated_at, alive_check_at, is_alive
    FROM deploy.masters;

CREATE OR REPLACE VIEW deploy.master_aliases_v1 (master_id, alias) AS
  SELECT master_id, alias
    FROM deploy.master_aliases;

CREATE OR REPLACE VIEW deploy.masters_change_log_v1 (change_id, master_id, changed_at, change_type, old_row, new_row) AS
  SELECT change_id, master_id, changed_at, change_type, old_row, new_row
    FROM deploy.masters_change_log;

CREATE OR REPLACE VIEW deploy.minions_v1 (minion_id, fqdn, group_id, master_id, pub_key, auto_reassign, created_at, updated_at, register_until) AS
  SELECT minion_id, fqdn, group_id, master_id, pub_key, auto_reassign, created_at, updated_at, register_until
    FROM deploy.minions;

CREATE OR REPLACE VIEW deploy.minions_change_log_v1 (change_id, minion_id, changed_at, change_type, old_row, new_row) AS
  SELECT change_id, minion_id, changed_at, change_type, old_row, new_row
    FROM deploy.minions_change_log;

CREATE OR REPLACE VIEW deploy.shipments_v1 (shipment_id, status, batch_size, stop_on_error_count, other_count, done_count, errors_count, total_count, created_at, updated_at, timeout) AS
  SELECT shipment_id, status, batch_size, stop_on_error_count, other_count, done_count, errors_count, total_count, created_at, updated_at, timeout
    FROM deploy.shipments;

CREATE OR REPLACE VIEW deploy.shipment_commands_v1 (shipment_command_id, shipment_id, type, arguments, timeout) AS
  SELECT shipment_command_id, shipment_id, type, arguments, timeout
    FROM deploy.shipment_commands;

CREATE OR REPLACE VIEW deploy.commands_v1 (command_id, minion_id, shipment_command_id, status, retries, created_at, updated_at) AS
  SELECT command_id, minion_id, shipment_command_id, status, retries, created_at, updated_at
    FROM deploy.commands;

CREATE OR REPLACE VIEW deploy.jobs_v1 (job_id, ext_job_id, command_id, status, created_at, updated_at) AS
  SELECT job_id, ext_job_id, command_id, status, created_at, updated_at
    FROM deploy.jobs;

CREATE OR REPLACE VIEW deploy.job_results_v1 (job_result_id, ext_job_id, fqdn, order_id, status, result, recorded_at) AS
  SELECT job_result_id, ext_job_id, fqdn, order_id, status, result, recorded_at
    FROM deploy.job_results;
