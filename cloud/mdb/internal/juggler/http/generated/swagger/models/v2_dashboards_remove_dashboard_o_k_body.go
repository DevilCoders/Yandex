// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
)

// V2DashboardsRemoveDashboardOKBody v2 dashboards remove dashboard o k body
//
// swagger:model v2DashboardsRemoveDashboardOKBody
type V2DashboardsRemoveDashboardOKBody struct {

	// dashboard id
	DashboardID string `json:"dashboard_id,omitempty"`

	// meta
	Meta *V2DashboardsRemoveDashboardOKBodyMeta `json:"meta,omitempty"`
}

// Validate validates this v2 dashboards remove dashboard o k body
func (m *V2DashboardsRemoveDashboardOKBody) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateMeta(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *V2DashboardsRemoveDashboardOKBody) validateMeta(formats strfmt.Registry) error {
	if swag.IsZero(m.Meta) { // not required
		return nil
	}

	if m.Meta != nil {
		if err := m.Meta.Validate(formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("meta")
			}
			return err
		}
	}

	return nil
}

// ContextValidate validate this v2 dashboards remove dashboard o k body based on the context it is used
func (m *V2DashboardsRemoveDashboardOKBody) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateMeta(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *V2DashboardsRemoveDashboardOKBody) contextValidateMeta(ctx context.Context, formats strfmt.Registry) error {

	if m.Meta != nil {
		if err := m.Meta.ContextValidate(ctx, formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("meta")
			}
			return err
		}
	}

	return nil
}

// MarshalBinary interface implementation
func (m *V2DashboardsRemoveDashboardOKBody) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2DashboardsRemoveDashboardOKBody) UnmarshalBinary(b []byte) error {
	var res V2DashboardsRemoveDashboardOKBody
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
