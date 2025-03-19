ALTER TABLE deploy.jobs ADD COLUMN last_running_check_at timestamptz;
ALTER TABLE deploy.jobs ADD COLUMN running_checks_failed int;

UPDATE deploy.jobs
    SET last_running_check_at = updated_at,
        running_checks_failed = 0;

ALTER TABLE deploy.jobs ALTER COLUMN last_running_check_at SET NOT NULL;
ALTER TABLE deploy.jobs ALTER COLUMN running_checks_failed SET NOT NULL;
ALTER TABLE deploy.jobs ALTER COLUMN running_checks_failed SET DEFAULT 0;
