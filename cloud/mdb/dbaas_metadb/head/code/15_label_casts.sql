
CREATE OR REPLACE FUNCTION code.labels_from_jsonb(
    ld jsonb
) RETURNS code.label[] AS $$
SELECT ARRAY(
    SELECT (key, value)::code.label
        FROM jsonb_each_text(ld)
);
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.labels_to_jsonb(
    ld code.label[]
) RETURNS jsonb AS $$
SELECT coalesce(js, '{}'::jsonb)
  FROM (
      SELECT jsonb_object_agg(key, value) js
        FROM unnest(ld)
  ) x;
$$ LANGUAGE SQL IMMUTABLE;

CREATE CAST (jsonb AS code.label[]) WITH FUNCTION code.labels_from_jsonb(jsonb);
CREATE CAST (code.label[] AS jsonb) WITH FUNCTION code.labels_to_jsonb(code.label[]);
