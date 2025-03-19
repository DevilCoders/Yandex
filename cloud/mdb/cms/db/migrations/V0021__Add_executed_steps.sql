ALTER TABLE cms.instance_operations
    ADD COLUMN executed_step_names text[] DEFAULT '{}' NOT NULL;
