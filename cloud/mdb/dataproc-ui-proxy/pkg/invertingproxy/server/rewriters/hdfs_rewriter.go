package rewriters

import (
	"io"
	"net/http"
	"strings"
)

type HDFSRewriter struct {
	GenericRewriter
}

func (hr *HDFSRewriter) Rewrite(headers http.Header, body io.Reader) (io.Reader, error) {
	hr.rewriteHeaders(headers)
	contentTypes := []string{"text/html", "application/javascript", "application/x-javascript", "application/json"}
	return hr.handleEncodingAndContentType(headers, body, contentTypes, func(body string) (string, error) {
		body = hr.rewriteURLs(body)

		// HDFS explorer is hosted on master node and sends XHR requests to data nodes.
		// By default (withCredentials==false) such requests are sent without cookies.
		// But UI Proxy needs cookies in order to authenticate request and pass it to the cluster.
		// See also https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest/withCredentials
		if hr.Request.URL.Path == "/explorer.js" {
			body = strings.ReplaceAll(body, "$.ajax({", "$.ajax({\n"+
				"xhrFields: {\n           withCredentials: true\n      },")
		}

		// Fix datanode link
		if hr.Request.URL.Path == "/dfshealth.js" {
			body = strings.ReplaceAll(body,
				"n.dnWebAddress = \"http://\" + dnHost + \":\" + port;",
				"n.dnWebAddress = \"https://ui-"+hr.ClusterID+"-\" + dnHost.split(\".\")[0] + \"-\" + port + \"."+hr.UIProxyDomain+"\";")
		}

		return body, nil
	})
}

func (hr *HDFSRewriter) ApplyCSP() bool {
	// HDFS explorer makes Cross-Site requests which are not allowed by our default Content Security Policy.
	// So disable CSP on explorer page.
	return hr.Request.URL.Path != "/explorer.html"
}
