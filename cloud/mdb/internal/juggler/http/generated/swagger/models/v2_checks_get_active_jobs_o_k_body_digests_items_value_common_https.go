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

// V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonHTTPS v2 checks get active jobs o k body digests items value common Https
//
// swagger:model v2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonHttps
type V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonHTTPS struct {

	// allow self signed
	AllowSelfSigned bool `json:"AllowSelfSigned,omitempty"`

	// base Http options
	BaseHTTPOptions *V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonHTTPSBaseHTTPOptions `json:"BaseHttpOptions,omitempty"`

	// crit expire
	CritExpire int64 `json:"CritExpire,omitempty"`

	// ssl ciphers
	SslCiphers string `json:"SslCiphers,omitempty"`

	// validate hostname
	ValidateHostname bool `json:"ValidateHostname,omitempty"`

	// warn expire
	WarnExpire int64 `json:"WarnExpire,omitempty"`
}

// Validate validates this v2 checks get active jobs o k body digests items value common Https
func (m *V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonHTTPS) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateBaseHTTPOptions(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonHTTPS) validateBaseHTTPOptions(formats strfmt.Registry) error {
	if swag.IsZero(m.BaseHTTPOptions) { // not required
		return nil
	}

	if m.BaseHTTPOptions != nil {
		if err := m.BaseHTTPOptions.Validate(formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("BaseHttpOptions")
			}
			return err
		}
	}

	return nil
}

// ContextValidate validate this v2 checks get active jobs o k body digests items value common Https based on the context it is used
func (m *V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonHTTPS) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateBaseHTTPOptions(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonHTTPS) contextValidateBaseHTTPOptions(ctx context.Context, formats strfmt.Registry) error {

	if m.BaseHTTPOptions != nil {
		if err := m.BaseHTTPOptions.ContextValidate(ctx, formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("BaseHttpOptions")
			}
			return err
		}
	}

	return nil
}

// MarshalBinary interface implementation
func (m *V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonHTTPS) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonHTTPS) UnmarshalBinary(b []byte) error {
	var res V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonHTTPS
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
