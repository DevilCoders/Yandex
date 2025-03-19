package s3driver

import "flag"

// Flags are flags for s3driver
var Flags = struct {
	Endpoint *string
	Bucket   *string
	TempDir  *string
}{
	Endpoint: flag.String("s3-endpoint", "s3.mdst.yandex.net", "Endpoint to use S3"),
	Bucket:   flag.String("s3-bucket", "ftpd", "Use AWS S3 with this bucket as storage backend"),
	TempDir:  flag.String("temp-dir", "", "Directory for temporary files"),
}
