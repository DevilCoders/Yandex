package storage

import (
	"context"
	"fmt"
	"time"

	"github.com/gofrs/uuid"
	"github.com/golang/protobuf/proto"
	"github.com/ydb-platform/ydb-go-sdk/v3"
	ydb_result "github.com/ydb-platform/ydb-go-sdk/v3/table/result"
	ydb_named "github.com/ydb-platform/ydb-go-sdk/v3/table/result/named"
	ydb_types "github.com/ydb-platform/ydb-go-sdk/v3/table/types"
	grpc_codes "google.golang.org/grpc/codes"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	tasks_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage/protos"
)

////////////////////////////////////////////////////////////////////////////////

func generateTaskID() string {
	return uuid.Must(uuid.NewV4()).String()
}

////////////////////////////////////////////////////////////////////////////////

type schedule struct {
	taskType      string
	scheduledAt   time.Time
	tasksInflight uint64
}

////////////////////////////////////////////////////////////////////////////////

type idempotencyKey struct {
	idempotencyID string
	accountID     string
}

func getIdempotencyKey(ctx context.Context) idempotencyKey {
	return idempotencyKey{
		idempotencyID: headers.GetIdempotencyKey(ctx),
		accountID:     headers.GetAccountID(ctx),
	}
}

func (k *idempotencyKey) isEmpty() bool {
	return len(k.idempotencyID) == 0
}

////////////////////////////////////////////////////////////////////////////////

func strListValue(strings []string) ydb_types.Value {
	values := make([]ydb_types.Value, 0)
	for _, value := range strings {
		values = append(values, ydb_types.UTF8Value(value))
	}

	var result ydb_types.Value
	if len(values) == 0 {
		result = ydb_types.ZeroValue(ydb_types.List(ydb_types.TypeUTF8))
	} else {
		result = ydb_types.ListValue(values...)
	}

	return result
}

////////////////////////////////////////////////////////////////////////////////

func unmarshalErrorDetails(bytes []byte) (*errors.ErrorDetails, error) {
	if len(bytes) == 0 {
		return nil, nil
	}

	m := &errors.ErrorDetails{}

	err := proto.Unmarshal(bytes, m)
	if err != nil {
		return nil, &errors.NonRetriableError{
			Err: fmt.Errorf("failed to unmarshal ErrorDetails: %w", err),
		}
	}

	return m, nil
}

func marshalErrorDetails(m *errors.ErrorDetails) []byte {
	if m == nil {
		return nil
	}

	bytes, err := proto.Marshal(m)
	if err != nil {
		// TODO: Throw an error.
		return nil
	}

	return bytes
}

func unmarshalStringMap(bytes []byte) (map[string]string, error) {
	m := &protos.StringMap{}

	err := proto.Unmarshal(bytes, m)
	if err != nil {
		return map[string]string{}, &errors.NonRetriableError{Err: err}
	}

	return m.Values, nil
}

func marshalStringMap(values map[string]string) []byte {
	m := &protos.StringMap{
		Values: values,
	}

	bytes, err := proto.Marshal(m)
	if err != nil {
		// TODO: Throw an error.
		return nil
	}

	return bytes
}

func marshalStrings(values []string) []byte {
	strings := &protos.Strings{
		Values: values,
	}

	bytes, err := proto.Marshal(strings)
	if err != nil {
		// TODO: Throw an error.
		return nil
	}

	return bytes
}

func unmarshalStrings(bytes []byte) ([]string, error) {
	strings := &protos.Strings{}

	err := proto.Unmarshal(bytes, strings)
	if err != nil {
		return []string{}, &errors.NonRetriableError{Err: err}
	}

	return strings.Values, nil
}

func isCancelledError(err error) bool {
	switch {
	case
		errors.Is(err, context.Canceled),
		ydb.IsTransportError(err, grpc_codes.Canceled):
		return true
	default:
		return false
	}
}

////////////////////////////////////////////////////////////////////////////////

func scanTaskInfos(ctx context.Context, res ydb_result.Result) (taskInfos []TaskInfo, err error) {
	for res.NextResultSet(ctx) {
		for res.NextRow() {
			var info TaskInfo
			err = res.ScanNamed(
				ydb_named.OptionalWithDefault("id", &info.ID),
				ydb_named.OptionalWithDefault("generation_id", &info.GenerationID),
			)
			if err != nil {
				return taskInfos, &errors.NonRetriableError{
					Err: fmt.Errorf("scanTaskInfos: failed to parse row: %w", err),
				}
			}
			taskInfos = append(taskInfos, info)
		}
	}

	return taskInfos, res.Err()
}

