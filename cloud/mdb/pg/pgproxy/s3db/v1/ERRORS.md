### List of S3-MDS Error Codes

All S3-MDS specific error codes have 'S3' error category.


#### Account Errors

All account error codes start with 'S3A' prefix.

* **S3A01** (AccessDenied)

   Anonymous access is forbidden for this operation.

* **S3A02** (NotSignedUp)

   Your account is not signed up for the S3-MDS service. You must sign up before
   you can use S3-MDS.

* **S3A03** (AccountProblem)

   There is a problem with your account that prevents the operation from
   completing successfully.

* **S3A04** (InvalidAccessKeyId)

   The specified access key ID does not exist in out records.

* **S3A05** (TooManyAccessKeys)

   The specified service user has reached maximum number of access keys.


#### Bucket Errors

All bucket error codes start with 'S3B' prefix.

* **S3B01** (NoSuchBucket)

   The specified bucket does not exist.

* **S3B02** (BucketAlreadyExists)

   The requested bucket name is not available.

* **S3B03** (AccessDenied)

   Access to the bucket is denied.

* **S3B04** (ServiceUnavailable)

   Access to the bucket is temporarily unavailable.

* **S3B05** (BucketAlreadyOwnedByYou)

   Your previous request to create the named bucket succeeded and you already
   own it.

* **S3B06** (BucketHasObjects)

   Couldn't remove non-empty bucket with objects.

* **S3B07** (BucketHasObjectParts)

   Couldn't remove non-empty bucket with incomplete multipart uploads.

#### Key Errors

All key (object) error codes start with 'S3K' prefix.

* **S3K01** (NoSuchKey)

   The specified key does not exist. The error is raised when the current version
   of the object is requested.

* **S3K02** (NoSuchVersion)

   The specified key's version does not exist. The error is raised when specific
   version of the object is requested.

* **S3K03** (EntityTooLarge)

   Proposed upload exceeds the maximum allowed object size. Currently single
   object size is limited by 5GB.

* **S3K04** (TransactionTooOld)

   Trying to add object version into versioning bucket which is older than current version.


#### Multipart Upload Errors

All multipart upload error codes start with 'S3M' prefix.

* **S3M01** (NoSuchUpload)

   The specified multipart upload does not exist. The multipart upload ID might
   be invalid, or the multipart upload might have been aborted or completed.

* **S3M02** (InvalidPart)

   One or more of the specified parts could not be found. The part might not
   have been uploaded, or the specified MD5 hash might not have matched the
   part's MD5 hash.

* **S3M03** (InvalidPartOrder)

   The list of parts was not in ascending order. Parts list must be specified in
   order by part number starting from 1.

* **S3M04** (EntityTooSmall)

   Proposed upload is smaller than the minimum allowed object part's size.
   Each part must be at least 5MB in size, except the last part.

* **S3M05** (EntityTooLarge)

   Proposed upload exceeds the maximum allowed object part's size. Currently part
   size is limited by 5GB.


#### Deletion Errors

All deletion errors codes start with 'S3D' prefix.

* **S3D01** (NoSuchDeletedObject)

   The specified deleted object does not exist.

#### Internal Errors

All internal error codes start with 'S3X' prefix.

* **S3X01** (NoSuchChunk)

   Chunk was not found on database. Maybe chunk was moved to another shard.
   This error must be retried by client.

* **S3X02** (ChunkCannotBeSplitted)

   Chunk can not be splitted. It can occurs when chunk hasn't enough objects.
