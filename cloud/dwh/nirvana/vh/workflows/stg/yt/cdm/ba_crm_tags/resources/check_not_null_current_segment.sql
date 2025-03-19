-- check not null current segment for crm accounts

$table = {{input1->table_quote()}};

$query = (
  SELECT
    billing_account_id,
    `date`,
    crm_account_id,
    crm_account_name,
    segment_current,
    segment,
  FROM $table
  WHERE crm_account_id IS NOT NULL AND segment_current IS NULL
  LIMIT 10000
);

DISCARD SELECT
  Ensure(
    0, -- will discard result anyway
    COUNT(*) = 0, -- if $query not empty than error
    "Result has " || CAST(COUNT(*) AS String) || " rows. For example: " || ToBytes(Yson::SerializePretty(Yson::From(SOME(TableRow()))))
  )
FROM $query