////////////////////////////////////////////////////////////////////////////////
// TaskState marshal/unmarshal routines.

func (s *TaskState) structValue() ydb_types.Value {
	return ydb_types.StructValue(
		ydb_types.StructFieldValue("id", ydb_types.UTF8Value(s.ID)),
		ydb_types.StructFieldValue("idempotency_key", ydb_types.UTF8Value(s.IdempotencyKey)),
		ydb_types.StructFieldValue("account_id", ydb_types.UTF8Value(s.AccountID)),
		ydb_types.StructFieldValue("task_type", ydb_types.UTF8Value(s.TaskType)),
		ydb_types.StructFieldValue("regular", ydb_types.BoolValue(s.Regular)),
		ydb_types.StructFieldValue("description", ydb_types.UTF8Value(s.Description)),
		ydb_types.StructFieldValue("created_at", persistence.TimestampValue(s.CreatedAt)),
		ydb_types.StructFieldValue("created_by", ydb_types.UTF8Value(s.CreatedBy)),
		ydb_types.StructFieldValue("modified_at", persistence.TimestampValue(s.ModifiedAt)),
		ydb_types.StructFieldValue("generation_id", ydb_types.Uint64Value(s.GenerationID)),
		ydb_types.StructFieldValue("status", ydb_types.Int64Value(int64(s.Status))),
		ydb_types.StructFieldValue("error_code", ydb_types.Int64Value(int64(s.ErrorCode))),
		ydb_types.StructFieldValue("error_message", ydb_types.UTF8Value(s.ErrorMessage)),
		ydb_types.StructFieldValue("error_silent", ydb_types.BoolValue(s.ErrorSilent)),
		ydb_types.StructFieldValue("error_details", ydb_types.StringValue(marshalErrorDetails(s.ErrorDetails))),
		ydb_types.StructFieldValue("retriable_error_count", ydb_types.Uint64Value(s.RetriableErrorCount)),
		ydb_types.StructFieldValue("state", ydb_types.StringValue(s.State)),
		ydb_types.StructFieldValue("metadata", ydb_types.StringValue(marshalStringMap(s.Metadata.Vals()))),
		ydb_types.StructFieldValue("dependencies", ydb_types.StringValue(marshalStrings(s.Dependencies.List()))),
		ydb_types.StructFieldValue("changed_state_at", persistence.TimestampValue(s.ChangedStateAt)),
		ydb_types.StructFieldValue("ended_at", persistence.TimestampValue(s.EndedAt)),
		ydb_types.StructFieldValue("last_host", ydb_types.UTF8Value(s.LastHost)),
		ydb_types.StructFieldValue("last_runner", ydb_types.UTF8Value(s.LastRunner)),
		ydb_types.StructFieldValue("zone_id", ydb_types.UTF8Value(s.ZoneID)),
		ydb_types.StructFieldValue("cloud_id", ydb_types.UTF8Value(s.CloudID)),
		ydb_types.StructFieldValue("folder_id", ydb_types.UTF8Value(s.FolderID)),
		ydb_types.StructFieldValue("estimated_time", persistence.TimestampValue(s.EstimatedTime)),
		ydb_types.StructFieldValue("dependants", ydb_types.StringValue(marshalStrings(s.dependants.List()))),
	)
}

func taskStateStructTypeString() string {
	return `Struct<
		id: Utf8,
		idempotency_key: Utf8,
		account_id: Utf8,
		task_type: Utf8,
		regular: Bool,
		description: Utf8,
		created_at: Timestamp,
		created_by: Utf8,
		modified_at: Timestamp,
		generation_id: Uint64,
		status: Int64,
		error_code: Int64,
		error_message: Utf8,
		error_silent: Bool,
		error_details: String,
		retriable_error_count: Uint64,
		state: String,
		metadata: String,
		dependencies: String,
		changed_state_at: Timestamp,
		ended_at: Timestamp,
		last_host: Utf8,
		last_runner: Utf8,
		zone_id: Utf8,
		cloud_id: Utf8,
		folder_id: Utf8,
		estimated_time: Timestamp,
		dependants: String>`
}

