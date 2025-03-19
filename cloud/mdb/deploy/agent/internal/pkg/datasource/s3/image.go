package s3

import (
	"io"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/datasource"
)

type image struct {
	obj  io.ReadCloser
	name string
}

var _ datasource.NamedReadCloser = &image{}

func (o *image) Read(p []byte) (n int, err error) {
	return o.obj.Read(p)
}

func (o *image) Close() error {
	return o.obj.Close()
}

func (o *image) Name() string {
	return o.name
}
