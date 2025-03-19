package ssmodels

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

var DatabaseNameBlackList []string = []string{
	"master",
	"msdb",
	"model",
	"tempdb",
}

var databaseNameValidator = models.MustDatabaseNameValidator(models.DefaultDatabaseNamePattern, DatabaseNameBlackList)

var s3BucketNameValidator = models.MustS3BucketNameValidator(models.DefaultS3BucketNamePattern, nil)

var s3PathValidator = models.MustS3PathValidator(models.DefaultS3PathPartPattern, models.DefaultS3PathMaxLen)

var s3FilesValidator = models.MustS3FileNameValidator("file", models.DefaultS3FilenamePattern, 1, models.DefaultS3PathPartMaxLen)

var s3PrefixValidator = models.MustS3FileNameValidator("prefix", models.DefaultS3FilenamePattern, 1, models.DefaultS3PathPartMaxLen)

type Database struct {
	ClusterID string
	Name      string
}

type DatabaseSpec struct {
	Name string
}

type RestoreDatabaseSpec struct {
	DatabaseSpec
	FromDatabase string
	BackupID     string
	Time         time.Time
}

func (ds DatabaseSpec) Validate() error {
	if err := databaseNameValidator.ValidateString(ds.Name); err != nil {
		return err
	}
	return nil
}

type ImportDatabaseBackupSpec struct {
	DatabaseSpec
	S3Bucket string
	S3Path   string
	Files    []string
}

func (is ImportDatabaseBackupSpec) Validate() error {
	if err := is.DatabaseSpec.Validate(); err != nil {
		return err
	}
	if err := s3BucketNameValidator.ValidateString(is.S3Bucket); err != nil {
		return err
	}
	if err := s3PathValidator.ValidateString(is.S3Path); err != nil {
		return err
	}
	for _, fn := range is.Files {
		if err := s3FilesValidator.ValidateString(fn); err != nil {
			return err
		}
	}
	return nil
}

type ExportDatabaseBackupSpec struct {
	DatabaseSpec
	S3Bucket string
	S3Path   string
	Prefix   string
}

func (es ExportDatabaseBackupSpec) Validate() error {
	if err := es.DatabaseSpec.Validate(); err != nil {
		return err
	}
	if err := s3BucketNameValidator.ValidateString(es.S3Bucket); err != nil {
		return err
	}
	if err := s3PathValidator.ValidateString(es.S3Path); err != nil {
		return err
	}
	if err := s3PrefixValidator.ValidateString(es.Prefix); err != nil {
		return err
	}
	return nil
}
