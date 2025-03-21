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

// MinionChangeList minion change list
//
// swagger:model MinionChangeList
type MinionChangeList struct {

	// masters
	Masters []*MinionChange `json:"masters"`
}

// Validate validates this minion change list
func (m *MinionChangeList) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateMasters(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *MinionChangeList) validateMasters(formats strfmt.Registry) error {
	if swag.IsZero(m.Masters) { // not required
		return nil
	}

	for i := 0; i < len(m.Masters); i++ {
		if swag.IsZero(m.Masters[i]) { // not required
			continue
		}

		if m.Masters[i] != nil {
			if err := m.Masters[i].Validate(formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("masters" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// ContextValidate validate this minion change list based on the context it is used
func (m *MinionChangeList) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateMasters(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *MinionChangeList) contextValidateMasters(ctx context.Context, formats strfmt.Registry) error {

	for i := 0; i < len(m.Masters); i++ {

		if m.Masters[i] != nil {
			if err := m.Masters[i].ContextValidate(ctx, formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("masters" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// MarshalBinary interface implementation
func (m *MinionChangeList) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *MinionChangeList) UnmarshalBinary(b []byte) error {
	var res MinionChangeList
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
