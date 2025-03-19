INSERT INTO dbaas.search_queue (doc)
SELECT doc FROM unnest(cast(%(docs)s AS jsonb[])) AS doc
