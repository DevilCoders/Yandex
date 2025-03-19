package logging

import (
	"go.uber.org/zap"

	"a.yandex-team.ru/cloud/compute/snapshot/pkg/logging"
)

const (
	operationID = "operation_id"
	requestID   = "request_id"
	requestUID  = "request_uid"
	snapshotID  = "snapshot_id"
)

// SnapshotMetaKeys is the preset of keys for snapshot.
var SnapshotMetaKeys = []logging.MetaKey{
	{Name: operationID, ShortName: "o", VariableName: "OPERATION_ID"},
	{Name: requestID, ShortName: "r", VariableName: "REQUEST_ID"},
	{Name: requestUID, ShortName: "u", VariableName: "REQUEST_UID"},
	{Name: snapshotID, ShortName: "s", VariableName: "SNAPSHOT_ID"},
}

// ChildID is the ID of child snapshot.
func ChildID(id string) zap.Field {
	return zap.String("child_id", id)
}

// ChunkID is ID of snapshot data chunk
func ChunkID(id string) zap.Field {
	return zap.String("chunk_id", id)
}

// SnapshotOffset is Offset of data in snapshot
func SnapshotOffset(offset int64) zap.Field {
	return zap.Int64("chunk_offset", offset)
}

// Function is Name function
func Function(name string) zap.Field {
	return zap.String("func", name)
}

// Method is the key for API method.
func Method(name string) zap.Field {
	return zap.String("method", name)
}

// OperationID is the ID of operation.
func OperationID(id string) zap.Field {
	return zap.String(operationID, id)
}

// Request is the key for API request.
func Request(req interface{}) zap.Field {
	return zap.Any("request", req)
}

// RequestID is the ID of request.
func RequestID(id string) zap.Field {
	return zap.String(requestID, id)
}

// RequestUID is the ID of sub-request.
func RequestUID(uid string) zap.Field {
	return zap.String(requestUID, uid)
}

// SnapshotID is the ID of snapshot.
func SnapshotID(id string) zap.Field {
	return zap.String(snapshotID, id)
}

// ComputeTaskID is the ID of snapshot task.
func TaskID(id string) zap.Field {
	return zap.String("task_id", id)
}
