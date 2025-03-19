ALTER TABLE deploy.shipments ALTER COLUMN created_at DROP DEFAULT;
ALTER TABLE deploy.shipments ALTER COLUMN updated_at DROP DEFAULT;

ALTER TABLE deploy.commands ALTER COLUMN created_at DROP DEFAULT;
ALTER TABLE deploy.commands ALTER COLUMN updated_at DROP DEFAULT;

ALTER TABLE deploy.jobs ALTER COLUMN created_at DROP DEFAULT;
ALTER TABLE deploy.jobs ALTER COLUMN updated_at DROP DEFAULT;

ALTER TABLE deploy.job_results ALTER COLUMN recorded_at DROP DEFAULT;
