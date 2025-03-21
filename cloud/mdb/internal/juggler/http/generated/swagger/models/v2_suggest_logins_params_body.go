// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"

	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
)

// V2SuggestLoginsParamsBody v2 suggest logins params body
//
// swagger:model v2SuggestLoginsParamsBody
type V2SuggestLoginsParamsBody struct {

	// limit
	Limit int64 `json:"limit,omitempty"`

	// query
	Query string `json:"query,omitempty"`

	// type
	// Enum: [staff_users notification_recipients notification_owners namespace_owners dashboard_owners]
	Type interface{} `json:"type,omitempty"`
}

// Validate validates this v2 suggest logins params body
func (m *V2SuggestLoginsParamsBody) Validate(formats strfmt.Registry) error {
	return nil
}

// ContextValidate validates this v2 suggest logins params body based on context it is used
func (m *V2SuggestLoginsParamsBody) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *V2SuggestLoginsParamsBody) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2SuggestLoginsParamsBody) UnmarshalBinary(b []byte) error {
	var res V2SuggestLoginsParamsBody
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
