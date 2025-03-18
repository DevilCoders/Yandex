package requestid

type MiddlewareOption = func(*middleware)

// WithHeaders sets list of headers to check for request id.
func WithHeaders(headers []string) MiddlewareOption {
	return func(m *middleware) {
		m.headers = headers
	}
}

// WithRequestIDGenerator sets custom hostname in request id.
func WithRequestIDGenerator(generator func(oldRequestID string) string) MiddlewareOption {
	return func(m *middleware) {
		m.generator = generator
	}
}
