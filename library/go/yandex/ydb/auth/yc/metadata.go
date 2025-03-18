package yc

import (
	"github.com/ydb-platform/ydb-go-sdk/v3"
	"github.com/ydb-platform/ydb-go-yc-metadata"
)

func WithMetadataCredentialsURL(url string) ydb.Option {
	return yc.WithCredentials(
		yc.WithURL(url),
	)
}

func WithMetadataCredentials() ydb.Option {
	return yc.WithCredentials()
}

func WithInternalCA() ydb.Option {
	return yc.WithInternalCA()
}
