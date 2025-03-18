package mapper

import (
	"fmt"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
)

func MakePBError(msg string, errorResult errors.ErrorResult) error {
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
