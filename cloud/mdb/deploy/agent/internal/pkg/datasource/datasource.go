package datasource

import (
	"context"
	"io"
)

//go:generate ../../../../../scripts/mockgen.sh DataSource

type NamedReadCloser interface {
	io.ReadCloser
	Name() string
}

// DataSource provides API to access /srv versions and data
type DataSource interface {
	LatestVersion(ctx context.Context) (string, error)
	ResolveVersion(ctx context.Context, version string) (string, error)
	Fetch(ctx context.Context, version string) (NamedReadCloser, error)
}
