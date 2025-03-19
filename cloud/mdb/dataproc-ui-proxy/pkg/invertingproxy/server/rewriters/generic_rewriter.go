package rewriters

import (
	"bytes"
	"compress/gzip"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"regexp"
	"strconv"
	"strings"

	"a.yandex-team.ru/library/go/core/log"
)

type GenericRewriter struct {
	Request         *http.Request
	ClusterID       string
	DataplaneDomain string
	UIProxyDomain   string
	logger          log.Logger
}

func (gr *GenericRewriter) ApplyCSP() bool {
	// By default we always apply default Content Security Policy.
	return true
}

func (gr *GenericRewriter) Rewrite(headers http.Header, body io.Reader) (io.Reader, error) {
	gr.rewriteHeaders(headers)
	return gr.rewriteBody(headers, body)
}

func (gr *GenericRewriter) rewriteHeaders(headers http.Header) {
	if headers.Get("Location") != "" {
		headers.Set("Location", gr.rewriteURLs(headers.Get("Location")))
	}
}

func (gr *GenericRewriter) rewriteBody(headers http.Header, body io.Reader) (io.Reader, error) {
	contentTypes := []string{"text/html", "application/json"}
	return gr.handleEncodingAndContentType(headers, body, contentTypes, func(body string) (string, error) {
		body = gr.rewriteURLs(body)

		// Zeppelin shows links to Spark UI of the form
		// https://ui-e4u5hiivcemf9o5fllva-rc1a-dataproc-d-bl490jo3dx52b7rh-38387.dataproc-ui.cloud-preprod.yandex.net/jobs/job/?id=19
		// and this page contains invalid asset URLs, fix them
		if strings.HasPrefix(gr.Request.URL.Path, "/jobs/") {
			r := regexp.MustCompile("\"/proxy/application_\\d+_\\d+/static/(.*?)\"")
			body = r.ReplaceAllString(body, "\"/static/$1\"")
		}

		return body, nil
	})
}

func (gr *GenericRewriter) rewriteURLs(content string) string {
	r := regexp.MustCompile(fmt.Sprintf("(?:https?:)?//([a-zA-Z0-9-]+).%s:?(\\d+)?", gr.DataplaneDomain))
	url := fmt.Sprintf("https://ui-%s-$1-$2.%s", gr.ClusterID, gr.UIProxyDomain)
	return r.ReplaceAllString(content, url)
}

func (gr *GenericRewriter) handleEncodingAndContentType(headers http.Header, responseBody io.Reader, contentTypes []string, rewrite func(body string) (string, error)) (io.Reader, error) {
	allowedContentType := false
	for _, contentType := range contentTypes {
		if strings.Contains(headers.Get("Content-Type"), contentType) {
			allowedContentType = true
		}
	}
	if !allowedContentType {
		return responseBody, nil
	}

	bodyReader := ioutil.NopCloser(responseBody)

	useGzip := headers.Get("Content-Encoding") == "gzip"
	if useGzip {
		var err error
		bodyReader, err = gzip.NewReader(responseBody)
		if err != nil {
			return nil, err
		}
		defer func() {
			if err := bodyReader.Close(); err != nil {
				gr.logger.Errorf("failed to close gzip reader: %s", err)
			}
		}()
	}

	buffer := bytes.Buffer{}
	if _, err := buffer.ReadFrom(bodyReader); err != nil {
		return nil, err
	}

	rewritten, err := rewrite(buffer.String())
	if err != nil {
		return nil, err
	}

	rewrittenBody := bytes.NewBufferString(rewritten)
	if useGzip {
		compressed := &bytes.Buffer{}
		uncompressed := gzip.NewWriter(compressed)
		if _, err := uncompressed.Write(rewrittenBody.Bytes()); err != nil {
			// for whatever reason this might happen we may handle this situation: just return uncompressed body
			headers.Del("Content-Encoding")
		} else {
			rewrittenBody = compressed
		}
		if err := uncompressed.Close(); err != nil {
			gr.logger.Errorf("Failed to close gzip writer: %s", err)
		}
	}

	// fix Content-Length header only if was set in initial upstream response
	if headers.Get("Content-Length") != "" {
		headers.Set("Content-Length", strconv.Itoa(rewrittenBody.Len()))
	}

	return rewrittenBody, nil
}
