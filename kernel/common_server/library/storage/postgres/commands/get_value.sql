SELECT b.value
FROM $ADDITIONAL_TABLE_NAME$ AS a
JOIN $TABLE_NAME$ AS b
ON a.key = b.key AND a.last_version = b.version
WHERE b.key = '$KEY$';
