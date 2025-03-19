package services

import (
	"context"
	"errors"
	"strings"

	"google.golang.org/grpc/metadata"
)

const (
	BearerPrefix = "Bearer "
)

var (
	errNoAuthMetaData       = errors.New("no authorization metadata provided")
	errMultipleAuthMetaData = errors.New("multiple authorization metadata provided")
	errInvalidAuthMetaData  = errors.New("invalid authorization metadata")
)

func getToken(ctx context.Context) (string, error) {
	metaData, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return "", errNoAuthMetaData
	}

	tokens := metaData.Get("authorization")

	if len(tokens) == 0 {
		return "", errNoAuthMetaData
	}
	if len(tokens) > 1 {
		return "", errMultipleAuthMetaData
	}
	if !strings.HasPrefix(tokens[0], BearerPrefix) {
		return "", errInvalidAuthMetaData
	}

	return strings.TrimPrefix(tokens[0], BearerPrefix), nil
}
