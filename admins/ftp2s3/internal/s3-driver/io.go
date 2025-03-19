package s3driver

import (
	"bytes"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/service/s3"
	"github.com/fclairamb/ftpserver/server"

	"a.yandex-team.ru/admins/ftp2s3/internal/stats"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	partSize = 64 * 1024 * 1024

	uploadPartCopyMinSize = 5 * 1024 * 1024
	uploadPartCopyMaxSize = 5 * 1024 * 1024 * 1024
)

// ----------------------------------------------------------------------------------

// S3FileSource read-only proxy of the corresponding s3 file
type S3FileSource struct {
	driver *S3Driver

	destPath string
	reader   io.ReadCloser
	offset   int64

	stats *stats.S3ReadStats
}

// S3FileDestination write-only proxy of the corresponding s3 object
type S3FileDestination struct {
	driver *S3Driver

	destPath       string
	uploadID       string
	partNumber     int64
	completedParts []*s3.CompletedPart
	tempFile       *os.File
	tempFileSize   int64

	needResume bool
	offset     int64

	stats *stats.S3WriteStats
}

// ----------------------------------------------------------------------------------

// NewS3FileSource create read-only proxy for the s3 object
func NewS3FileSource(driver *S3Driver, path string, m metrics.Registry) (*S3FileSource, error) {
	stats := stats.GetS3ReadStats(m)
	stats.Open.Inc()
	return &S3FileSource{
		driver:   driver,
		destPath: filepath.Join(driver.Prefix, path),
		stats:    stats,
	}, nil
}

func (f *S3FileSource) Read(buffer []byte) (int, error) {
	f.stats.Read.Inc()
	defer f.stats.ReadDuration(time.Now())

	if f.reader == nil {
		var err error
		f.reader, err = f.newReader()
		if err != nil {
			return 0, err
		}
	}
	return f.reader.Read(buffer)
}

// RangeRead read up to len(buffer) bytes of an object from specified offset
func (f *S3FileSource) RangeRead(buffer []byte, offset int64) (int, error) {
	f.stats.Read.Inc()
	defer f.stats.ReadDuration(time.Now())

	rangeReader, err := f.newRangeReader(offset, offset+int64(len(buffer)))
	if err != nil {
		return 0, err
	}
	defer rangeReader.Close()
	var n int
	tmp, err := ioutil.ReadAll(rangeReader)
	n = copy(buffer, tmp)
	if rangeReader != nil {
		_ = rangeReader.Close()
	}
	return n, err
}

func (f *S3FileSource) newReader() (io.ReadCloser, error) {
	return f.newRangeReader(f.offset, -1)
}

func (f *S3FileSource) newRangeReader(from, to int64) (io.ReadCloser, error) {
	getObjectTask := &s3.GetObjectInput{
		Bucket: Flags.Bucket,
		Key:    &f.destPath,
	}

	bytesRange := fmt.Sprintf("bytes=%d", from)
	if from >= 0 {
		bytesRange += "-"
		if to > 0 {
			bytesRange += fmt.Sprintf("%d", to)
		}
	}
	if from != 0 || to > 0 {
		getObjectTask = getObjectTask.SetRange(bytesRange)
	}

	result, err := f.driver.Service.GetObject(getObjectTask)
	if err != nil {
		return nil, err
	}
	return result.Body, nil
}

func (f *S3FileSource) Write([]byte) (int, error) {
	f.stats.Write.Inc()
	return 0, xerrors.New("S3File.Write: O_RDONLY")
}

// Seek operation are posible only before first read
// Seeking to current position are always acceptable
func (f *S3FileSource) Seek(offset int64, whence int) (int64, error) {
	f.stats.Seek.Inc()
	var abs int64
	switch whence {
	case io.SeekStart:
		abs = offset
	case io.SeekCurrent:
		abs = f.offset + offset
	default:
		return 0, xerrors.New("S3File.Seek: invalid whence")
	}
	if abs < 0 {
		return 0, xerrors.New("S3File.Seek: negative position")
	}
	if f.offset != abs && f.reader != nil {
		return 0, xerrors.New("S3File.Seek: reader is already initialized")
	}
	f.offset = abs
	return abs, nil
}

