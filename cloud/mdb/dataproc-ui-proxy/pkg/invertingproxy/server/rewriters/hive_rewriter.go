package rewriters

import (
	"io"
	"net/http"
)

type HiveRewriter struct {
	GenericRewriter
}

func (hr *HiveRewriter) Rewrite(headers http.Header, body io.Reader) (io.Reader, error) {
	// see https://st.yandex-team.ru/MDB-13955 and https://st.yandex-team.ru/CLOUDSUPPORT-86655
	if headers.Get("Content-Type") == "" {
		headers.Set("Content-Type", "text/html")
	}
	return hr.GenericRewriter.Rewrite(headers, body)
}
