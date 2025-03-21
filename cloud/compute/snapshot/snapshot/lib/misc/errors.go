package misc

import (
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
)

var (
	// Chunker errors

	// ErrChunkSizeTooBig ...
	ErrChunkSizeTooBig = common.NewError("ErrChunkSizeTooBig", "chunk size must be less then 64 Mb", false)
	// ErrSmallChunk ...
	ErrSmallChunk = common.NewError("ErrSmallChunk", "last chunks is smaller then chunk size", false)
	// ErrInvalidSize ...
	ErrInvalidSize = common.NewError("ErrInvalidSize", "snapshot size is invalid", false)

	// Database errors

	// ErrSnapshotNotFound ...
	ErrSnapshotNotFound = common.NewError("ErrSnapshotNotFound", "snapshot not found", true)
	// ErrSnapshotCorrupted ...
	ErrSnapshotCorrupted = common.NewError("ErrSnapshotCorrupted", "snapshot is corrupted", false)
	// ErrSnapshotNotReady ...
	ErrSnapshotNotReady = common.NewError("ErrSnapshotNotReady", "snapshot is not ready yet", false)
	// ErrSnapshotReadOnly ...
	ErrSnapshotReadOnly = common.NewError("ErrSnapshotReadOnly", "snapshot is read-only", false)
	// ErrInvalidBase ...
	ErrInvalidBase = common.NewError("ErrInvalidBase", "base is invalid", false)
	// ErrInvalidOffset ...
	ErrInvalidOffset = common.NewError("ErrInvalidOffset", "Offset must be at the border of a chunk", false)
	// ErrDifferentSize ...
	ErrDifferentSize = common.NewError("ErrDifferentSize", "snapshot chunks are of different size", false)
	// ErrSnapshotNotFull ...
	ErrSnapshotNotFull = common.NewError("ErrSnapshotNotFull", "snapshot is not fully loaded", false)
	// ErrInvalidChange ...
	ErrInvalidChange = common.NewError("ErrInvalidChange", "size change is invalid", false)
	// ErrNoSuchChunk ...
	ErrNoSuchChunk = common.NewError("ErrNoSuchChunk", "no chunk with specified offset in snapshot", false)
	// ErrStateChanged ...
	ErrStateChanged = common.NewError("ErrStateChanged", "internal: unexpected state change", false)
	// ErrInvalidName ...
	ErrInvalidName = common.NewError("ErrInvalidName", "snapshot name must not be empty", false)
	// ErrDuplicateName ...
	ErrDuplicateName = common.NewError("ErrDuplicateName", "snapshot name must be unique", false)
	// ErrDuplicateID ...
	ErrDuplicateID = common.NewError("ErrDuplicateID", "snapshot id must be unique", false)
	// ErrSortingInvalidField ...
	ErrSortingInvalidField = common.NewError("ErrSortingInvalidField", "invalid field for sorting", false)
	// ErrSortingDuplicateField ...
	ErrSortingDuplicateField = common.NewError("ErrSortingDuplicateField", "duplicate field for sorting", false)
	// ErrInvalidProgress ...
	ErrInvalidProgress = common.NewError("ErrInvalidProgress", "write progress is invalid", false)
	// ErrSnapshotLocked ...
	ErrSnapshotLocked = common.NewError("ErrSnapshotLocked", "snapshot is locked due to child creation", false)
	// ErrDuplicateChunk ...
	ErrDuplicateChunk = common.NewError("ErrDuplicateChunk", "such chunk already exists", false)
	// ErrInvalidBlockSize ...
	ErrInvalidBlockSize = common.NewError("ErrInvalidBlockSize", "Storage has incompatible block size", false)
	// ErrInvalidMoveSrc ...
	ErrInvalidMoveSrc = common.NewError("ErrInvalidMoveSrc", "unknown move source", false)
	// ErrInvalidMoveDst ...
	ErrInvalidMoveDst = common.NewError("ErrInvalidMoveDst", "unknown move destination", false)
	// ErrInvalidNbsClusterID ...
	ErrInvalidNbsClusterID = common.NewError("ErrInvalidNbsClusterID", "invalid NBS cluster ID", false)
	// ErrTaskNotFound ...
	ErrTaskNotFound = common.NewError("ErrTaskNotFound", "task not found", false)
	// ErrDuplicateTaskID ...
	ErrDuplicateTaskID = common.NewError("ErrDuplicateTaskID", "task ID must be unique", false)
	// ErrInvalidTaskID ...
	ErrInvalidTaskID = common.NewError("ErrInvalidTaskID", "task ID is invalid", false)
	// ErrHeartbeatTimeout ...
	ErrHeartbeatTimeout = common.NewError("ErrHeartbeatTimeout", "no heartbeats received during timeout", false)
	// ErrInvalidState ...
	ErrInvalidState = common.NewError("ErrInvalidState", "snapshot state is invalid", false)

	// MDS errors

	// ErrClientNil ...
	ErrClientNil = common.NewError("ErrClientNil", "MDS client is nil", false)
	// ErrUploadInfoNil ...
	ErrUploadInfoNil = common.NewError("ErrUploadInfoNil", "Upload info is nil", false)
	// ErrDuplicateKey ...
	ErrDuplicateKey = common.NewError("ErrDuplicateKey", "duplicate chunk key", false)

	// S3 errors

	// ErrInvalidObject ...
	ErrInvalidObject = common.NewError("ErrInvalidObject", "Bucket or object is invalid", true)

	// Image errors

	// ErrInvalidFormat ...
	ErrInvalidFormat = common.NewError("ErrInvalidFormat", "image format is invalid", true)
	// ErrTooFragmented ...
	ErrTooFragmented = common.NewError("ErrTooFragmented", "image is too fragmented", true)
	// ErrIncorrectMap ...
	ErrIncorrectMap = common.NewError("ErrIncorrectMap", "image map is not valid", true)
	// ErrUnknownSource ...
	ErrUnknownSource = common.NewError("ErrUnknownSource", "image source is unknown", true)
	// ErrSourceNoRange ...
	ErrSourceNoRange = common.NewError("ErrSourceNoRange", "image source does not support HTTP ranges", true)
	// ErrUnreachableSource ...
	ErrUnreachableSource = common.NewError("ErrUnreachableSource", "image source is unreachable", true)
	// ErrDenyRedirect ...
	ErrDenyRedirect = common.NewError("ErrDenyRedirect", "redirect is not allowed", true)
	// ErrSourceChanged ...
	ErrSourceChanged = common.NewError("ErrSourceChanged", "source change is not supported", true)
	// ErrSourceURLNotFound ...
	ErrSourceURLNotFound = common.NewError("ErrSourceURLNotFound", "url source not found", true)
	// ErrSourceURLAccessDenied ...
	ErrSourceURLAccessDenied = common.NewError("ErrSourceURLAccessDenied", "denied access for source url", true)

	// GC errors

	// ErrNothingProcessed ...
	ErrNothingProcessed = common.NewError("ErrNothingProcessed", "no entries were successfully processed on this iteration", false)
	// ErrNotATombstone ...
	ErrNotATombstone = common.NewError("ErrNotATombstone", "the requested snapshot is not a tombstone", false)

	// Other errors

	// ErrInternalRetry ...
	// Must not leave lib scope
	ErrInternalRetry = common.NewError("ErrInternalRetry", "something was wrong", false)
	// ErrNoCommit ...
	// Must not leave lib scope
	ErrNoCommit = common.NewError("ErrNoCommit", "do not commit the transaction", false)

	ErrTaskCancelled = common.NewError("ErrTaskCancelled", "task was cancelled", false)

	// ErrAlreadyLocked ...
	// Must not leave lib scope
	ErrAlreadyLocked = common.NewError("ErrAlreadyLocked", "snapshot is already locked", false)

	// ErrMaxShareLockExceeded
	// Must not leave lib scope
	ErrMaxShareLockExceeded = common.NewError("ErrMaxShareLockExceeded", "Maximum number of shared locks exceeded", false)

	ErrLockGet = common.NewError("ErrLockGet", "Error while get lock", false)

	// ErrWrongLock ...
	// Must not leave lib scope
	ErrWrongLock = common.NewError("ErrWrongLock", "actual lock differs from requested", false)
	// ErrNotLocked ...
	// Must not leave lib scope
	ErrNotLocked = common.NewError("ErrWrongLock", "snapshot is not locked", false)
	// ErrTooLongOuput ...
	ErrTooLongOuput = common.NewError("ErrTooLongOuput", "output is too long", false)
	// ErrCorruptedSource ...
	ErrCorruptedSource = common.NewError("ErrCorruptedSource", "source corrupted", false)
)
