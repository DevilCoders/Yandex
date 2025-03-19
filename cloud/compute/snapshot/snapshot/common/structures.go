package common

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"go.uber.org/zap"
)

// UpdatableMetadata is a part of snapshot metadata which can be updated.
type UpdatableMetadata struct {
	Metadata    string `json:"metadata"`
	Description string `json:"description"`
}

// CreationMetadata is a part of snapshot metadata which cannot be derived through conversion.
type CreationMetadata struct {
	UpdatableMetadata
	ID           string `json:"id"`
	Organization string `json:"organization"`
	ProjectID    string `json:"project_id"`
	ProductID    string `json:"product_id"`
	ImageID      string `json:"image_id"`
	Name         string `json:"name"`
	Public       bool   `json:"public"`
}

// CreationInfo is a part of snapshot metadata which must be passed for snapshot creation.
type CreationInfo struct {
	CreationMetadata
	Base          string `json:"base"`
	BaseProjectID string `json:"base_project_id"`
	Size          int64  `json:"size"`
	Disk          string `json:"disk"`
}

func (ci *CreationInfo) String() string {
	return fmt.Sprintf("id=%s,name=%s", ci.ID, ci.Name)
}

// SizeChange is an event of snapshot RealSize change.
type SizeChange struct {
	Timestamp time.Time
	RealSize  int64
}

// State describes current snapshot state.
type State struct {
	Code        string `json:"code"`
	Description string `json:"description"`
}

// SnapshotInfo contains snapshot metadata.
type SnapshotInfo struct {
	CreationInfo
	Created    time.Time    `json:"created"`
	RealSize   int64        `json:"real_size"`
	SharedWith []string     `json:"shared_with"`
	Changes    []SizeChange `json:"changes"`
	State      State        `json:"state"`
}

// Equals checks whether another SnapshotInfo is equal to current.
func (i SnapshotInfo) Equals(other SnapshotInfo) bool {
	if len(i.SharedWith) != len(other.SharedWith) ||
		len(i.Changes) != len(other.Changes) {
		return false
	}

	for j, v := range i.SharedWith {
		if v != other.SharedWith[j] {
			return false
		}
	}

	for j, v := range i.Changes {
		if v.RealSize != other.Changes[j].RealSize ||
			!v.Timestamp.Equal(other.Changes[j].Timestamp) {
			return false
		}
	}

	return i.CreationInfo == other.CreationInfo &&
		i.ID == other.ID &&
		i.Created.Equal(other.Created) &&
		i.RealSize == other.RealSize &&
		i.Public == other.Public &&
		i.State == other.State
}

// ChunkInfo contains metadata of a single chunk.
type ChunkInfo struct {
	Hashsum string
	Offset  int64
	Size    int64
	Zero    bool
}

// ConvertRequest describes image source and metadata for conversion.
type ConvertRequest struct {
	CreationMetadata
	Format string `json:"format"`
	// s3
	Bucket string `json:"bucket"`
	Key    string `json:"key"`
	// http
	URL string `json:"url"`
}

func (cr *ConvertRequest) String() string {
	return fmt.Sprintf("id=%s,name=%s,url=%s", cr.ID, cr.Name, cr.URL)
}

// UpdateRequest describes snapshot fields to be changed
type UpdateRequest struct {
	ID          string
	Metadata    *string
	Description *string
	Public      *bool
}

// ChunkMismatch is one chunk checksum mismatch message
type ChunkMismatch struct {
	Offset  int64
	ChunkID string
	Details string
}

// VerifyReport contains verification status
type VerifyReport struct {
	ID              string
	Status          string
	Details         string
	ChecksumMatched bool
	ChunkSize       int64
	TotalChunks     int64
	EmptyChunks     int64
	ZeroChunks      int64
	DataChunks      int64
	Mismatches      []*ChunkMismatch
}

// TaskRequest is an interface to implement for all request going into task
type TaskRequest interface {
	IsTaskRequest()
}

// CopyParams is task-like params for shallow copy
type CopyParams struct {
	TaskID             string
	HeartbeatTimeoutMs int32
	HeartbeatEnabled   bool
}

func (c CopyRequest) IsTaskRequest() {}

// CopyRequest is used to initiate a shallow copy
type CopyRequest struct {
	OperationCloudID   string
	OperationProjectID string

	ID              string
	TargetID        string
	TargetProjectID string
	Name            string
	ImageID         string
	Params          CopyParams
}

// MoveSrc is an interface to implement for data moving source
type MoveSrc interface {
	IsMoveSrc()
	Description() string
	fmt.Stringer
}

// MoveDst is an interface to implement for data moving destination
type MoveDst interface {
	IsMoveDst()
	Description() string
	fmt.Stringer
}

// NbsMoveSrc is an NBS data moving source
type NbsMoveSrc struct {
	ClusterID         string
	DiskID            string
	FirstCheckpointID string
	LastCheckpointID  string
}

// IsMoveSrc ...
func (*NbsMoveSrc) IsMoveSrc() {}

func (nms *NbsMoveSrc) Description() string {
	return fmt.Sprintf("nbs=%s/%s", nms.ClusterID, nms.DiskID)
}

func (nms *NbsMoveSrc) String() string {
	return fmt.Sprintf("%#v", nms)
}

// NbsMoveDst is an NBS data moving destination
type NbsMoveDst struct {
	ClusterID string
	DiskID    string
	Mount     bool
}

// IsMoveDst ...
func (*NbsMoveDst) IsMoveDst() {}