// Close ...
func (f *S3FileSource) Close() error {
	f.stats.Close.Inc()
	if f.reader != nil {
		return f.reader.Close()
	}
	return nil
}

// ----------------------------------------------------------------------------------

// NewS3FileDestination create write-only proxy for the s3 object
func NewS3FileDestination(driver *S3Driver, path string, flag int, m metrics.Registry) (f *S3FileDestination, err error) {
	stats := stats.GetS3WriteStats(m)
	stats.Open.Inc()
	f = &S3FileDestination{
		driver:     driver,
		destPath:   filepath.Join(driver.Prefix, path),
		needResume: false,
		offset:     0,
		stats:      stats,
	}

	createUploadOutput, err := driver.Service.CreateMultipartUpload(&s3.CreateMultipartUploadInput{
		Bucket: Flags.Bucket,
		Key:    &f.destPath,
	})
	if err != nil {
		return
	}

	f.uploadID = *createUploadOutput.UploadId
	f.partNumber = int64(1)

	if flag&os.O_APPEND != 0 {
		f.needResume = true
	}

	f.tempFile, err = ioutil.TempFile(*Flags.TempDir, "ftpd-tmp-")
	if err != nil {
		return
	}
	_ = os.Remove(f.tempFile.Name())
	f.tempFileSize = 0

	return
}

func (f *S3FileDestination) Write(buffer []byte) (n int, err error) {
	f.stats.Write.Inc()
	defer f.stats.WriteDuration(time.Now())

	if f.needResume {
		f.needResume = false
		err = f.resumeUpload()
		if err != nil {
			return
		}
	}
	reader := bytes.NewReader(buffer)
	for {
		var bytesRead int64
		bytesRead, err = io.Copy(f.tempFile, io.LimitReader(reader, partSize-f.tempFileSize))
		if err == io.ErrUnexpectedEOF {
			err = nil
		}
		if err != nil {
			return
		}
		if bytesRead == 0 {
			return
		}
		n += int(bytesRead)
		f.tempFileSize += bytesRead

		if f.tempFileSize >= partSize {
			err = f.uploadPart()
			if err != nil {
				return
			}
		}
	}
}

// Seek operation are posible only before first write
// Seeking to current position are always acceptable
func (f *S3FileDestination) Seek(offset int64, whence int) (int64, error) {
	f.stats.Seek.Inc()
	switch whence {
	case io.SeekStart:
		if f.offset == offset {
			break
		}
		if f.partNumber > 1 {
			return 0, xerrors.New("S3File.Seek: writer is already initialized")
		}
		if offset < 0 {
			return 0, xerrors.New("S3File.Seek: negative position")
		}

		f.offset = offset
		if f.offset > 0 {
			f.needResume = true
		}
	default:
		return 0, xerrors.New("S3File.Seek: invalid whence")
	}

	return f.offset, nil
}

func (f *S3FileDestination) Read([]byte) (int, error) {
	f.stats.Read.Inc()
	return 0, xerrors.New("S3File.Read: O_WRONLY")
}

// Close underlying tmp files and complete s3 multipart uploads
func (f *S3FileDestination) Close() error {
	f.stats.Close.Inc()
	if f.tempFileSize > 0 {
		if err := f.uploadPart(); err != nil {
			return err
		}
	}
	_ = f.tempFile.Close()

	if f.partNumber > 1 {
		return f.completeUpload()
	}
	f.abortUpload()
	return nil
}

func (f *S3FileDestination) uploadPart() error {
	f.stats.UploadPart.Inc()
	defer f.stats.UploadDuration(time.Now())

	_, err := f.tempFile.Seek(0, 0)
	if err != nil {
		return err
	}

	part, err := f.driver.Service.UploadPart(&s3.UploadPartInput{
		Bucket:     Flags.Bucket,
		Key:        &f.destPath,
		PartNumber: &f.partNumber,
		UploadId:   &f.uploadID,
		Body:       f.tempFile,
	})
	if err != nil {
		return err
	}

	f.completedParts = append(f.completedParts, &s3.CompletedPart{
		PartNumber: aws.Int64(f.partNumber),
		ETag:       part.ETag,
	})

	f.partNumber++

	_, err = f.tempFile.Seek(0, 0)
	if err != nil {
		return err
	}

	err = f.tempFile.Truncate(0)
	if err != nil {
		return err
	}

	f.tempFileSize = 0

	return nil
}

