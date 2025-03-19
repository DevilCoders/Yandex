package sqlutil

type queryContext struct {
	ErrorWrapper ErrorWrapperFunc
}

type QueryOption func(qc queryContext) queryContext

func WithErrorWrapper(ew ErrorWrapperFunc) QueryOption {
	return func(qc queryContext) queryContext {
		qc.ErrorWrapper = ew
		return qc
	}
}
