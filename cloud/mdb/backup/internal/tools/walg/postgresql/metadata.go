package postgresql

import (
	"encoding/json"
	"fmt"
	"strconv"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	sizeofInt32           = 4
	sizeofInt32bits       = sizeofInt32 * 8
	WalSegmentSize        = uint64(16 * 1024 * 1024)
	xLogSegmentsPerXLogID = 0x100000000 / WalSegmentSize // xlog_internal.h line 101
	WalSegmentNameSize    = 24

	PGBackupPrefix = "base_"
)

// Extended metadata should describe backup in more details, but be small enough to be downloaded often
// https://github.com/wal-g/wal-g/blob/8a211e8f6a92bc676703de15cc8c6a0b1b48be63/internal/backup_sentinel_dto.go#L67
type ExtendedMetadataDto struct {
	StartTime        time.Time `json:"start_time"`
	FinishTime       time.Time `json:"finish_time"`
	DatetimeFormat   string    `json:"date_fmt"`
	Hostname         string    `json:"hostname"`
	DataDir          string    `json:"data_dir"`
	PgVersion        int       `json:"pg_version"`
	StartLsn         uint64    `json:"start_lsn"`
	FinishLsn        uint64    `json:"finish_lsn"`
	IsPermanent      bool      `json:"is_permanent"`
	SystemIdentifier *uint64   `json:"system_identifier"`

	UncompressedSize int64 `json:"uncompressed_size"`
	CompressedSize   int64 `json:"compressed_size"`

	UserData UserDataMeta `json:"user_data,omitempty"`
}

func UnmarshalExtendedMetadataDto(raw []byte) (ExtendedMetadataDto, error) {
	var metadata ExtendedMetadataDto
	if err := json.Unmarshal(raw, &metadata); err != nil {
		return ExtendedMetadataDto{}, err
	}
	return metadata, nil
}

// BackupSentinelDto describes file structure of json sentinel
// https://github.com/wal-g/wal-g/blob/8a211e8f6a92bc676703de15cc8c6a0b1b48be63/internal/backup_sentinel_dto.go#L9
type BackupSentinelDto struct {
	BackupStartLSN    *uint64 `json:"LSN"`
	IncrementFrom     *string `json:"DeltaFrom,omitempty"`
	IncrementFullName *string `json:"DeltaFullName,omitempty"`
	IncrementCount    *int    `json:"DeltaCount,omitempty"`

	PgVersion        int     `json:"PgVersion"`
	BackupFinishLSN  *uint64 `json:"FinishLSN"`
	SystemIdentifier *uint64 `json:"SystemIdentifier,omitempty"`

	UncompressedSize int64 `json:"UncompressedSize"`
	CompressedSize   int64 `json:"CompressedSize"`

	UserData interface{} `json:"UserData,omitempty"`

	// these non-primitive fields are intentionally commented out
	// because we don't need them in this provider right now

	//Files       internal.BackupFileList `json:"Files"`
	//TarFileSets TarFileSets             `json:"TarFileSets"`
	//TablespaceSpec   *TablespaceSpec `json:"Spec"`
}

type PostgreSQLMetadata struct {
	BackupName       string                      `json:"name,omitempty"`
	BackupID         string                      `json:"id,omitempty"`
	StartTime        time.Time                   `json:"start_time,omitempty"`
	FinishTime       time.Time                   `json:"finish_time,omitempty"`
	DataSize         int64                       `json:"compressed_size,omitempty"`
	DatetimeFormat   string                      `json:"date_fmt"`
	RootPath         string                      `json:"root_path,omitempty"`
	RawMeta          json.RawMessage             `json:"meta,omitempty"`
	IsIncremental    bool                        `json:"is_incremental"`
	IncrementDetails *PostgreSQLIncrementDetails `json:"increment_details,omitempty"`
}

func (sentinel *BackupSentinelDto) GetPostgreSQLIncrementDetails() (
	isIncremental bool, details *PostgreSQLIncrementDetails, err error) {
	// If we have increment base, we must have all the rest properties.
	if sentinel.IncrementFrom != nil {
		if sentinel.IncrementFullName == nil || sentinel.IncrementCount == nil {
			return false, nil, xerrors.Errorf("Inconsistent BackupSentinelDto")
		}
		return true, &PostgreSQLIncrementDetails{
			IncrementFrom:     *sentinel.IncrementFrom,
			IncrementFullName: *sentinel.IncrementFullName,
			IncrementCount:    *sentinel.IncrementCount,
		}, nil
	}
	return false, nil, nil
}
func NewPostgreSQLMetadata(backupName, rootPath string, metadata ExtendedMetadataDto, rawMeta []byte,
	sentinel BackupSentinelDto) (PostgreSQLMetadata, error) {
	isIncremental, incrementDetails, err := sentinel.GetPostgreSQLIncrementDetails()
	if err != nil {
		return PostgreSQLMetadata{}, err
	}

	return PostgreSQLMetadata{
		BackupName:       backupName,
		BackupID:         metadata.UserData.BackupID,
		StartTime:        metadata.StartTime,
		FinishTime:       metadata.FinishTime,
		DatetimeFormat:   metadata.DatetimeFormat,
		DataSize:         metadata.CompressedSize,
		RawMeta:          rawMeta,
		RootPath:         rootPath,
		IsIncremental:    isIncremental,
		IncrementDetails: incrementDetails,
	}, nil
}

