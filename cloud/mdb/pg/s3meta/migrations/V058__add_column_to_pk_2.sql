ALTER TABLE s3.chunks_counters DROP CONSTRAINT chunks_counters_pkey;
ALTER TABLE s3.chunks_counters ADD PRIMARY KEY USING INDEX pk_chunks_counters;
