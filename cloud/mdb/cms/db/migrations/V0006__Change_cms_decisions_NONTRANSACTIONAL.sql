ALTER TYPE cms.decision_status ADD VALUE 'at-wall-e' BEFORE 'escalated';

CREATE TYPE cms.ad_resolution AS ENUM (
    'unknown',
    'approved',
    'rejected'
    );

ALTER TABLE cms.decisions ADD COLUMN ad_resolution cms.ad_resolution NULL;
UPDATE cms.decisions SET ad_resolution = 'unknown';
ALTER TABLE cms.decisions ALTER COLUMN ad_resolution SET NOT NULL;

ALTER TABLE cms.decisions ADD COLUMN mutations_log TEXT NULL;
UPDATE cms.decisions SET mutations_log = 'created in migration';

