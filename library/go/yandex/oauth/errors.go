package oauth

type ServerError struct {
	message string
}

func (e *ServerError) Error() string {
	return e.message
}
