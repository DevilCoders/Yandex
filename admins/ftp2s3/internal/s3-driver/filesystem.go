package s3driver

import (
	"os"

	"bytes"
	"path/filepath"
	"strings"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/service/s3"
)

const (
	keepfile = ".keep"

	delimeter = string(os.PathSeparator)
)

// MakeDirectory emulate directory on S3 by creating '.keep' file in it
func (driver *S3Driver) MakeDirectory(directory string) error {
	_, err := driver.Service.PutObject(&s3.PutObjectInput{
		Bucket: Flags.Bucket,
		Key:    aws.String(filepath.Join(driver.Prefix, directory, keepfile)),
		Body:   bytes.NewReader(make([]byte, 0)),
	})
	return err
}

// ListFiles list s3 Object in the path
func (driver *S3Driver) ListFiles(directory string) (files []os.FileInfo, err error) {
	defer func() {
		names := make([]string, len(files))
		for idx, f := range files {
			names[idx] = f.Name()
		}
		log.Infof("ListFiles: %s: %q, err=%v", filepath.Join(driver.Prefix, directory), names, err)
	}()
	if strings.HasPrefix(filepath.Base(directory), "-") {
		// it's possible not a path, but a flag (like "-la")
		// check for dir existance
		// ref: https://github.com/fclairamb/ftpserver/commit/21db09da498a5d7d8a135189d9b2037dbde6b780#commitcomment-36936717
		info, err := driver.GetFileInfo(filepath.Join(directory, keepfile))
		if err != nil {
			return nil, err
		}
		// keepfile should not be a directory
		if info.IsDir() {
			directory = filepath.Dir(directory)
		}
	}
	// explicitly join filepath.Clean()'ed path with prefix to prevent prefix bypass
	strippedPath := strings.TrimPrefix(
		filepath.Join(
			driver.Prefix,
			directory,
		),
		delimeter,
	)
	if strippedPath != "" {
		strippedPath = strippedPath + delimeter
	}

	objects, err := driver.Service.ListObjectsV2(&s3.ListObjectsV2Input{
		Bucket:    Flags.Bucket,
		Prefix:    &strippedPath,
		Delimiter: aws.String(delimeter),
	})
	if err != nil {
		return nil, err
	}

	fileInfoMap := make(map[string]*S3FileInfo)
	for _, prefix := range objects.CommonPrefixes {
		fileInfo := fromS3Prefix(prefix.Prefix)
		fileInfoMap[fileInfo.Name()] = fileInfo
	}
	for _, obj := range objects.Contents {
		fileInfo := fromS3Object(obj)
		if fileInfo.Name() == keepfile && fileInfo.Size() == 0 {
			continue
		}
		fileInfoMap[fileInfo.Name()] = fileInfo
	}

	files = make([]os.FileInfo, 0, len(fileInfoMap))
	for _, fileInfo := range fileInfoMap {
		files = append(files, fileInfo)
	}

	return files, nil
}

// GetFileInfo convert s3 HeadObject to FileInfo
func (driver *S3Driver) GetFileInfo(path string) (os.FileInfo, error) {
	if path == delimeter {
		return fromS3Prefix(&path), nil
	}
	objectHead, err := driver.Service.HeadObject(&s3.HeadObjectInput{
		Bucket: Flags.Bucket,
		Key:    aws.String(filepath.Join(driver.Prefix, path)),
	})
	if err != nil {
		if err, ok := err.(awserr.Error); ok && err.Code() == "NotFound" {
			return fromS3Prefix(&path), nil
		}
		return nil, err
	}
	return fromS3HeadOutput(&path, objectHead), nil
}
