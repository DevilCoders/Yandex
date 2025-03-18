package unknownjson

import "encoding/json"

// Store is special type for storing unknown fields.
//
// Unknown fields stored in this field are accessible only to the unknownjson package.
// This forces users to extend typed part of the structure, instead of using ",store" field.
type Store struct {
	m map[string]json.RawMessage
}
