package backups

import (
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

type Backup struct {
	ID               string
	FolderID         string
	SourceClusterID  string
	Databases        []string
	CreatedAt        time.Time
	StartedAt        optional.Time
	SourceShardNames []string
	S3Path           string
	Size             int64
	Metadata         interface{} // db specific
}

type BackupStatus int

const (
	BackupStatusDeleting BackupStatus = iota
	BackupStatusDeleted
	BackupStatusDeleteError
	BackupStatusDone
	BackupStatusPlanned
	BackupStatusCreating
	BackupStatusCreateError
)

var (
	mapBackupStatusToString = map[BackupStatus]string{
		BackupStatusDeleting:    "DELETING",
		BackupStatusDeleted:     "DELETED",
		BackupStatusDeleteError: "DELETE-ERROR",
		BackupStatusDone:        "DONE",
		BackupStatusPlanned:     "PLANNED",
		BackupStatusCreating:    "CREATING",
		BackupStatusCreateError: "CREATE-ERROR",
	}
	NameToBackupStatusMapping = make(map[string]BackupStatus, len(mapBackupStatusToString))
)

type BackupInitiator int

const (
	BackupInitiatorSchedule BackupInitiator = iota
	BackupInitiatorUser
)

var (
	mapBackupInitiatorToString = map[BackupInitiator]string{
		BackupInitiatorSchedule: "SCHEDULE",
		BackupInitiatorUser:     "USER",
	}
	NameToBackupInitiatorMapping = make(map[string]BackupInitiator, len(mapBackupInitiatorToString))
)

type BackupMethod int

const (
	BackupMethodFull BackupMethod = iota
	BackupMethodIncremental
)

var (
	mapBackupMethodToString = map[BackupMethod]string{
		BackupMethodFull:        "FULL",
		BackupMethodIncremental: "INCREMENTAL",
	}
	NameToBackupMethodMapping = make(map[string]BackupMethod, len(mapBackupMethodToString))
	BackupMethodToNameMapping = make(map[BackupMethod]string, len(mapBackupMethodToString))
)

func init() {
	for status, str := range mapBackupStatusToString {
		NameToBackupStatusMapping[strings.ToLower(str)] = status
	}

	for initiator, str := range mapBackupInitiatorToString {
		NameToBackupInitiatorMapping[strings.ToLower(str)] = initiator
	}

	for method, str := range mapBackupMethodToString {
		NameToBackupMethodMapping[strings.ToLower(str)] = method
		BackupMethodToNameMapping[method] = strings.ToUpper(str)
	}
}

// Probably we need to merge Backup and ManagedBackup into one structure in future. (Possibly, when all db's will use backup service)
type ManagedBackup struct {
	ID           string
	ClusterID    string
	SubClusterID string
	ShardID      optional.String
	Status       BackupStatus
	Scheduled    optional.Time
	CreatedAt    time.Time
	DelayedUntil time.Time
	StartedAt    optional.Time
	FinishedAt   optional.Time
	UpdatedAt    optional.Time
	ShipmentID   optional.String
	Metadata     optional.String // TODO: Replace to some struct, probably
	Errors       optional.String // TODO: Replace by struct someday
	Initiator    BackupInitiator
	Method       BackupMethod
}

type BackupConverter interface {
	ManagedToRegular(backup ManagedBackup) (Backup, error)
}

func (b Backup) GlobalBackupID() string {
	return EncodeGlobalBackupID(b.SourceClusterID, b.ID)
}

func (b *Backup) ValidateRestoreTime(t time.Time) error {
	if b.StartedAt.Valid && b.StartedAt.Time.After(t) {
		return semerr.FailedPreconditionf(
			"unable to restore to %q using this backup, "+
				"cause it started at %q "+
				"(use older backup or increase \"time\")",
			t.UTC().Format(time.RFC3339), b.StartedAt.Time.UTC().Format(time.RFC3339))
	}
	if b.CreatedAt.After(t) {
		return semerr.FailedPreconditionf(
			"unable to restore to %q using this backup, "+
				"cause it finished at %q "+
				"(use older backup or increase \"time\")",
			t.UTC().Format(time.RFC3339), b.CreatedAt.UTC().Format(time.RFC3339))
	}
	return nil
}

type BackupSchedule struct {
	Sleep            int               `json:"sleep" yaml:"sleep"`
	Start            BackupWindowStart `json:"start" yaml:"start"`
	UseBackupService bool              `json:"use_backup_service" yaml:"use_backup_service"`
}

func GetBackupSchedule(def BackupSchedule, start OptionalBackupWindowStart) BackupSchedule {
	bs := def
	if start.Valid {
		bs.Start = start.Value
	}
	return bs
}

type BackupWindowStart struct {
	Hours   int `json:"hours"`
	Minutes int `json:"minutes"`
	Seconds int `json:"seconds"`
	Nanos   int `json:"nanos"`
}

func (w *BackupWindowStart) Validate() error {
	if w.Hours < 0 || w.Hours > 23 {
		return semerr.InvalidInputf("Backup window start hours should be in range from 0 to 23 instead of %d", w.Hours)
	}
	if w.Minutes < 0 || w.Minutes > 59 {
		return semerr.InvalidInputf("Backup window start minutes should be in range from 0 to 59 instead of %d", w.Minutes)
	}
	if w.Seconds < 0 || w.Seconds > 59 {
		return semerr.InvalidInputf("Backup window start seconds should be in range from 0 to 59 instead of %d", w.Seconds)
	}
	if w.Nanos < 0 || w.Nanos > 999999999 {
		return semerr.InvalidInputf("Backup window start nanos should be in range from 0 to 10^9-1 instead of %d", w.Nanos)
	}
	return nil
}

type OptionalBackupWindowStart struct {
	Value BackupWindowStart
	Valid bool
}

func (o *OptionalBackupWindowStart) Set(v BackupWindowStart) {
	o.Valid = true
	o.Value = v
}

func (o *OptionalBackupWindowStart) Get() (BackupWindowStart, error) {
	if !o.Valid {
		return BackupWindowStart{}, optional.ErrMissing
	}
	return o.Value, nil
}

func DecodeGlobalBackupID(globalBackupID string) (string, string, error) {
	parts := strings.Split(globalBackupID, ":")
	if len(parts) != 2 {
		return "", "", semerr.InvalidInputf("malformed backup id: %s", globalBackupID)
	}
	return parts[0], parts[1], nil
}

func EncodeGlobalBackupID(cid, backupID string) string {
	return cid + ":" + backupID
}

type BackupsPageToken struct {
	ClusterID string
	BackupID  string
	More      bool `json:"-"`
}

func (bpt BackupsPageToken) HasMore() bool {
	return bpt.More
}
