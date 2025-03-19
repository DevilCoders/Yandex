package grpcerr

import (
	"context"
	"fmt"
	"sync"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/any"
	"google.golang.org/genproto/googleapis/rpc/errdetails"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func SemanticErrorToGRPC(s semerr.Semantic) codes.Code {
	switch s {
	case semerr.SemanticInvalidInput:
		return codes.InvalidArgument
	case semerr.SemanticAuthentication:
		return codes.Unauthenticated
	case semerr.SemanticAuthorization:
		return codes.PermissionDenied
	case semerr.SemanticNotFound:
		return codes.NotFound
	case semerr.SemanticFailedPrecondition:
		return codes.FailedPrecondition
	case semerr.SemanticAlreadyExists:
		return codes.AlreadyExists
	case semerr.SemanticNotImplemented:
		return codes.Unimplemented
	case semerr.SemanticInternal:
		return codes.Internal
	case semerr.SemanticUnavailable:
		return codes.Unavailable
	case semerr.SemanticUnknown:
		fallthrough
	default:
		return codes.Unknown
	}
}

//nolint:ST1008
func SemanticErrorFromGRPC(err error) error {
	s, ok := status.FromError(err)
	if !ok {
		return err
	}

	switch s.Code() {
	case codes.Canceled:
		err = xerrors.Errorf("%s: %w", s.Message(), context.Canceled)
	//case codes.Unknown: // no need to convert unknown error, non-semerr errors are 'unknown' by default
	case codes.InvalidArgument:
		err = semerr.InvalidInput(s.Message())
	case codes.DeadlineExceeded:
		err = xerrors.Errorf("%s: %w", s.Message(), context.DeadlineExceeded)
	case codes.NotFound:
		err = semerr.NotFound(s.Message())
	case codes.AlreadyExists:
		err = semerr.AlreadyExists(s.Message())
	case codes.PermissionDenied:
		err = semerr.Authorization(s.Message())
	//case codes.ResourceExhausted:
	case codes.FailedPrecondition:
		err = semerr.FailedPrecondition(s.Message())
	//case codes.Aborted:
	//case codes.OutOfRange:
	case codes.Unimplemented:
		err = semerr.NotImplemented(s.Message())
	case codes.Internal:
		err = semerr.Internal(s.Message())
	case codes.Unavailable:
		err = semerr.Unavailable(s.Message())
	//case codes.DataLoss:
	case codes.Unauthenticated:
		err = semerr.Authentication(s.Message())
	}

	return err
}

func ErrorToGRPC(e error, exposeDebug bool, l log.Logger) *status.Status {
	se := semerr.AsSemanticError(e)
	if se == nil {
		l.Warnf("no semantic error found when translating to gRPC error: %+v", e)

		// Set default code and message
		code := codes.Unknown
		msg := "Unknown error"

		if xerrors.Is(e, context.DeadlineExceeded) {
			// Context timeout, report it as such
			code = codes.DeadlineExceeded
			msg = "Deadline exceeded"
		} else if xerrors.Is(e, context.Canceled) {
			// Context cancellation, report it as such
			code = codes.Canceled
			msg = "Operation canceled"
		}

		if !exposeDebug {
			// Do not expose anything for non-semantic error
			return status.New(code, msg)
		}

		// Expose everything
		return makeGRPCStatus(e, code, e.Error(), nil, true, l)
	}

	// Always expose semantic error message, expose details based on flag
	return makeGRPCStatus(e, SemanticErrorToGRPC(se.Semantic), se.Message, se.Details, exposeDebug, l)
}

func makeGRPCStatus(e error, code codes.Code, msg string, details interface{}, exposeDebug bool, l log.Logger) *status.Status {
	st := status.New(code, msg)

	var det []proto.Message
	if details != nil {
		// TODO: Remove this magic and uncomment code below when UI will be able to handle standard typeurl
		// https://st.yandex-team.ru/CLOUDFRONT-4923
		p := st.Proto()
		res, err := convertDetails(details)
		if err != nil {
			l.Warnf("failed to convert error details to gRPC error details: %s", err)
		} else {
			failure, err := proto.Marshal(res)
			if err != nil {
				l.Warnf("failed to marshal error details to gRPC status: %s", err)
			} else {
				p.Details = append(p.Details, &any.Any{
					TypeUrl: "type.private-api.yandex-cloud.ru/quota.QuotaFailure",
					Value:   failure,
				})

				st = status.FromProto(p)
			}
		}
	}

	/*failure, err := ptypes.MarshalAny(res)
	if err != nil {
		l.Warnf("failed to marshal error details to gRPC status: %s", err)
	} else {
		det = append(det, failure)
	}*/

	if exposeDebug {
		// Expose debug (full error message, stack frames, etc)
		det = append(det, &errdetails.DebugInfo{Detail: fmt.Sprintf("%+v", e)})
	}

	detailed, err := st.WithDetails(det...)
	if err != nil {
		l.Warnf("failed to add details to gRPC status: %s", err)
		return st
	}

	return detailed
}

// IsTemporary returns true of code reports temporary error that can be retried
func IsTemporary(code codes.Code) bool {
	return code == codes.Unavailable
}

type ErrorDetailsConverterFunc func(details interface{}) (proto.Message, bool, error)

var (
	errorDetailsConverters   []ErrorDetailsConverterFunc
	errorDetailsConvertersMu sync.Mutex
)

func RegisterErrorDetailsConverter(f ErrorDetailsConverterFunc) {
	errorDetailsConvertersMu.Lock()
	defer errorDetailsConvertersMu.Unlock()
	errorDetailsConverters = append(errorDetailsConverters, f)
}

func convertDetails(details interface{}) (proto.Message, error) {
	errorDetailsConvertersMu.Lock()
	defer errorDetailsConvertersMu.Unlock()
	for _, converter := range errorDetailsConverters {
		res, ok, err := converter(details)
		if err != nil {
			return nil, err
		}

		if ok {
			return res, nil
		}
	}

	return nil, xerrors.Errorf("no converter found for details %+v", details)
}
