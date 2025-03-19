package writer

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
)

//go:generate ../../../../scripts/mockgen.sh Writer

type Writer interface {
	// Write data to stream, block until written.
	Write(docs []writer.Doc, timeout time.Duration) error
}
