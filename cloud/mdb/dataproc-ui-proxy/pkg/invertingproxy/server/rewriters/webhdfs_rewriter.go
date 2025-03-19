package rewriters

import (
	"io"
	"net/http"
)

type WebHDFSRewriter struct {
	GenericRewriter
}

func (whr *WebHDFSRewriter) Rewrite(headers http.Header, body io.Reader) (io.Reader, error) {
	requestHeaders := whr.Request.Header
	if requestHeaders.Get("Origin") != "" && requestHeaders.Get("Cookie") != "" {
		headers.Set("Access-Control-Allow-Origin", requestHeaders.Get("Origin"))
		headers.Set("Access-Control-Allow-Credentials", "true")
	}
	return whr.GenericRewriter.Rewrite(headers, body)
}