type PostgreSQLIncrementDetails struct {
	IncrementFrom     string `json:"delta_from"`
	IncrementFullName string `json:"delta_full_name"`
	IncrementCount    int    `json:"delta_count"`
}

var _ metadb.BackupMetadata = PostgreSQLMetadata{}

func UnmarshalPostgreSQLMetadata(raw []byte) (PostgreSQLMetadata, error) {
	var meta PostgreSQLMetadata
	if err := json.Unmarshal(raw, &meta); err != nil {
		return PostgreSQLMetadata{}, err
	}
	return meta, nil
}

func (meta PostgreSQLMetadata) ID() string {
	return meta.BackupID
}

func (meta PostgreSQLMetadata) Marshal() ([]byte, error) {
	return json.Marshal(meta)
}

func (meta PostgreSQLMetadata) String() string {
	raw, err := meta.Marshal()
	if err != nil {
		return fmt.Sprintf("can not marshal postgresql metadata with error %v: %+v", err, raw)
	}
	return string(raw)
}

type UserDataMeta struct {
	BackupID string `json:"backup_id"`
}

func CommonPrefixesFromTimelineLsn(timelines []uint32, beginLSN, endLSN uint64) []string {
	beginSegPart := LogSegWalPartFromLSN(beginLSN)
	endSegPart := LogSegWalPartFromLSN(endLSN)
	i := 0
	for i = range beginSegPart {
		if beginSegPart[i] != endSegPart[i] {
			break
		}
	}
	commonSegPart := beginSegPart[:i]
	var prefixes []string
	for _, t := range timelines {
		prefixes = append(prefixes, fmt.Sprintf("%08X%s", t, commonSegPart))
	}
	return prefixes
}

func LogSegWalPartFromLSN(lsn uint64) string {
	logSegNo := LogSegNoFromLSN(lsn)
	return fmt.Sprintf("%08X%08X", logSegNo/xLogSegmentsPerXLogID, logSegNo%xLogSegmentsPerXLogID)
}

func LogSegNoFromLSN(lsn uint64) uint64 {
	return lsn / WalSegmentSize
}

// ParseWALFilename is taken from wal-g repo
// https://github.com/wal-g/wal-g/blob/62904211c17da69887058eac785de93688bc2b20/internal/databases/postgres/timeline.go#L103
func ParseWALFilename(name string) (timelineID uint32, logSegNo uint64, err error) {
	if len(name) != 24 {
		return 0, 0, xerrors.Errorf("incorrect WalSegment name: %s", name)
	}
	timelineID64, err := strconv.ParseUint(name[0:8], 0x10, sizeofInt32bits)
	if err != nil {
		return 0, 0, xerrors.Errorf("can not parse logSegNoHi in WalSegment: %s", name)
	}

	timelineID = uint32(timelineID64)
	logSegNoHi, err := strconv.ParseUint(name[8:16], 0x10, sizeofInt32bits)
	if err != nil {
		return 0, 0, xerrors.Errorf("can not parse logSegNoHi in WalSegment: %s", name)
	}
	logSegNoLo, err := strconv.ParseUint(name[16:24], 0x10, sizeofInt32bits)
	if err != nil {
		return 0, 0, xerrors.Errorf("can not parse logSegNoLo in WalSegment: %s", name)
	}
	if logSegNoLo >= xLogSegmentsPerXLogID {
		return 0, 0, xerrors.Errorf("incorrect logSegNoLo in WalSegment: %s", name)
	}

	logSegNo = logSegNoHi*xLogSegmentsPerXLogID + logSegNoLo
	return
}

func ParseTimelineFromBackupName(backupName string) (uint32, error) {
	if len(backupName) == 0 {
		return 0, xerrors.Errorf("incorrect backup name: %s", backupName)
	}
	prefixLength := len(PGBackupPrefix)
	return ParseTimelineFromString(backupName[prefixLength : prefixLength+8])
}

func ParseTimelineFromString(timelineString string) (uint32, error) {
	timelineID64, err := strconv.ParseUint(timelineString, 0x10, sizeofInt32bits)
	if err != nil {
		return 0, err
	}
	return uint32(timelineID64), nil
}

type WalSegment struct {
	Name       string
	TimelineID uint32
	LogSegNo   uint64
	StartLSN   uint64
	Size       int64
}

func NewWalSegmentFromFilename(name string, size int64) (WalSegment, error) {
	timelineID, logSegNo, err := ParseWALFilename(name)
	if err != nil {
		return WalSegment{}, err
	}
	return WalSegment{name, timelineID, logSegNo, logSegNo * WalSegmentSize, size}, nil
}
