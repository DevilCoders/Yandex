ALTER TABLE cs_propositions ADD COLUMN IF NOT EXISTS proposition_status TEXT DEFAULT 'waiting' NOT NULL;
ALTER TABLE cs_propositions_history ADD COLUMN IF NOT EXISTS proposition_status TEXT DEFAULT 'waiting' NOT NULL;
