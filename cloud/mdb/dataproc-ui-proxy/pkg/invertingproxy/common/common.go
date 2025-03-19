package common

const (

	// HeaderAgentID is the name of a request header used to uniquely identify this agent.
	HeaderAgentID = "X-Inverting-Proxy-Agent-ID"

	// HeaderRequestID is the name of a request/response header used to uniquely
	// identify a proxied request.
	HeaderRequestID = "X-Inverting-Proxy-Request-ID"

	// HeaderRequestStartTime is the name of a response header used by the proxy
	// to report the start time of a proxied request.
	HeaderRequestStartTime = "X-Inverting-Proxy-Request-Start-Time"

	// HeaderUpstreamHost is an address within Data Proc cluster to send proxied request to.
	// Format is "host:port".
	HeaderUpstreamHost = "X-Inverting-Proxy-Upstream-Host"
)

// Hop-by-hop headers. These are removed when sent to the backend.
// As of RFC 7230, hop-by-hop headers are required to appear in the
// Connection header field. These are the headers defined by the
// obsoleted RFC 2616 (section 13.5.1) and are used for backward
// compatibility.
var HopHeaders = []string{
	"Connection",
	"Proxy-Connection", // non-standard but still sent by libcurl and rejected by e.g. google
	"Keep-Alive",
	"Proxy-Authenticate",
	"Proxy-Authorization",
	"Te",      // canonicalized version of "TE"
	"Trailer", // not Trailers per URL above; https://www.rfc-editor.org/errata_search.php?eid=4522
	"Transfer-Encoding",
	"Upgrade",
}
