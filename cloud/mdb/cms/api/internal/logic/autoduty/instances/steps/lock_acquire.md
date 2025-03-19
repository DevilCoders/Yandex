# lock acquire

Acquire lock in mlock with 
* fqdn "A" 
* and all legs in subshard/subcluster that "A" resides in.

LockID is generated like `fmt.Sprintf("cms-instances-%s", operationID)`
