package rewriters

import (
	"io"
	"net/http"
)

type Rewriter interface {
	Rewrite(header http.Header, body io.Reader) (io.Reader, error)
	ApplyCSP() bool
}
