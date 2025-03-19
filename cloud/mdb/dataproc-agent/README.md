## Dataproc agent

### s3logger design and implementation

s3logger implements log upload to Yandex S3 storage. It is used to save job driver's process' output (stdout & stderr).

s3logger design is motivated by the fact that s3 protocol does not support file append. 
That's why we split driver's output into fixed-size chunks. Chunk entity is represented by LogChunk class. 
Chunk's size is defined by ChunkSize config param. Changed chunks are uploaded to s3 periodically. 
Periodicity is given by SyncInterval config param. PipeProcessor reads data from io.Reader stream, 
splits data into chunks and syncs changed chunks to s3 within separate goroutine. 
Full chunks that are synced to s3 are deleted from memory. 
In order to mitigate the case when the queue of dirty (not synchronized) chunks grows 
(eg. when s3 is not available or driver generates output faster than it is uploaded to s3) 
memory cache size is limited by MaxPoolSize config param; 
currently this limit is applied individually to every output stream but the whole process.
