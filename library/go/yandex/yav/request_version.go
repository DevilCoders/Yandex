package yav

type CreateVersionRequest struct {
	Comment string  `json:"comment,omitempty"`
	Values  []Value `json:"value,omitempty"`
}
