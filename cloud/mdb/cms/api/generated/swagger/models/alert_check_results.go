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

// AlertCheckResults alert check results
//
// swagger:model AlertCheckResults
type AlertCheckResults struct {

	// Requests number that duty should consider
	// Required: true
	ConsiderRequestsNum *int64 `json:"consider_requests_num"`

	// maps request names to list of dom0 that should be back already, but they not
	// Required: true
	ExpiredDom0 []*AlertCheckResultsExpiredDom0Items0 `json:"expired_dom0"`

	// list of dom0 that Wall-e has been holding for too long, they need to be resetuped
	ResetupDom0 []*AlertCheckResultsResetupDom0Items0 `json:"resetup_dom0"`

	// list of dom0 Wall-e returned, but CMS haven't finished yet
	// Required: true
	UnfinishedDom0 []*AlertCheckResultsUnfinishedDom0Items0 `json:"unfinished_dom0"`
}

// Validate validates this alert check results
func (m *AlertCheckResults) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateConsiderRequestsNum(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateExpiredDom0(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateResetupDom0(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateUnfinishedDom0(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *AlertCheckResults) validateConsiderRequestsNum(formats strfmt.Registry) error {

	if err := validate.Required("consider_requests_num", "body", m.ConsiderRequestsNum); err != nil {
		return err
	}

	return nil
}

func (m *AlertCheckResults) validateExpiredDom0(formats strfmt.Registry) error {

	if err := validate.Required("expired_dom0", "body", m.ExpiredDom0); err != nil {
		return err
	}

	for i := 0; i < len(m.ExpiredDom0); i++ {
		if swag.IsZero(m.ExpiredDom0[i]) { // not required
			continue
		}

		if m.ExpiredDom0[i] != nil {
			if err := m.ExpiredDom0[i].Validate(formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("expired_dom0" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

func (m *AlertCheckResults) validateResetupDom0(formats strfmt.Registry) error {
	if swag.IsZero(m.ResetupDom0) { // not required
		return nil
	}

	for i := 0; i < len(m.ResetupDom0); i++ {
		if swag.IsZero(m.ResetupDom0[i]) { // not required
			continue
		}

		if m.ResetupDom0[i] != nil {
			if err := m.ResetupDom0[i].Validate(formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("resetup_dom0" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

func (m *AlertCheckResults) validateUnfinishedDom0(formats strfmt.Registry) error {

	if err := validate.Required("unfinished_dom0", "body", m.UnfinishedDom0); err != nil {
		return err
	}

	for i := 0; i < len(m.UnfinishedDom0); i++ {
		if swag.IsZero(m.UnfinishedDom0[i]) { // not required
			continue
		}

		if m.UnfinishedDom0[i] != nil {
			if err := m.UnfinishedDom0[i].Validate(formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("unfinished_dom0" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// ContextValidate validate this alert check results based on the context it is used
func (m *AlertCheckResults) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateExpiredDom0(ctx, formats); err != nil {
		res = append(res, err)
	}

	if err := m.contextValidateResetupDom0(ctx, formats); err != nil {
		res = append(res, err)
	}

	if err := m.contextValidateUnfinishedDom0(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *AlertCheckResults) contextValidateExpiredDom0(ctx context.Context, formats strfmt.Registry) error {

	for i := 0; i < len(m.ExpiredDom0); i++ {

		if m.ExpiredDom0[i] != nil {
			if err := m.ExpiredDom0[i].ContextValidate(ctx, formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("expired_dom0" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

func (m *AlertCheckResults) contextValidateResetupDom0(ctx context.Context, formats strfmt.Registry) error {

	for i := 0; i < len(m.ResetupDom0); i++ {

		if m.ResetupDom0[i] != nil {
			if err := m.ResetupDom0[i].ContextValidate(ctx, formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("resetup_dom0" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

func (m *AlertCheckResults) contextValidateUnfinishedDom0(ctx context.Context, formats strfmt.Registry) error {

	for i := 0; i < len(m.UnfinishedDom0); i++ {

		if m.UnfinishedDom0[i] != nil {
			if err := m.UnfinishedDom0[i].ContextValidate(ctx, formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("unfinished_dom0" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// MarshalBinary interface implementation
func (m *AlertCheckResults) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *AlertCheckResults) UnmarshalBinary(b []byte) error {
	var res AlertCheckResults
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}

// AlertCheckResultsExpiredDom0Items0 alert check results expired dom0 items0
//
// swagger:model AlertCheckResultsExpiredDom0Items0
type AlertCheckResultsExpiredDom0Items0 struct {

	// dom0
	// Required: true
	Dom0 *string `json:"dom0"`

	// duration
	Duration string `json:"duration,omitempty"`

	// req name
	// Required: true
	ReqName *string `json:"req_name"`
}

// Validate validates this alert check results expired dom0 items0
func (m *AlertCheckResultsExpiredDom0Items0) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateDom0(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateReqName(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *AlertCheckResultsExpiredDom0Items0) validateDom0(formats strfmt.Registry) error {

	if err := validate.Required("dom0", "body", m.Dom0); err != nil {
		return err
	}

	return nil
}

func (m *AlertCheckResultsExpiredDom0Items0) validateReqName(formats strfmt.Registry) error {

	if err := validate.Required("req_name", "body", m.ReqName); err != nil {
		return err
	}

	return nil
}

// ContextValidate validates this alert check results expired dom0 items0 based on context it is used
func (m *AlertCheckResultsExpiredDom0Items0) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *AlertCheckResultsExpiredDom0Items0) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *AlertCheckResultsExpiredDom0Items0) UnmarshalBinary(b []byte) error {
	var res AlertCheckResultsExpiredDom0Items0
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}

// AlertCheckResultsResetupDom0Items0 alert check results resetup dom0 items0
//
// swagger:model AlertCheckResultsResetupDom0Items0
type AlertCheckResultsResetupDom0Items0 struct {

	// dom0
	// Required: true
	Dom0 *string `json:"dom0"`

	// duration
	Duration string `json:"duration,omitempty"`

	// req name
	// Required: true
	ReqName *string `json:"req_name"`
}

// Validate validates this alert check results resetup dom0 items0
func (m *AlertCheckResultsResetupDom0Items0) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateDom0(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateReqName(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *AlertCheckResultsResetupDom0Items0) validateDom0(formats strfmt.Registry) error {

	if err := validate.Required("dom0", "body", m.Dom0); err != nil {
		return err
	}

	return nil
}

func (m *AlertCheckResultsResetupDom0Items0) validateReqName(formats strfmt.Registry) error {

	if err := validate.Required("req_name", "body", m.ReqName); err != nil {
		return err
	}

	return nil
}

// ContextValidate validates this alert check results resetup dom0 items0 based on context it is used
func (m *AlertCheckResultsResetupDom0Items0) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *AlertCheckResultsResetupDom0Items0) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *AlertCheckResultsResetupDom0Items0) UnmarshalBinary(b []byte) error {
	var res AlertCheckResultsResetupDom0Items0
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}

// AlertCheckResultsUnfinishedDom0Items0 alert check results unfinished dom0 items0
//
// swagger:model AlertCheckResultsUnfinishedDom0Items0
type AlertCheckResultsUnfinishedDom0Items0 struct {

	// dom0
	// Required: true
	Dom0 *string `json:"dom0"`

	// duration
	Duration string `json:"duration,omitempty"`

	// req name
	// Required: true
	ReqName *string `json:"req_name"`
}

// Validate validates this alert check results unfinished dom0 items0
func (m *AlertCheckResultsUnfinishedDom0Items0) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateDom0(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateReqName(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *AlertCheckResultsUnfinishedDom0Items0) validateDom0(formats strfmt.Registry) error {

	if err := validate.Required("dom0", "body", m.Dom0); err != nil {
		return err
	}

	return nil
}

func (m *AlertCheckResultsUnfinishedDom0Items0) validateReqName(formats strfmt.Registry) error {

	if err := validate.Required("req_name", "body", m.ReqName); err != nil {
		return err
	}

	return nil
}

// ContextValidate validates this alert check results unfinished dom0 items0 based on context it is used
func (m *AlertCheckResultsUnfinishedDom0Items0) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *AlertCheckResultsUnfinishedDom0Items0) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *AlertCheckResultsUnfinishedDom0Items0) UnmarshalBinary(b []byte) error {
	var res AlertCheckResultsUnfinishedDom0Items0
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
