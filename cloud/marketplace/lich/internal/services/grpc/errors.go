package grpc

import (
	"context"
	"strings"

	"golang.org/x/xerrors"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/pkg/auth/access-backend"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
)

var grpcInternalErr = status.Errorf(codes.Internal, "internal error")

func makeErrInvalidArgument(format string, args ...interface{}) error {
	return status.Errorf(codes.InvalidArgument, format, args...)
}

func makeErrNotImplemented(format string, args ...interface{}) error {
	return status.Errorf(codes.Unimplemented, format, args...)
}

func makeErrNotFound(format string, args ...interface{}) error {
	return status.Errorf(codes.NotFound, format, args...)
}

func makeErrPermissionDenied(format string, args ...interface{}) error {
	return status.Errorf(codes.PermissionDenied, format, args...)
}

func makeErrUnauthenticated(format string, args ...interface{}) error {
	return status.Errorf(codes.Unauthenticated, format, args...)
}

func makeErrFailedPrecondition(format string, args ...interface{}) error {
	return status.Errorf(codes.FailedPrecondition, format, args...)
}

func makeErrAlreadyExists(format string, args ...interface{}) error {
	return status.Errorf(codes.AlreadyExists, format, args...)
}

func (b *baseService) mapActionError(ctx context.Context, err error) error {
	if err == nil {
		logging.Logger().Debug("action completed without errors")
		return err
	}

	logging.Logger().Error("action failed", log.Error(err))

	var (
		licenseCheckErr actions.ErrLicenseCheck

		statusErr interface {
			GRPCStatus() *status.Status
		}
	)

	switch {
	case xerrors.Is(err, actions.ErrNoCloudID):
		return makeErrInvalidArgument("no cloud id")
	case xerrors.Is(err, actions.ErrLicensedInstancePoolValue):
		return grpcInternalErr
	case xerrors.As(err, &licenseCheckErr):
		return makeErrInvalidArgument("license check error, cloud-id:%s, product-ids:%s",
			licenseCheckErr.CloudID,
			strings.Join(licenseCheckErr.ProductsIDs, ","),
		)
	case xerrors.Is(err, access.ErrUnauthenticated):
		return makeErrUnauthenticated("failed to authenticate requester")
	case xerrors.Is(err, access.ErrMissingAuthToken):
		return makeErrPermissionDenied("request auth credentials should be provided")
	case xerrors.As(err, &statusErr):
		logging.Logger().Error("general grpc error", log.Error(err))
		return statusErr.GRPCStatus().Err()
	case xerrors.Is(err, actions.ErrRMPermissionDenied):
		return makeErrPermissionDenied("permissions denied for services interconnect")
	case xerrors.Is(err, actions.ErrEmptyProductIDs):
		return makeErrInvalidArgument("empty set of product ids")
	default:
		logging.Logger().Error("unclassified internal error", log.Error(err))
		return grpcInternalErr
	}
}
