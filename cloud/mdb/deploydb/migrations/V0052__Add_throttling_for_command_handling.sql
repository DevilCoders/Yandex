CREATE INDEX i_commands_minion_id_status_running
    ON deploy.commands (minion_id)
WHERE status = 'RUNNING'::deploy.command_status;
