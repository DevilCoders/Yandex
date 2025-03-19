ALTER TABLE s3.buckets ADD COLUMN
    updated_ts TIMESTAMPTZ NOT NULL DEFAULT current_timestamp;

CREATE OR REPLACE FUNCTION trigger_set_updated_ts()
RETURNS TRIGGER AS $$
BEGIN
  NEW.updated_ts = current_timestamp;
  RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER set_updated_ts
BEFORE UPDATE ON s3.buckets
FOR EACH ROW
EXECUTE FUNCTION trigger_set_updated_ts();
