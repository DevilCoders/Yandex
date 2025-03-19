ALTER TYPE cms.decision_status ADD VALUE 'before-done' BEFORE 'escalated';
ALTER TYPE cms.decision_status ADD VALUE 'done' BEFORE 'escalated';
ALTER TABLE cms.requests ADD COLUMN came_back_at timestamp with time zone;
ALTER TABLE cms.requests ADD COLUMN done_at timestamp with time zone;
