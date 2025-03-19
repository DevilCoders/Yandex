ALTER TABLE s3.object_parts VALIDATE CONSTRAINT check_data_1;

ALTER TABLE s3.object_parts DROP CONSTRAINT check_data;

ALTER TABLE s3.object_parts RENAME CONSTRAINT check_data_1 TO check_data;
