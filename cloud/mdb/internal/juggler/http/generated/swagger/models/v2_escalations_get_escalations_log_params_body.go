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

// V2EscalationsGetEscalationsLogParamsBody v2 escalations get escalations log params body
//
// swagger:model v2EscalationsGetEscalationsLogParamsBody
type V2EscalationsGetEscalationsLogParamsBody struct {

	// filters
	Filters []*V2EscalationsGetEscalationsLogParamsBodyFiltersItems `json:"filters"`

	// only running
	OnlyRunning bool `json:"only_running,omitempty"`

	// page
	Page int64 `json:"page,omitempty"`

	// page size
	PageSize int64 `json:"page_size,omitempty"`
}

// Validate validates this v2 escalations get escalations log params body
func (m *V2EscalationsGetEscalationsLogParamsBody) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateFilters(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *V2EscalationsGetEscalationsLogParamsBody) validateFilters(formats strfmt.Registry) error {
	if swag.IsZero(m.Filters) { // not required
		return nil
	}

	for i := 0; i < len(m.Filters); i++ {
		if swag.IsZero(m.Filters[i]) { // not required
			continue
		}

		if m.Filters[i] != nil {
			if err := m.Filters[i].Validate(formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("filters" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// ContextValidate validate this v2 escalations get escalations log params body based on the context it is used
func (m *V2EscalationsGetEscalationsLogParamsBody) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateFilters(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *V2EscalationsGetEscalationsLogParamsBody) contextValidateFilters(ctx context.Context, formats strfmt.Registry) error {

	for i := 0; i < len(m.Filters); i++ {

		if m.Filters[i] != nil {
			if err := m.Filters[i].ContextValidate(ctx, formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("filters" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// MarshalBinary interface implementation
func (m *V2EscalationsGetEscalationsLogParamsBody) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2EscalationsGetEscalationsLogParamsBody) UnmarshalBinary(b []byte) error {
	var res V2EscalationsGetEscalationsLogParamsBody
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
