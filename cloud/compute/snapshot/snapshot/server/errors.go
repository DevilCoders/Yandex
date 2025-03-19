package server

import (
	stdctx "context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"reflect"
	"strings"

	"a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"

	"github.com/grpc-ecosystem/grpc-gateway/runtime"
	"go.uber.org/zap"
	"golang.org/x/net/context"
	"google.golang.org/grpc/codes"

	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

var (
	// ErrInvalidTimestamp ...
	ErrInvalidTimestamp = common.NewError("ErrInvalidTimestamp", "timestamp: invalid seconds or nanos", false)

	// ErrInvalidCoord ...
	ErrInvalidCoord = common.NewError("ErrInvalidCoord", "coord is invalid", false)
	// ErrInvalidLimit ...
	ErrInvalidLimit = common.NewError("ErrInvalidLimit", "limit must be between 1 and 1000", false)
	// ErrCursorAndSortLimit ...
	ErrCursorAndSortLimit = common.NewError("ErrCursorAndSortLimit", "either cursor or sort/limit may be specified", false)
	// ErrSortWithoutLimit ...
	ErrSortWithoutLimit = common.NewError("ErrSortWithoutLimit", "limit must be specified with sort", false)
	// ErrInvalidChange ...
	ErrInvalidChange = common.NewError("ErrInvalidChange", "only 'metadata' and 'description' are updatable", false)
	// ErrOperationCanceled ...
	ErrOperationCanceled = common.NewError("ErrOperationCanceled", "operation was canceled", false)
	// ErrDeadlineExceeded ...
	ErrDeadlineExceeded = common.NewError("ErrDeadlineExceeded", "deadline exceeded", false)
)

func convertError(e error) error {
	switch e := e.(type) {
	case *common.SnapshotError:
		switch e {
		case misc.ErrSnapshotNotFound, misc.ErrNoSuchChunk,
			misc.ErrTaskNotFound:
			return status.Errorf(codes.NotFound, e.JSON())

		case misc.ErrChunkSizeTooBig, misc.ErrSmallChunk,
			misc.ErrInvalidSize, misc.ErrInvalidOffset,
			ErrInvalidCoord, misc.ErrInvalidFormat, misc.ErrTooFragmented,
			misc.ErrInvalidBase,
			misc.ErrSortingInvalidField, misc.ErrSortingDuplicateField,
			ErrInvalidLimit, ErrCursorAndSortLimit, ErrSortWithoutLimit,
			ErrInvalidChange, misc.ErrInvalidName, misc.ErrDuplicateName,
			misc.ErrInvalidProgress, misc.ErrUnknownSource,
			misc.ErrSourceNoRange, misc.ErrUnreachableSource,
			misc.ErrInvalidObject, misc.ErrDuplicateID,
			misc.ErrInvalidBlockSize, misc.ErrInvalidNbsClusterID,
			misc.ErrInvalidMoveSrc, misc.ErrInvalidMoveDst, misc.ErrDenyRedirect,
			misc.ErrSourceChanged, misc.ErrSourceURLNotFound, misc.ErrSourceURLAccessDenied, misc.ErrIncorrectMap:
			return status.Errorf(codes.InvalidArgument, e.JSON())

		case misc.ErrSnapshotNotReady, misc.ErrSnapshotReadOnly,
			misc.ErrDifferentSize, misc.ErrSnapshotNotFull,
			misc.ErrSnapshotLocked, misc.ErrAlreadyLocked:
			return status.Errorf(codes.FailedPrecondition, e.JSON())

		case misc.ErrSnapshotCorrupted, misc.ErrClientNil,
			misc.ErrUploadInfoNil, misc.ErrDuplicateKey,
			misc.ErrInvalidChange, misc.ErrTooLongOuput, misc.ErrCorruptedSource:
			return status.Errorf(codes.Internal, e.JSON())

		case misc.ErrDuplicateChunk, misc.ErrDuplicateTaskID:
			return status.Errorf(codes.AlreadyExists, e.JSON())
		}
	case *client.ClientError:
		metadata, err := json.Marshal(e)
		if err == nil {
			return status.Errorf(codes.Internal, common.NewError("ErrNbs", string(metadata), false).JSON())
		}
	default:
		switch e {
		case nil:
			return nil
		case context.Canceled, stdctx.Canceled:
			return status.Errorf(codes.Canceled, ErrOperationCanceled.JSON())
		case context.DeadlineExceeded, stdctx.DeadlineExceeded:
			return status.Errorf(codes.DeadlineExceeded, ErrDeadlineExceeded.JSON())
		default:
			if strings.HasPrefix(e.Error(), "timestamp: ") {
				return status.Errorf(codes.InvalidArgument, ErrInvalidTimestamp.JSON())
			}
		}
	}

	return status.Errorf(codes.Internal, common.NewError("ErrUnknown", e.Error(), false).JSON())
}

func httpError(ctx context.Context, mux *runtime.ServeMux, marshaler runtime.Marshaler, w http.ResponseWriter, r *http.Request, err error) {
	runtime.DefaultHTTPError(ctx, mux, customErrorMarshaler{marshaler}, w, r, err)
}

func getErrorCode(grpcCode int) string {
	return "Err" + codes.Code(grpcCode).String()
}

func getDescription(v interface{}) (desc []byte, err error) {
	defer func() {
		if rec := recover(); rec != nil {
			zap.L().Error("Failed to marshal error", zap.Any("error", v), zap.Any("panic", rec))
			err = fmt.Errorf("failed to marshal error")
		}
	}()
	elem := reflect.ValueOf(v).Elem()
	s := elem.FieldByName("Error").String()
	b := []byte(s)
	if err := json.Unmarshal(b, &common.SnapshotError{}); err == nil {
		return b, nil
	}
	code := int(elem.FieldByName("Code").Int())
	zap.L().Error("HTTP/GRPC internal error", zap.Any("error", v), zap.Error(err))
	return []byte(common.NewError(getErrorCode(code), s, false).JSON()), nil
}

// Implements custom error marshaling.
// We fill response payload with error description.
type customErrorMarshaler struct {
	runtime.Marshaler
}

func (m customErrorMarshaler) Marshal(v interface{}) ([]byte, error) {
	desc, err := getDescription(v)
	if err != nil {
		return m.Marshaler.Marshal(v)
	}
	return desc, nil
}

func (m customErrorMarshaler) NewEncoder(w io.Writer) runtime.Encoder {
	return &customErrorEncoder{
		Encoder: m.Marshaler.NewEncoder(w),
		Writer:  w,
	}
}

type customErrorEncoder struct {
	runtime.Encoder
	io.Writer
}

func (e customErrorEncoder) Encode(v interface{}) error {
	desc, err := getDescription(v)
	if err != nil {
		return e.Encoder.Encode(v)
	}
	_, err = e.Write(desc)
	return err
}
