ALTER TABLE cs_propositions ADD COLUMN IF NOT EXISTS proposition_policy jsonb;
ALTER TABLE cs_propositions_history ADD COLUMN IF NOT EXISTS proposition_policy jsonb;
