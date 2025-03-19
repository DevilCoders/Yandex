-- check duplicates row for (billing_account_id, date)

$table = {{input1->table_quote()}};

$query = (
  SELECT
    billing_account_id,
    `date`,
    COUNT(*) as cnt
  FROM $table
  GROUP BY billing_account_id, `date`
  HAVING COUNT(*) > 1
  LIMIT 10000
);

DISCARD SELECT
  Ensure(
    0, -- will discard result anyway
    COUNT(*) = 0, -- if $query not empty than error
    "Result has " || CAST(COUNT(*) AS String) || " rows. For example: " || ToBytes(Yson::SerializePretty(Yson::From(SOME(TableRow()))))
  )
FROM $query