func taskStateTableDescription() persistence.CreateTableDescription {
	return persistence.MakeCreateTableDescription(
		persistence.WithColumn("id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("idempotency_key", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("account_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("task_type", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("regular", ydb_types.Optional(ydb_types.TypeBool)),
		persistence.WithColumn("description", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("created_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("created_by", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("modified_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("generation_id", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("status", ydb_types.Optional(ydb_types.TypeInt64)),
		persistence.WithColumn("error_code", ydb_types.Optional(ydb_types.TypeInt64)),
		persistence.WithColumn("error_message", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("error_silent", ydb_types.Optional(ydb_types.TypeBool)),
		persistence.WithColumn("error_details", ydb_types.Optional(ydb_types.TypeString)),
		persistence.WithColumn("retriable_error_count", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("state", ydb_types.Optional(ydb_types.TypeString)),
		persistence.WithColumn("metadata", ydb_types.Optional(ydb_types.TypeString)),
		persistence.WithColumn("dependencies", ydb_types.Optional(ydb_types.TypeString)),
		persistence.WithColumn("changed_state_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("ended_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("last_host", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("last_runner", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("cloud_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("folder_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("estimated_time", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("dependants", ydb_types.Optional(ydb_types.TypeString)),
		persistence.WithPrimaryKeyColumn("id"),
	)
}

func readyToExecuteStructTypeString() string {
	return `Struct<
		id: Utf8,
		generation_id: Uint64,
		task_type: Utf8,
		zone_id: Utf8>`
}

func executingStructTypeString() string {
	return `Struct<
		id: Utf8,
		generation_id: Uint64,
		modified_at: Timestamp,
		task_type: Utf8,
		zone_id: Utf8>`
}

func (s *storageYDB) scanTaskState(res ydb_result.Result) (state TaskState, err error) {
	var (
		errorCode    int64
		errorDetails []byte
		metadata     []byte
		dependencies []byte
		dependants   []byte
	)
	err = res.ScanNamed(
		ydb_named.OptionalWithDefault("id", &state.ID),
		ydb_named.OptionalWithDefault("idempotency_key", &state.IdempotencyKey),
		ydb_named.OptionalWithDefault("account_id", &state.AccountID),
		ydb_named.OptionalWithDefault("task_type", &state.TaskType),
		ydb_named.OptionalWithDefault("regular", &state.Regular),
		ydb_named.OptionalWithDefault("description", &state.Description),
		ydb_named.OptionalWithDefault("created_at", &state.CreatedAt),
		ydb_named.OptionalWithDefault("created_by", &state.CreatedBy),
		ydb_named.OptionalWithDefault("modified_at", &state.ModifiedAt),
		ydb_named.OptionalWithDefault("generation_id", &state.GenerationID),
		ydb_named.OptionalWithDefault("status", &state.Status),
		ydb_named.OptionalWithDefault("error_code", &errorCode),
		ydb_named.OptionalWithDefault("error_message", &state.ErrorMessage),
		ydb_named.OptionalWithDefault("error_silent", &state.ErrorSilent),
		ydb_named.OptionalWithDefault("error_details", &errorDetails),
		ydb_named.OptionalWithDefault("retriable_error_count", &state.RetriableErrorCount),
		ydb_named.OptionalWithDefault("state", &state.State),
		ydb_named.OptionalWithDefault("metadata", &metadata),
		ydb_named.OptionalWithDefault("dependencies", &dependencies),
		ydb_named.OptionalWithDefault("changed_state_at", &state.ChangedStateAt),
		ydb_named.OptionalWithDefault("ended_at", &state.EndedAt),
		ydb_named.OptionalWithDefault("last_host", &state.LastHost),
		ydb_named.OptionalWithDefault("last_runner", &state.LastRunner),
		ydb_named.OptionalWithDefault("zone_id", &state.ZoneID),
		ydb_named.OptionalWithDefault("cloud_id", &state.CloudID),
		ydb_named.OptionalWithDefault("folder_id", &state.FolderID),
		ydb_named.OptionalWithDefault("estimated_time", &state.EstimatedTime),
		ydb_named.OptionalWithDefault("dependants", &dependants),
	)
	if err != nil {
		return state, &errors.NonRetriableError{
			Err: fmt.Errorf("scanTaskStates: failed to parse row: %w", err),
		}
	}

	state.StorageFolder = s.folder
	state.ErrorCode = grpc_codes.Code(errorCode)
	state.ErrorDetails, err = unmarshalErrorDetails(errorDetails)
	if err != nil {
		return state, err
	}

	metadataValues, err := unmarshalStringMap(metadata)
	if err != nil {
		return state, fmt.Errorf("failed to parse metadata: %w", err)
	}

	state.Metadata = MakeMetadata(metadataValues)
	depsValues, err := unmarshalStrings(dependencies)
	if err != nil {
		return state, fmt.Errorf("failed to parse dependencies: %w", err)
	}

	state.Dependencies = MakeStringSet(depsValues...)
	dependantValues, err := unmarshalStrings(dependants)
	if err != nil {
		return state, &errors.NonRetriableError{
			Err: fmt.Errorf("failed to parse dependants: %w", err),
		}
	}

	state.dependants = MakeStringSet(dependantValues...)
	return state, res.Err()
}

func (s *storageYDB) scanTaskStates(ctx context.Context, res ydb_result.Result) ([]TaskState, error) {
	var states []TaskState
	for res.NextResultSet(ctx) {
		for res.NextRow() {
			state, err := s.scanTaskState(res)
			if err != nil {
				return nil, err
			}
			states = append(states, state)
		}
	}

	return states, res.Err()
}

////////////////////////////////////////////////////////////////////////////////

func CreateYDBTables(
	ctx context.Context,
	config *tasks_config.TasksConfig,
	db *persistence.YDBClient,
) error {

	logging.Info(ctx, "Creating tables for tasks in %v", db.AbsolutePath(config.GetStorageFolder()))

	err := db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "tasks", taskStateTableDescription())
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created tasks table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "task_ids", persistence.MakeCreateTableDescription(
		persistence.WithColumn("task_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("idempotency_key", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("account_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("idempotency_key", "account_id"),
		persistence.WithSecondaryKeyColumn("task_id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created task_ids table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "ready_to_run", persistence.MakeCreateTableDescription(
		persistence.WithColumn("id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("generation_id", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("task_type", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created ready_to_run table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "ready_to_cancel", persistence.MakeCreateTableDescription(
		persistence.WithColumn("id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("generation_id", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("task_type", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created ready_to_cancel table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "running", persistence.MakeCreateTableDescription(
		persistence.WithColumn("id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("generation_id", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("modified_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("task_type", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created running table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "cancelling", persistence.MakeCreateTableDescription(
		persistence.WithColumn("id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("generation_id", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithColumn("modified_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("task_type", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("zone_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created cancelling table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "ended", persistence.MakeCreateTableDescription(
		persistence.WithColumn("ended_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("idempotency_key", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("account_id", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithPrimaryKeyColumn("ended_at", "id"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created ended table")

	err = db.CreateOrAlterTable(ctx, config.GetStorageFolder(), "schedules", persistence.MakeCreateTableDescription(
		persistence.WithColumn("task_type", ydb_types.Optional(ydb_types.TypeUTF8)),
		persistence.WithColumn("scheduled_at", ydb_types.Optional(ydb_types.TypeTimestamp)),
		persistence.WithColumn("tasks_inflight", ydb_types.Optional(ydb_types.TypeUint64)),
		persistence.WithPrimaryKeyColumn("task_type"),
	))
	if err != nil {
		return err
	}
	logging.Info(ctx, "Created schedules table")

	logging.Info(ctx, "Created tables for tasks")

	return nil
}

func DropYDBTables(
	ctx context.Context,
	config *tasks_config.TasksConfig,
	db *persistence.YDBClient,
) error {

	logging.Info(ctx, "Dropping tables for tasks in %v", db.AbsolutePath(config.GetStorageFolder()))

	err := db.DropTable(ctx, config.GetStorageFolder(), "tasks")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped tasks table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "task_ids")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped task_ids table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "ready_to_run")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped ready_to_run table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "ready_to_cancel")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped ready_to_cancel table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "running")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped running table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "cancelling")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped cancelling table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "ended")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped ended table")

	err = db.DropTable(ctx, config.GetStorageFolder(), "schedules")
	if err != nil {
		return err
	}
	logging.Info(ctx, "Dropped schedules table")

	logging.Info(ctx, "Dropped tables for tasks")

	return nil
}
