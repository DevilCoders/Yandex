package errtool

import (
	"context"
	"errors"
	"fmt"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/billing/go/pkg/errsentinel"
)

var (
	grpcCanceled         = status.FromContextError(context.Canceled).Err()
	grpcDeadlineExceeded = status.FromContextError(context.DeadlineExceeded).Err()
)

// ContextOrGRPCError check if error returned from GRPC call caused by external context cancelation.
func ContextOrGRPCError(ctx context.Context, err error) (bool, error) {
	switch {
	case errors.Is(err, context.DeadlineExceeded) || errors.Is(err, context.Canceled):
		return true, err
	case errors.Is(err, grpcCanceled) && errors.Is(ctx.Err(), context.Canceled):
		return true, ctx.Err()
	case errors.Is(err, grpcDeadlineExceeded) && errors.Is(ctx.Err(), context.DeadlineExceeded):
		return true, ctx.Err()
	}
	return false, err
}

var (
	ErrInvalidArgument  = errsentinel.New("invalid request argument")
	ErrNotFound         = errsentinel.New("requested entity not found")
	ErrPermissionDenied = errsentinel.New("client has no permission for this call")
	ErrUnavailable      = errsentinel.New("service is currently unavailable")
	ErrInternalError    = errsentinel.New("service is broken")
)

func MapGRPCErr(ctx context.Context, err error) error {
	if err == nil {
		return nil
	}
	if ok, ctxErr := ContextOrGRPCError(ctx, err); ok {
		return ctxErr
	}

	st, ok := status.FromError(err)
	if !ok {
		return err
	}
	switch st.Code() {
	case codes.OK:
		return nil
	case codes.InvalidArgument:
		return ErrInvalidArgument.Wrap(err)
	case codes.NotFound:
		return ErrNotFound.Wrap(err)
	case codes.PermissionDenied:
		return ErrPermissionDenied.Wrap(err)
	case codes.Unavailable:
		return ErrUnavailable.Wrap(err)
	}

	return ErrInternalError.Wrap(err)
}

type OperationError struct {
	Code    int32
	Message string
}

func (e OperationError) Error() string {
	return fmt.Sprintf("operation error (%d): %s", e.Code, e.Message)
}
