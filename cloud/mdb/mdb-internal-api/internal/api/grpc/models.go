package grpc

import (
	"context"
	"fmt"
	"reflect"
	"strings"
	"time"

	"github.com/golang/protobuf/ptypes"
	"github.com/golang/protobuf/ptypes/timestamp"
	"github.com/golang/protobuf/ptypes/wrappers"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../../scripts/mockgen.sh StreamClusterLogsRequest

func TimeToGRPC(ts time.Time) *timestamp.Timestamp {
	if ts.IsZero() {
		return nil
	}
	return &timestamp.Timestamp{Seconds: ts.Unix(), Nanos: int32(ts.Nanosecond())}
}

func TimePtrToGRPC(ts *time.Time) *timestamp.Timestamp {
	if ts == nil {
		return nil
	}
	return TimeToGRPC(*ts)
}

func OptionalTimeToGRPC(ts optional.Time) *timestamp.Timestamp {
	if !ts.Valid {
		return nil
	}
	return &timestamp.Timestamp{Seconds: ts.Time.Unix(), Nanos: int32(ts.Time.Nanosecond())}
}

func TimeFromGRPC(ts *timestamp.Timestamp) time.Time {
	return time.Unix(ts.GetSeconds(), int64(ts.GetNanos())).UTC()
}

func OptionalTimeFromGRPC(ts *timestamp.Timestamp) optional.Time {
	return optional.Time{
		Time:  ts.AsTime(),
		Valid: ts.CheckValid() == nil,
	}
}

func FilterFromGRPC(filterString string) ([]sqlfilter.Term, error) {
	if filterString == "" {
		return nil, nil
	}
	return sqlfilter.Parse(filterString)
}

func OperationToGRPC(ctx context.Context, op operations.Operation, l log.Logger) (*operation.Operation, error) {
	desc, err := operations.GetDescriptor(op.Type)
	if err != nil {
		return nil, err
	}

	md, ok := op.MetaData.(operations.Metadata)
	if !ok {
		return nil, xerrors.Errorf("operation %+v missing metadata", op)
	}

	mdAny, err := ptypes.MarshalAny(md.Build(op))
	if err != nil {
		return nil, xerrors.Errorf("operation metadata proto marshal: %w", err)
	}

	res := operation.Operation{
		Id:          op.OperationID,
		Description: desc.Description,
		CreatedAt:   TimeToGRPC(op.CreatedAt),
		CreatedBy:   op.CreatedBy,
		ModifiedAt:  TimeToGRPC(op.ModifiedAt),
		Done:        op.Status.IsTerminal(),
		Metadata:    mdAny,
	}

	switch op.Status {
	case operations.StatusFailed:
		res.Result, err = buildOperationError(ctx, op, l)
	default:
		res.Result, err = buildOperationResponse(op)
	}
	if err != nil {
		return nil, err
	}

	return &res, nil
}

func buildOperationError(ctx context.Context, op operations.Operation, l log.Logger) (*operation.Operation_Error, error) {
	// Remember code and message of last error for later use as root error
	// Create statuses for all errors
	code := codes.Unknown
	msg := "Unknown error"
	var errors []*status.Status
	for i := len(op.Errors) - 1; i >= 0; i-- {
		opErr := op.Errors[i]
		// TODO: check for support users (always expose)
		// TODO: check for API flag (always expose)
		if !opErr.Exposable {
			continue
		}

		code = codes.Code(opErr.Code)
		msg = opErr.Message
		errors = append(errors, status.New(codes.Code(opErr.Code), opErr.Message))
	}

	// Build root error with array of details
	root := status.New(code, msg)
	for _, e := range errors {
		// Can return nil on failure so do not overwrite root right away
		r, err := root.WithDetails(e.Proto())
		if err != nil {
			ctxlog.Error(ctx, l, fmt.Sprintf("failed to unmarshal operation error %q", e.Err()), log.Error(err))
			continue
		}

		root = r
	}

	return &operation.Operation_Error{Error: root.Proto()}, nil
}

func buildOperationResponse(op operations.Operation) (*operation.Operation_Response, error) {
	r, err := ptypes.MarshalAny(status.New(codes.OK, "OK").Proto())
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal operation %+v response: %w", op, err)
	}

	return &operation.Operation_Response{Response: r}, nil
}

func OptionalInt64FromGRPC(v *wrappers.Int64Value) optional.Int64 {
	if v == nil {
		return optional.Int64{}
	}
	return optional.NewInt64(v.GetValue())
}

func OptionalInt64ZeroInvalidFromGRPC(v int64) optional.Int64 {
	if v == 0 {
		return optional.Int64{}
	}
	return optional.NewInt64(v)
}

func OptionalInt64ToGRPC(v optional.Int64) *wrappers.Int64Value {
	if !v.Valid {
		return nil
	}
	return &wrappers.Int64Value{Value: v.Must()}
}

func OptionalFloat64FromGRPC(v *wrappers.DoubleValue) optional.Float64 {
	if v == nil {
		return optional.Float64{}
	}
	return optional.NewFloat64(v.GetValue())
}

