CREATE INDEX i_commands_minion_id_last_dispatch_attempt_at
    ON deploy.commands (minion_id, last_dispatch_attempt_at)
 WHERE status = 'AVAILABLE'::deploy.command_status;
