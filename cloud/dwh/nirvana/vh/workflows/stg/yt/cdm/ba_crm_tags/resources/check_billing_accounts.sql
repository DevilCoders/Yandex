-- check all billing accounts used

PRAGMA DisableSimpleColumns;

$table = {{input1->table_quote()}};
$billing_accounts_table = {{param["billing_accounts_table"]->quote()}};

$now_ts = CurrentUtcTimestamp() - INTERVAL('PT18H');

$query = (
  SELECT
    t.billing_account_id,
    t.`date`,
    ba.billing_account_id,
    CAST(ba.created_at as String) AS created_at,
  FROM $table as t
  EXCLUSION JOIN $billing_accounts_table as ba USING(billing_account_id)
  WHERE ba.created_at < $now_ts
  LIMIT 10000
);

DISCARD SELECT
  Ensure(
    0, -- will discard result anyway
    COUNT(*) = 0, -- if $query not empty than error
    "Result has " || CAST(COUNT(*) AS String) || " rows. For example: " || ToBytes(Yson::SerializePretty(Yson::From(SOME(TableRow()))))
  )
FROM $query