func (nmd *NbsMoveDst) Description() string {
	return fmt.Sprintf("nbs=%s/%s", nmd.ClusterID, nmd.DiskID)
}

func (nmd *NbsMoveDst) String() string {
	return fmt.Sprintf("%#v", nmd)
}

// SnapshotMoveSrc is a snapshot data moving source
type SnapshotMoveSrc struct {
	SnapshotID string
}

// IsMoveSrc ...
func (*SnapshotMoveSrc) IsMoveSrc() {}

func (sms *SnapshotMoveSrc) Description() string {
	return fmt.Sprintf("snapshot=%s", sms.SnapshotID)
}

func (sms *SnapshotMoveSrc) String() string {
	return fmt.Sprintf("%#v", sms)
}

// SnapshotMoveDst is a snapshot data moving destination
type SnapshotMoveDst struct {
	SnapshotID     string
	ProjectID      string
	Create         bool
	Commit         bool
	Fail           bool
	Name           string
	ImageID        string
	BaseSnapshotID string
}

// IsMoveDst ...
func (*SnapshotMoveDst) IsMoveDst() {}

func (smd *SnapshotMoveDst) Description() string {
	if smd.SnapshotID != "" {
		return fmt.Sprintf("snapshot=%s", smd.SnapshotID)
	}
	return fmt.Sprintf("snapshot=%s", smd.Name)
}

func (smd *SnapshotMoveDst) String() string {
	return fmt.Sprintf("%#v", smd)
}

// NullMoveSrc is a dummy moving source for tests
type NullMoveSrc struct {
	Size      int64
	BlockSize int64
	Random    bool
}

// IsMoveSrc ...
func (*NullMoveSrc) IsMoveSrc() {}

func (nms *NullMoveSrc) Description() string {
	if nms.Random {
		return "null=random"
	}
	return "null=zero"
}

func (nms *NullMoveSrc) String() string {
	return fmt.Sprintf("%#v", nms)
}

// NullMoveDst is a dummy moving destination for tests
type NullMoveDst struct {
	Size          int64
	BlockSize     int64
	CloseCallback func(context.Context, error)
}

// IsMoveDst ...
func (*NullMoveDst) IsMoveDst() {}

func (nmd *NullMoveDst) Description() string {
	return "null"
}

func (nmd *NullMoveDst) String() string {
	return fmt.Sprintf("%#v", nmd)
}

// URLMoveSrc is a url moving source for conversion
type URLMoveSrc struct {
	Bucket string
	Key    string
	URL    string
	Format string

	ShadowedURL string
}

// IsMoveSrc ...
func (*URLMoveSrc) IsMoveSrc() {}

func (ums *URLMoveSrc) Description() string {
	var src string
	if ums.URL != "" {
		src = fmt.Sprintf("url=%s", ums.URL)
	} else {
		src = fmt.Sprintf("s3=%s/%s", ums.Bucket, ums.Key)
	}
	if ums.Format != "" {
		src += fmt.Sprintf("(%s)", ums.Format)
	}
	return src
}

func (ums *URLMoveSrc) String() string {
	return fmt.Sprintf("%#v", ums)
}

// MoveParams contains data moving task parameters
type MoveParams struct {
	Offset             int64
	BlockSize          int64
	WorkersCount       int
	TaskID             string
	HeartbeatTimeoutMs int32
	HeartbeatEnabled   bool
	SkipZeroes         bool
	SkipNonexistent    bool
}

// MoveRequest is used to initiate data moving task
type MoveRequest struct {
	Src                MoveSrc
	Dst                MoveDst
	Params             MoveParams
	OperationCloudID   string
	OperationProjectID string
}

func (m MoveRequest) IsTaskRequest() {}

// TaskInfo is a data task status and it's id
type TaskInfo struct {
	TaskID string
	Status TaskStatus
}

// TaskStatus is a data moving task status
type TaskStatus struct {
	Finished   bool
	Success    bool
	Progress   float64
	Offset     int64
	Error      error
	CreatedAt  time.Time
	FinishedAt *time.Time // nil for not finished yet tasks
}

type TaskData struct {
	Status  TaskStatus
	Request TaskRequest
}

// DeleteParams is task-like params for deletion
type DeleteParams struct {
	TaskID             string
	HeartbeatTimeoutMs int32
	HeartbeatEnabled   bool
}

// DeleteRequest is used to initiate snapshot deletion
type DeleteRequest struct {
	OperationCloudID   string
	OperationProjectID string

	ID              string
	SkipStatusCheck bool
	Params          DeleteParams
}

func (d DeleteRequest) IsTaskRequest() {}

// ChangedChild indicates child size change on base deletion
type ChangedChild struct {
	ID        string
	Timestamp time.Time
	RealSize  int64
}

// SnapshotError is struct for meaningful snapshot service errors.
type SnapshotError struct {
	Code    string `json:"code"`
	Message string `json:"message"`
	Public  bool   `json:"public"`
}

// NewError creates new error.
func NewError(code, message string, public bool) *SnapshotError {
	return &SnapshotError{
		Code:    code,
		Message: message,
		Public:  public,
	}
}

// Error returns error message.
func (e *SnapshotError) Error() string {
	return e.Message
}

// JSON converts error to json.
func (e *SnapshotError) JSON() string {
	b, err := json.Marshal(e)
	if err != nil {
		zap.L().Error("error conversion failed", zap.Any("error", e), zap.Error(err))
		return e.Message
	}
	return string(b)
}

func IsSnapshotPublicError(err error) bool {
	e, ok := err.(*SnapshotError)
	return ok && e.Public
}
