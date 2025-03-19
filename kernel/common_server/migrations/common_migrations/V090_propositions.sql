ALTER TABLE cs_propositions ALTER COLUMN data SET DATA TYPE bytea USING data::bytea;
ALTER TABLE cs_propositions_history ALTER COLUMN data SET DATA TYPE bytea USING data::bytea;
