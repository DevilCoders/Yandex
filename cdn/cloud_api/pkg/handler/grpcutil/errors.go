package grpcutil

import (
	"fmt"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
)

func WrapValidateError(err error) error {
	return status.Error(codes.InvalidArgument, fmt.Sprintf("validate request: %+v", err))
}

func WrapMapperError(msg string, err error) error {
	return status.Error(codes.InvalidArgument, fmt.Sprintf("%s: %v", msg, err))
}

func WrapAuthError(err error) error {
	return status.Error(codes.PermissionDenied, fmt.Sprintf("failed authorization request: %v", err))
}

func ValidationError(msg string) error {
	return status.Error(codes.InvalidArgument, msg)
}

func NotFoundError(msg string) error {
	return status.Error(codes.NotFound, msg)
}

func WrapInternalError(msg string, err error) error {
	return status.Error(codes.Internal, fmt.Sprintf("%s: %v", msg, err))
}

func ExtractErrorResult(msg string, errorResult errors.ErrorResult) error {
	var code codes.Code
	switch errorResult.Status() {
	case errors.ValidationError:
		code = codes.InvalidArgument
	case errors.AuthorizationError, errors.ForbiddenError:
		code = codes.PermissionDenied
	case errors.NotFoundError:
		code = codes.NotFound
	case errors.InternalError, errors.FatalError:
		code = codes.Internal
	default:
		code = codes.Internal
	}

	return status.Error(code, fmt.Sprintf("%s: %v", msg, errorResult.Error()))
}
