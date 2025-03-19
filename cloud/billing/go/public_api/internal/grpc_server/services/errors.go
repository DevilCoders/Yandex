package services

import (
	"errors"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
)

var errInternal = status.Error(codes.Internal, "internal error")

func parseError(err error) error {
	msg := getResponseErrMessage(err)
	switch {
	case errors.Is(err, console.ErrInvalidRequest):
		return status.Error(codes.InvalidArgument, msg)
	case errors.Is(err, console.ErrUnauthenticated):
		return status.Error(codes.Unauthenticated, msg)
	case errors.Is(err, console.ErrPermissionDenied):
		return status.Error(codes.PermissionDenied, msg)
	case errors.Is(err, console.ErrNotFound):
		return status.Error(codes.NotFound, msg)
	case errors.Is(err, console.ErrResourceExhausted):
		return status.Error(codes.ResourceExhausted, msg)
	case msg != "":
		return status.Error(codes.Internal, msg)
	}
	return errInternal
}

func getResponseErrMessage(err error) string {
	var responseErr *console.ResponseErr
	if errors.As(err, &responseErr) {
		return responseErr.Message
	}
	return ""
}
