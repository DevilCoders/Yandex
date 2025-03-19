package access

import (
	"context"

	"golang.org/x/xerrors"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

var (
	// ErrMalformedToken empty or malformed token.
	ErrMalformedToken = xerrors.New("malformed token")

	ErrMissingAuthToken = xerrors.New("auth token not provided")

	ErrUnauthenticated = xerrors.New("unauthenticated")

	ErrPermissionDenied = xerrors.New("permission denied")

	ErrInternalError = xerrors.New("internal error")

	ErrInvalidArgument = xerrors.New("invalid argument")
)

func (c *Client) mapGRPCErr(ctx context.Context, err error) error {
	scopedLogger := ctxtools.Logger(ctx)

	if err == nil {
		scopedLogger.Debug("grpc request has been completed")
		return nil
	}

	scopedLogger.Error("failed to make grpc request", log.Error(err))

	if xerrors.Is(err, context.DeadlineExceeded) || xerrors.Is(err, context.Canceled) {
		return err
	}

	st := status.Convert(err)

	switch st.Code() {
	case codes.OK:
		return nil
	case codes.Unauthenticated:
		return ErrUnauthenticated
	case codes.PermissionDenied:
		return ErrPermissionDenied
	case codes.InvalidArgument:
		return ErrInvalidArgument
	}

	return ErrInternalError
}
