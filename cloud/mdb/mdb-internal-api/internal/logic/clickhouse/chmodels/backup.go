package chmodels

import (
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/valid"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const BackupPathPrefix = "ch_backup/%s/"

var BackupPathDelimiter = "/"

var BackupMetaFiles = []string{
	"backup_light_struct.json",
	"backup_struct.json",
}

type BackupLabels struct {
	Name      string `json:"name"`
	ShardName string `json:"shard_name"`
}

type S3BackupMeta struct {
	Meta struct {
		DateFmt   string       `json:"date_fmt"`
		StartTime string       `json:"start_time"`
		EndTime   string       `json:"end_time"`
		Labels    BackupLabels `json:"labels"`
		State     string       `json:"state"`
		Size      int64        `json:"bytes"`
	}
}

func cDateNotationToGoDateLayout(cLayout string) string {
	replacer := strings.NewReplacer(
		"%a", "Mon",
		"%A", "Monday",
		"%b", "Jan",
		"%B", "January",
		"%c", "Mon Jan 02 15:04:05 2006",
		"%d", "02",
		"%D", "01/02/06",
		"%e", "2",
		"%F", "2006-01-02",
		"%h", "Jan",
		"%H", "15",
		"%I", "3",
		"%m", "01",
		"%M", "04",
		"%n", "\n",
		"%p", "PM",
		"%r", "03:04:05 PM",
		"%R", "15:04",
		"%S", "05",
		"%t", "\t",
		"%T", "15:04:05",
		"%x", "01/02/06",
		"%X", "15:04:05",
		"%y", "06",
		"%Y", "2006",
		"%z", "-0700",
		"%Z", "MST",
	)
	return replacer.Replace(cLayout)
}

func backupTime(t, dateFormat string) (time.Time, error) {
	return time.Parse(cDateNotationToGoDateLayout(dateFormat), t)
}

func BackupFromBackupMeta(backupMeta S3BackupMeta) (bmodels.Backup, error) {
	if backupMeta.Meta.State != "created" {
		return bmodels.Backup{}, xerrors.Errorf("backup is not created")
	}
	if len(backupMeta.Meta.DateFmt) == 0 {
		return bmodels.Backup{}, xerrors.Errorf("unexpected backup meta")
	}

	var err error
	var createdAt, startedAt time.Time
	startedAt, err = backupTime(backupMeta.Meta.StartTime, backupMeta.Meta.DateFmt)
	if len(backupMeta.Meta.EndTime) != 0 {
		createdAt, err = backupTime(backupMeta.Meta.EndTime, backupMeta.Meta.DateFmt)
	}
	if err != nil {
		return bmodels.Backup{}, err
	}

	var shardNames []string
	if len(backupMeta.Meta.Labels.ShardName) != 0 {
		shardNames = []string{backupMeta.Meta.Labels.ShardName}
	}

	backup := bmodels.Backup{
		SourceShardNames: shardNames,
		StartedAt:        optional.NewTime(startedAt),
		CreatedAt:        createdAt,
		Size:             backupMeta.Meta.Size,
		Metadata:         backupMeta.Meta.Labels,
	}

	return backup, nil
}

func mustBackupNameValidator() *valid.StringComposedValidator {
	b, err := valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: "^[a-zA-Z0-9_:-]+$",
			Msg:     "backup name %q has invalid symbols",
		},
		&valid.StringLength{
			Min:         1,
			Max:         64,
			TooShortMsg: "backup name %q is too short",
			TooLongMsg:  "backup name %q is too long",
		},
	)
	if err != nil {
		panic(err)
	}

	return b
}

var backupNameValidator = mustBackupNameValidator()

func ValidateBackupName(name string) error {
	return backupNameValidator.ValidateString(name)
}
