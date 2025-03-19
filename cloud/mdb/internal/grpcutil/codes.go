package grpcutil

import (
	"google.golang.org/grpc/codes"

	"a.yandex-team.ru/library/go/core/xerrors"
)

var grpcStatusCodes = map[string]codes.Code{
	`CANCELLED`:           codes.Canceled,
	`UNKNOWN`:             codes.Unknown,
	`INVALID_ARGUMENT`:    codes.InvalidArgument,
	`DEADLINE_EXCEEDED`:   codes.DeadlineExceeded,
	`NOT_FOUND`:           codes.NotFound,
	`ALREADY_EXISTS`:      codes.AlreadyExists,
	`PERMISSION_DENIED`:   codes.PermissionDenied,
	`RESOURCE_EXHAUSTED`:  codes.ResourceExhausted,
	`FAILED_PRECONDITION`: codes.FailedPrecondition,
	`ABORTED`:             codes.Aborted,
	`OUT_OF_RANGE`:        codes.OutOfRange,
	`UNIMPLEMENTED`:       codes.Unimplemented,
	`INTERNAL`:            codes.Internal,
	`UNAVAILABLE`:         codes.Unavailable,
	`DATA_LOSS`:           codes.DataLoss,
	`UNAUTHENTICATED`:     codes.Unauthenticated,
}

func ParseStatusCode(s string) (codes.Code, error) {
	c, ok := grpcStatusCodes[s]
	if !ok {
		return codes.OK, xerrors.Errorf("unknown gRPC status code %q", s)
	}

	return c, nil
}
