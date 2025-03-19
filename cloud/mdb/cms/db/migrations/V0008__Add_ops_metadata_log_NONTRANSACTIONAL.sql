ALTER TABLE cms.decisions ADD COLUMN ops_metadata_log  jsonb NULL;
UPDATE cms.decisions SET ops_metadata_log = '{}';
ALTER TABLE cms.decisions ALTER COLUMN ops_metadata_log SET NOT NULL;
