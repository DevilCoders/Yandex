package status

type CodeGroup int

const (
	Unknown       CodeGroup = iota
	Informational           // 1xx
	Successful              // 2xx
	Redirection             // 3xx
	ClientError             // 4xx
	ServerError             // 5xx
)

// GetCodeGroup returns corresponding HTTP code group for given status code.
// See: https://tools.ietf.org/html/rfc7231#page-47
func GetCodeGroup(statusCode int) CodeGroup {
	g := CodeGroup(statusCode / 100)
	if g < Unknown || g > ServerError {
		return Unknown
	}
	return g
}
