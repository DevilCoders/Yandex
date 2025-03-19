package rewriters

import (
	"bytes"
	"compress/gzip"
	"io"
	"net/http"
	"net/http/httptest"
	"strconv"
	"strings"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/log/nop"
)

func rewriteWithGenericRewriter(headers http.Header, body io.Reader) (io.Reader, error) {
	req := httptest.NewRequest("GET", "http://example.com/foo", nil)
	gr := GenericRewriter{
		Request:         req,
		ClusterID:       "foo",
		DataplaneDomain: "dataplane",
		UIProxyDomain:   "dataproc-ui",
		logger:          &nop.Logger{},
	}
	return gr.Rewrite(headers, body)
}

func readerToString(t *testing.T, reader io.Reader) string {
	buf := new(strings.Builder)
	_, err := io.Copy(buf, reader)
	require.NoError(t, err)
	return buf.String()
}

func TestGenericRewriterRewritesBody(t *testing.T) {
	body := bytes.NewBufferString(
		"Link 1 is http://datanode1.dataplane:7777/some/path\n" +
			"Link 2 is http://datanode2.dataplane:8888/another/path")
	headers := http.Header{}
	headers.Set("Content-Type", "text/html")
	headers.Set("Content-Length", strconv.Itoa(body.Len()))
	rewritten, err := rewriteWithGenericRewriter(headers, body)
	require.NoError(t, err)
	expected := "Link 1 is https://ui-foo-datanode1-7777.dataproc-ui/some/path\n" +
		"Link 2 is https://ui-foo-datanode2-8888.dataproc-ui/another/path"
	require.Equal(t, expected, readerToString(t, rewritten))
	require.Equal(t, strconv.Itoa(len(expected)), headers.Get("Content-Length"))
}

func TestGenericRewriterRewritesRedirectLocation(t *testing.T) {
	headers := http.Header{}
	headers.Set("Location", "http://datanode1.dataplane:7777/some/path")
	_, err := rewriteWithGenericRewriter(headers, bytes.NewBufferString(""))
	require.NoError(t, err)
	expected := "https://ui-foo-datanode1-7777.dataproc-ui/some/path"
	require.Equal(t, expected, headers.Get("Location"))
}

func TestGenericRewriterHandlesEncodedResponses(t *testing.T) {
	var buf bytes.Buffer
	zw := gzip.NewWriter(&buf)
	_, err := zw.Write([]byte("Link 1 is http://datanode1.dataplane:7777/some/path"))
	require.NoError(t, err)
	require.NoError(t, zw.Close())

	headers := http.Header{}
	headers.Set("Content-Type", "text/html")
	headers.Set("Content-Encoding", "gzip")
	headers.Set("Content-Length", strconv.Itoa(buf.Len()))

	rewritten, err := rewriteWithGenericRewriter(headers, &buf)
	require.NoError(t, err)

	// copy rewritten in order to calculate length after it will be read by gzip
	var copyOfRewritten bytes.Buffer
	rewritten = io.TeeReader(rewritten, &copyOfRewritten)

	zr, err := gzip.NewReader(rewritten)
	require.NoError(t, err)

	expected := "Link 1 is https://ui-foo-datanode1-7777.dataproc-ui/some/path"
	require.Equal(t, expected, readerToString(t, zr))
	require.NoError(t, zr.Close())
	require.Equal(t, strconv.Itoa(copyOfRewritten.Len()), headers.Get("Content-Length"))
}

func TestGenericRewriterDoesNotRewritesSomeMimeTypes(t *testing.T) {
	headers := http.Header{}
	headers.Set("Content-Type", "application/octet-stream")
	content := "Link in binary file: http://datanode1.dataplane:7777/some/path"
	rewritten, err := rewriteWithGenericRewriter(headers, bytes.NewBufferString(content))
	require.NoError(t, err)
	require.Equal(t, content, readerToString(t, rewritten))
}