func OptionalFloat64ToGRPC(v optional.Float64) *wrappers.DoubleValue {
	if !v.Valid {
		return nil
	}
	return &wrappers.DoubleValue{Value: v.Must()}
}

func OptionalBoolFromGRPC(v *wrappers.BoolValue) optional.Bool {
	if v == nil {
		return optional.Bool{}
	}
	return optional.NewBool(v.GetValue())
}

func OptionalBoolToGRPC(v optional.Bool) *wrappers.BoolValue {
	if !v.Valid {
		return nil
	}
	return &wrappers.BoolValue{Value: v.Must()}
}

func OptionalInt64ToBoolGRPC(v optional.Int64) *wrappers.BoolValue {
	if !v.Valid {
		return nil
	}
	return &wrappers.BoolValue{Value: v.Must() == 1}
}

func OptionalInt64FromBoolGRPC(b *wrappers.BoolValue) optional.Int64 {
	if b == nil {
		return optional.Int64{}
	}
	if b.Value {
		return optional.NewInt64(1)
	}
	return optional.NewInt64(0)
}

func OptionalDurationFromGRPC(d *wrappers.Int64Value) optional.Duration {
	if d == nil {
		return optional.Duration{}
	}
	return optional.NewDuration(time.Millisecond * time.Duration(d.GetValue()))
}

func OptionalDurationToGRPC(d optional.Duration) *wrappers.Int64Value {
	if !d.Valid {
		return nil
	}
	return &wrappers.Int64Value{Value: d.Must().Milliseconds()}
}

func OptionalSecondsFromGRPCMilliseconds(w *wrappers.Int64Value) optional.Int64 {
	if w == nil {
		return optional.Int64{}
	}

	return optional.NewInt64(w.Value / 1000)
}

func OptionalSecondsToGRPCMilliseconds(s optional.Int64) *wrappers.Int64Value {
	if !s.Valid {
		return nil
	}

	return &wrappers.Int64Value{Value: s.Int64 * 1000}
}

func IsNotEmptyValue(_ string, field interface{}) bool {
	strField, ok := field.(string)
	if ok {
		return strField != ""
	}
	if reflect.TypeOf(field).Kind() != reflect.Ptr {
		return false
	}
	return !reflect.ValueOf(field).IsNil()
}

type ResourcesGRPC interface {
	GetResourcePresetId() string
	GetDiskSize() int64
	GetDiskTypeId() string
}

func ResourcesFromGRPC(resSpec ResourcesGRPC, paths *FieldPaths) models.ClusterResourcesSpec {
	var cr models.ClusterResourcesSpec
	if resSpec == nil {
		return cr
	}
	if paths.Remove("disk_size") && resSpec.GetDiskSize() != 0 {
		cr.DiskSize.Set(resSpec.GetDiskSize())
	}
	if paths.Remove("disk_type_id") && resSpec.GetDiskTypeId() != "" {
		cr.DiskTypeExtID.Set(resSpec.GetDiskTypeId())
	}
	if paths.Remove("resource_preset_id") && resSpec.GetResourcePresetId() != "" {
		cr.ResourcePresetExtID.Set(resSpec.GetResourcePresetId())
	}
	return cr
}

func findProtoTag(f reflect.StructField, name string) (string, bool) {
	protoTag := f.Tag.Get("protobuf")
	if protoTag == "" {
		return "", false
	}

	tags := strings.Split(protoTag, ",")
	for _, tag := range tags {
		kvs := strings.SplitN(tag, "=", 2)
		if kvs[0] != name {
			continue
		}
		val := ""
		if len(kvs) >= 2 {
			val = kvs[1]
		}
		return val, true
	}
	return "", false
}

func FieldNamesFromGRPCPaths(spec interface{}, paths *FieldPaths) []string {
	var names []string
	specT := reflect.ValueOf(spec).Elem().Type()
	for i := 0; i < specT.NumField(); i++ {
		specFieldT := specT.Field(i)
		protoName, ok := findProtoTag(specFieldT, "name")
		if !ok {
			continue
		}
		if paths.Remove(protoName) {
			names = append(names, specFieldT.Name)
		}
	}
	return names
}

func OptionalStringFromGRPC(v string) optional.String {
	if v == "" {
		return optional.String{}
	}

	return optional.NewString(v)
}

func OptionalPasswordFromGRPC(v string) optional.OptionalPassword {
	if v == "" {
		return optional.OptionalPassword{}
	}

	return optional.NewOptionalPassword(secret.NewString(v))
}

type AccessGRPC interface {
	GetDataLens() bool
	GetWebSql() bool
	GetMetrika() bool
	GetServerless() bool
	GetDataTransfer() bool
	GetYandexQuery() bool
}

func AccessFromGRPC(a AccessGRPC) clusters.Access {
	var access clusters.Access
	access.DataLens = optional.NewBool(a.GetDataLens())
	access.WebSQL = optional.NewBool(a.GetWebSql())
	access.Metrica = optional.NewBool(a.GetMetrika())
	access.Serverless = optional.NewBool(a.GetServerless())
	access.DataTransfer = optional.NewBool(a.GetDataTransfer())
	access.YandexQuery = optional.NewBool(a.GetYandexQuery())
	return access
}
