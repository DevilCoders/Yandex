package valid

// Validator declares basic validation interface
type Validator interface {
	Validate() error
}
