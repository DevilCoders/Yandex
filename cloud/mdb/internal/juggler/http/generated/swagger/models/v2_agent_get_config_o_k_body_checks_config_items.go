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

// V2AgentGetConfigOKBodyChecksConfigItems v2 agent get config o k body checks config items
//
// swagger:model v2AgentGetConfigOKBodyChecksConfigItems
type V2AgentGetConfigOKBodyChecksConfigItems struct {

	// checks
	Checks []*V2AgentGetConfigOKBodyChecksConfigItemsChecksItems `json:"checks"`

	// host name
	HostName string `json:"host_name,omitempty"`
}

// Validate validates this v2 agent get config o k body checks config items
func (m *V2AgentGetConfigOKBodyChecksConfigItems) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateChecks(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *V2AgentGetConfigOKBodyChecksConfigItems) validateChecks(formats strfmt.Registry) error {
	if swag.IsZero(m.Checks) { // not required
		return nil
	}

	for i := 0; i < len(m.Checks); i++ {
		if swag.IsZero(m.Checks[i]) { // not required
			continue
		}

		if m.Checks[i] != nil {
			if err := m.Checks[i].Validate(formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("checks" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// ContextValidate validate this v2 agent get config o k body checks config items based on the context it is used
func (m *V2AgentGetConfigOKBodyChecksConfigItems) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateChecks(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *V2AgentGetConfigOKBodyChecksConfigItems) contextValidateChecks(ctx context.Context, formats strfmt.Registry) error {

	for i := 0; i < len(m.Checks); i++ {

		if m.Checks[i] != nil {
			if err := m.Checks[i].ContextValidate(ctx, formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("checks" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// MarshalBinary interface implementation
func (m *V2AgentGetConfigOKBodyChecksConfigItems) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2AgentGetConfigOKBodyChecksConfigItems) UnmarshalBinary(b []byte) error {
	var res V2AgentGetConfigOKBodyChecksConfigItems
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
