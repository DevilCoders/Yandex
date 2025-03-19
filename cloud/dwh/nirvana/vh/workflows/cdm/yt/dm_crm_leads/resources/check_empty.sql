PRAGMA DisableSimpleColumns;

$table = {{input1->table_quote()}};


$query = (
  SELECT
    COUNT(*) AS rows_count
  FROM $table AS table
  
);

DISCARD SELECT
  Ensure(
    0, -- will discard result anyway
    rows_count > 0, -- if $query is empty than error
    "Result has " || CAST(rows_count AS String) || " rows."
  )
FROM $query
