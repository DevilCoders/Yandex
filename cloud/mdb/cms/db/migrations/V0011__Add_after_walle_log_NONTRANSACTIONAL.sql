ALTER TABLE cms.decisions ADD COLUMN after_walle_log text NULL;
UPDATE cms.decisions SET after_walle_log = '';
ALTER TABLE cms.decisions ALTER COLUMN after_walle_log SET NOT NULL;
