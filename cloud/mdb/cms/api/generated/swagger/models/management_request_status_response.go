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
	"github.com/go-openapi/validate"
)

// ManagementRequestStatusResponse management request status response
//
// swagger:model ManagementRequestStatusResponse
type ManagementRequestStatusResponse struct {

	// Hosts to process the action on, must match value from the request
	// Required: true
	// Min Items: 1
	Hosts []Fqdn `json:"hosts"`

	// id
	// Required: true
	ID *ManagementRequestID `json:"id"`

	// Message for the current status
	Message string `json:"message,omitempty"`

	// status
	// Required: true
	Status *TaskStatus `json:"status"`
}

// Validate validates this management request status response
func (m *ManagementRequestStatusResponse) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateHosts(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateID(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateStatus(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *ManagementRequestStatusResponse) validateHosts(formats strfmt.Registry) error {

	if err := validate.Required("hosts", "body", m.Hosts); err != nil {
		return err
	}

	iHostsSize := int64(len(m.Hosts))

	if err := validate.MinItems("hosts", "body", iHostsSize, 1); err != nil {
		return err
	}

	for i := 0; i < len(m.Hosts); i++ {

		if err := m.Hosts[i].Validate(formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("hosts" + "." + strconv.Itoa(i))
			}
			return err
		}

	}

	return nil
}

func (m *ManagementRequestStatusResponse) validateID(formats strfmt.Registry) error {

	if err := validate.Required("id", "body", m.ID); err != nil {
		return err
	}

	if err := validate.Required("id", "body", m.ID); err != nil {
		return err
	}

	if m.ID != nil {
		if err := m.ID.Validate(formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("id")
			}
			return err
		}
	}

	return nil
}

func (m *ManagementRequestStatusResponse) validateStatus(formats strfmt.Registry) error {

	if err := validate.Required("status", "body", m.Status); err != nil {
		return err
	}

	if err := validate.Required("status", "body", m.Status); err != nil {
		return err
	}

	if m.Status != nil {
		if err := m.Status.Validate(formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("status")
			}
			return err
		}
	}

	return nil
}

// ContextValidate validate this management request status response based on the context it is used
func (m *ManagementRequestStatusResponse) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateHosts(ctx, formats); err != nil {
		res = append(res, err)
	}

	if err := m.contextValidateID(ctx, formats); err != nil {
		res = append(res, err)
	}

	if err := m.contextValidateStatus(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *ManagementRequestStatusResponse) contextValidateHosts(ctx context.Context, formats strfmt.Registry) error {

	for i := 0; i < len(m.Hosts); i++ {

		if err := m.Hosts[i].ContextValidate(ctx, formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("hosts" + "." + strconv.Itoa(i))
			}
			return err
		}

	}

	return nil
}

func (m *ManagementRequestStatusResponse) contextValidateID(ctx context.Context, formats strfmt.Registry) error {

	if m.ID != nil {
		if err := m.ID.ContextValidate(ctx, formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("id")
			}
			return err
		}
	}

	return nil
}

func (m *ManagementRequestStatusResponse) contextValidateStatus(ctx context.Context, formats strfmt.Registry) error {

	if m.Status != nil {
		if err := m.Status.ContextValidate(ctx, formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("status")
			}
			return err
		}
	}

	return nil
}

// MarshalBinary interface implementation
func (m *ManagementRequestStatusResponse) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *ManagementRequestStatusResponse) UnmarshalBinary(b []byte) error {
	var res ManagementRequestStatusResponse
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
