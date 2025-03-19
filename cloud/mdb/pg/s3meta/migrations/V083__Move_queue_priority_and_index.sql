ALTER TABLE s3.chunks_move_queue ADD COLUMN priority int DEFAULT 0 NOT NULL;
ALTER TABLE s3.chunks_move_queue ADD CONSTRAINT chunks_move_queue_bid_cid_uidx UNIQUE (bid, cid);
