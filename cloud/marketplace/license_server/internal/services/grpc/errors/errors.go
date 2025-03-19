package errors

import (
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

var GRPCInternalErr = status.Errorf(codes.Internal, "internal error")

func MakeErrInvalidArgument(format string, args ...interface{}) error {
	return status.Errorf(codes.InvalidArgument, format, args...)
}

func MakeErrNotImplemented(format string, args ...interface{}) error {
	return status.Errorf(codes.Unimplemented, format, args...)
}

func MakeErrNotFound(format string, args ...interface{}) error {
	return status.Errorf(codes.NotFound, format, args...)
}

func MakeErrPermissionDenied(format string, args ...interface{}) error {
	return status.Errorf(codes.PermissionDenied, format, args...)
}

func MakeErrUnauthenticated(format string, args ...interface{}) error {
	return status.Errorf(codes.Unauthenticated, format, args...)
}

func MakeErrFailedPrecondition(format string, args ...interface{}) error {
	return status.Errorf(codes.FailedPrecondition, format, args...)
}

func MakeErrAlreadyExists(format string, args ...interface{}) error {
	return status.Errorf(codes.AlreadyExists, format, args...)
}
