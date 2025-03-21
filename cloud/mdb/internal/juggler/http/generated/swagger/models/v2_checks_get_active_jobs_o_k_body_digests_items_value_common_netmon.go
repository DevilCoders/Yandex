// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"

	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
)

// V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonNetmon v2 checks get active jobs o k body digests items value common netmon
//
// swagger:model v2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonNetmon
type V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonNetmon struct {

	// crit percent
	CritPercent int64 `json:"CritPercent,omitempty"`

	// metric
	Metric string `json:"Metric,omitempty"`

	// network
	Network string `json:"Network,omitempty"`

	// protocol
	Protocol string `json:"Protocol,omitempty"`

	// warn percent
	WarnPercent int64 `json:"WarnPercent,omitempty"`
}

// Validate validates this v2 checks get active jobs o k body digests items value common netmon
func (m *V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonNetmon) Validate(formats strfmt.Registry) error {
	return nil
}

// ContextValidate validates this v2 checks get active jobs o k body digests items value common netmon based on context it is used
func (m *V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonNetmon) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonNetmon) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonNetmon) UnmarshalBinary(b []byte) error {
	var res V2ChecksGetActiveJobsOKBodyDigestsItemsValueCommonNetmon
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
