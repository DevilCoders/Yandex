package s3driver

import (
	"os"
	"syscall"

	"path/filepath"
	"time"

	"github.com/aws/aws-sdk-go/service/s3"
)

const (
	fakeFileMode os.FileMode = 0640
	fakeDirMode  os.FileMode = 0750 | os.ModeDir

	fakeDirSize = 4096
)

var (
	fakeDirMtime = time.Unix(946684800, 0)
)

type S3FileInfo struct {
	name         *string
	size         int64
	lastModified time.Time
	isDir        bool
}

func (f S3FileInfo) Name() string {
	return *f.name
}

func (f S3FileInfo) Size() int64 {
	return f.size
}

func (f S3FileInfo) Mode() os.FileMode {
	if f.IsDir() {
		return fakeDirMode
	}
	return fakeFileMode
}

func (f S3FileInfo) ModTime() time.Time {
	return f.lastModified
}

func (f S3FileInfo) IsDir() bool {
	return f.isDir
}

func (f S3FileInfo) Sys() interface{} {
	// fix filezilla directory listing MUSICBACKEND-3734#5e1b3b0e62659a23f57ff748
	return &syscall.Stat_t{}
}

// constructors

func fromS3HeadOutput(key *string, output *s3.HeadObjectOutput) *S3FileInfo {
	basename := filepath.Base(*key)
	return &S3FileInfo{
		name:         &basename,
		size:         *output.ContentLength,
		lastModified: *output.LastModified,
		isDir:        false,
	}
}

func fromS3Object(object *s3.Object) *S3FileInfo {
	basename := filepath.Base(*object.Key)
	return &S3FileInfo{
		name:         &basename,
		size:         *object.Size,
		lastModified: *object.LastModified,
		isDir:        false,
	}
}

func fromS3Prefix(prefix *string) *S3FileInfo {
	basename := filepath.Base(*prefix)
	return &S3FileInfo{
		name:         &basename,
		size:         fakeDirSize,
		lastModified: fakeDirMtime,
		isDir:        true,
	}
}
