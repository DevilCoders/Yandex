// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"

	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
)

// V2MutesGetMutesOKBodyMeta v2 mutes get mutes o k body meta
//
// swagger:model v2MutesGetMutesOKBodyMeta
type V2MutesGetMutesOKBodyMeta struct {

	// backend
	Backend string `json:"_backend,omitempty"`
}

// Validate validates this v2 mutes get mutes o k body meta
func (m *V2MutesGetMutesOKBodyMeta) Validate(formats strfmt.Registry) error {
	return nil
}

// ContextValidate validates this v2 mutes get mutes o k body meta based on context it is used
func (m *V2MutesGetMutesOKBodyMeta) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *V2MutesGetMutesOKBodyMeta) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2MutesGetMutesOKBodyMeta) UnmarshalBinary(b []byte) error {
	var res V2MutesGetMutesOKBodyMeta
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
