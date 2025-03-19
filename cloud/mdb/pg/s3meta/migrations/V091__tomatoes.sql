CREATE TABLE s3.tomatoes
-- buckets to priority move by smart mover
(
    bid         uuid NOT NULL PRIMARY KEY,
    priority    int  NOT NULL DEFAULT 1,
    comment     text COLLATE "C"
);
