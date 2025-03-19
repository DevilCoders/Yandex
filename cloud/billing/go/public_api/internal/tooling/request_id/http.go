package requestid

import "github.com/go-resty/resty/v2"

var (
	requestIDHeaderName  = "X-Request-ID"
	requestUIDHeaderName = "X-Request-UID"
)

func InjectHTTP(requestID string, request *resty.Request) {
	request.SetHeader(requestIDHeaderName, requestID)
	request.SetHeader(requestUIDHeaderName, generateRequestID()) // distinguish different requests inside one user's request-id scope (retries or different endpoints)
}
