SELECT * FROM (
  SELECT ErrorEventItem FROM Input
  WHERE ErrorEventItem IS NOT NULL
) FLATTEN COLUMNS;
