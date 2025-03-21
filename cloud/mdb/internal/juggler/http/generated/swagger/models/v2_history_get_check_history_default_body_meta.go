// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"

	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
)

// V2HistoryGetCheckHistoryDefaultBodyMeta v2 history get check history default body meta
//
// swagger:model v2HistoryGetCheckHistoryDefaultBodyMeta
type V2HistoryGetCheckHistoryDefaultBodyMeta struct {

	// backend
	Backend string `json:"_backend,omitempty"`
}

// Validate validates this v2 history get check history default body meta
func (m *V2HistoryGetCheckHistoryDefaultBodyMeta) Validate(formats strfmt.Registry) error {
	return nil
}

// ContextValidate validates this v2 history get check history default body meta based on context it is used
func (m *V2HistoryGetCheckHistoryDefaultBodyMeta) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *V2HistoryGetCheckHistoryDefaultBodyMeta) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2HistoryGetCheckHistoryDefaultBodyMeta) UnmarshalBinary(b []byte) error {
	var res V2HistoryGetCheckHistoryDefaultBodyMeta
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
