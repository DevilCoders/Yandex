// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"
	"strconv"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
)

// UAAvailability Array of clusters/shards which are readable or writable and its read/write counts.
//
// swagger:model UAAvailability
type UAAvailability []*UAAvailabilityItems0

// Validate validates this u a availability
func (m UAAvailability) Validate(formats strfmt.Registry) error {
	var res []error

	for i := 0; i < len(m); i++ {
		if swag.IsZero(m[i]) { // not required
			continue
		}

		if m[i] != nil {
			if err := m[i].Validate(formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName(strconv.Itoa(i))
				}
				return err
			}
		}

	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

// ContextValidate validate this u a availability based on the context it is used
func (m UAAvailability) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	for i := 0; i < len(m); i++ {

		if m[i] != nil {
			if err := m[i].ContextValidate(ctx, formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName(strconv.Itoa(i))
				}
				return err
			}
		}

	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

// UAAvailabilityItems0 u a availability items0
//
// swagger:model UAAvailabilityItems0
type UAAvailabilityItems0 struct {

	// count
	Count int64 `json:"count"`

	// examples
	Examples []string `json:"examples"`

	// no read count
	NoReadCount int64 `json:"no_read_count"`

	// no write count
	NoWriteCount int64 `json:"no_write_count"`

	// readable
	Readable bool `json:"readable"`

	// userfault broken
	UserfaultBroken bool `json:"userfaultBroken"`

	// writable
	Writable bool `json:"writable"`
}

// Validate validates this u a availability items0
func (m *UAAvailabilityItems0) Validate(formats strfmt.Registry) error {
	return nil
}

// ContextValidate validates this u a availability items0 based on context it is used
func (m *UAAvailabilityItems0) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *UAAvailabilityItems0) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *UAAvailabilityItems0) UnmarshalBinary(b []byte) error {
	var res UAAvailabilityItems0
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