func (f *S3FileDestination) resumeUpload() error {
	f.stats.ResumeUpload.Inc()
	objectHead, err := f.driver.Service.HeadObject(&s3.HeadObjectInput{
		Bucket: Flags.Bucket,
		Key:    &f.destPath,
	})
	if err != nil {
		if err, ok := err.(awserr.Error); ok && err.Code() == "NotFound" {
			// nothing to resume, just ignore
			return nil
		}
		return err
	}

	if f.offset == 0 || *objectHead.ContentLength < f.offset {
		f.offset = *objectHead.ContentLength
	}

	if f.offset < uploadPartCopyMinSize {
		getObjectInput := &s3.GetObjectInput{
			Bucket: Flags.Bucket,
			Key:    &f.destPath,
			Range:  aws.String(fmt.Sprintf("bytes=0-%d", f.offset-1)),
		}
		object, err := f.driver.Service.GetObject(getObjectInput)
		if err != nil {
			return err
		}
		_, err = io.Copy(f, object.Body)
		if err != nil {
			return err
		}
	} else if f.offset > uploadPartCopyMaxSize {
		// FIXME: UploadPartCopyInput will fail if the source file is greater than 5 GB.
		// Such files should be splitted to several Content-Range's.
		// See https://st.yandex-team.ru/CMS-204
		return xerrors.New("the upload can not be continued. Please remove the file and try again.")
	} else {
		uploadPartCopyInput := &s3.UploadPartCopyInput{
			Bucket:          Flags.Bucket,
			Key:             &f.destPath,
			UploadId:        &f.uploadID,
			PartNumber:      &f.partNumber,
			CopySource:      aws.String(*Flags.Bucket + f.destPath),
			CopySourceRange: aws.String(fmt.Sprintf("bytes=0-%d", f.offset-1)),
		}
		part, err := f.driver.Service.UploadPartCopy(uploadPartCopyInput)
		if err != nil {
			return err
		}
		f.completedParts = append(f.completedParts, &s3.CompletedPart{
			PartNumber: aws.Int64(f.partNumber),
			ETag:       part.CopyPartResult.ETag,
		})
		f.partNumber++
	}
	return nil
}

func (f *S3FileDestination) completeUpload() error {
	f.stats.CompleteUpload.Inc()
	_, err := f.driver.Service.CompleteMultipartUpload(&s3.CompleteMultipartUploadInput{
		Bucket:   Flags.Bucket,
		Key:      &f.destPath,
		UploadId: &f.uploadID,
		MultipartUpload: &s3.CompletedMultipartUpload{
			Parts: f.completedParts,
		},
	})

	return err
}

func (f *S3FileDestination) abortUpload() {
	f.stats.AbortUpload.Inc()
	_, _ = f.driver.Service.AbortMultipartUpload(&s3.AbortMultipartUploadInput{
		Bucket:   Flags.Bucket,
		Key:      &f.destPath,
		UploadId: &f.uploadID,
	})
}

// ----------------------------------------------------------------------------------

// OpenFile create s3 object reader/writer
func (driver *S3Driver) OpenFile(path string, flag int, m metrics.Registry) (server.FileStream, error) {
	switch flag {
	case os.O_RDONLY:
		return NewS3FileSource(driver, path, m.WithPrefix("read.s3"))
	case os.O_WRONLY, os.O_WRONLY | os.O_APPEND:
		return NewS3FileDestination(driver, path, flag, m.WithPrefix("write.s3"))
	default:
		return nil, xerrors.New("S3Driver.OpenFile: invalid flag")
	}
}

// ----------------------------------------------------------------------------------

// DeleteFile delete appropriate s3 object
func (driver *S3Driver) DeleteFile(path string) error {
	fileInfo, err := driver.GetFileInfo(path)
	if err != nil {
		return err
	}
	key := filepath.Join(driver.Prefix, path)
	if fileInfo.IsDir() {
		// TODO: Error 'Directory not empty'
		key = filepath.Join(key, keepfile)
	}
	_, err = driver.Service.DeleteObject(&s3.DeleteObjectInput{
		Bucket: Flags.Bucket,
		Key:    &key,
	})
	return err
}
