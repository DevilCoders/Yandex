ALTER TABLE cms.requests ADD COLUMN scenario_info jsonb DEFAULT '{}'::jsonb NOT NULL;
