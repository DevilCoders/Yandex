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

// JobResultsListResp job results list resp
//
// swagger:model JobResultsListResp
type JobResultsListResp struct {
	Paging

	// job results
	JobResults []*JobResultResp `json:"jobResults"`
}

// UnmarshalJSON unmarshals this object from a JSON structure
func (m *JobResultsListResp) UnmarshalJSON(raw []byte) error {
	// AO0
	var aO0 Paging
	if err := swag.ReadJSON(raw, &aO0); err != nil {
		return err
	}
	m.Paging = aO0

	// AO1
	var dataAO1 struct {
		JobResults []*JobResultResp `json:"jobResults"`
	}
	if err := swag.ReadJSON(raw, &dataAO1); err != nil {
		return err
	}

	m.JobResults = dataAO1.JobResults

	return nil
}

// MarshalJSON marshals this object to a JSON structure
func (m JobResultsListResp) MarshalJSON() ([]byte, error) {
	_parts := make([][]byte, 0, 2)

	aO0, err := swag.WriteJSON(m.Paging)
	if err != nil {
		return nil, err
	}
	_parts = append(_parts, aO0)
	var dataAO1 struct {
		JobResults []*JobResultResp `json:"jobResults"`
	}

	dataAO1.JobResults = m.JobResults

	jsonDataAO1, errAO1 := swag.WriteJSON(dataAO1)
	if errAO1 != nil {
		return nil, errAO1
	}
	_parts = append(_parts, jsonDataAO1)
	return swag.ConcatJSON(_parts...), nil
}

// Validate validates this job results list resp
func (m *JobResultsListResp) Validate(formats strfmt.Registry) error {
	var res []error

	// validation for a type composition with Paging
	if err := m.Paging.Validate(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateJobResults(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *JobResultsListResp) validateJobResults(formats strfmt.Registry) error {

	if swag.IsZero(m.JobResults) { // not required
		return nil
	}

	for i := 0; i < len(m.JobResults); i++ {
		if swag.IsZero(m.JobResults[i]) { // not required
			continue
		}

		if m.JobResults[i] != nil {
			if err := m.JobResults[i].Validate(formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("jobResults" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// ContextValidate validate this job results list resp based on the context it is used
func (m *JobResultsListResp) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	// validation for a type composition with Paging
	if err := m.Paging.ContextValidate(ctx, formats); err != nil {
		res = append(res, err)
	}

	if err := m.contextValidateJobResults(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *JobResultsListResp) contextValidateJobResults(ctx context.Context, formats strfmt.Registry) error {

	for i := 0; i < len(m.JobResults); i++ {

		if m.JobResults[i] != nil {
			if err := m.JobResults[i].ContextValidate(ctx, formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("jobResults" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// MarshalBinary interface implementation
func (m *JobResultsListResp) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *JobResultsListResp) UnmarshalBinary(b []byte) error {
	var res JobResultsListResp
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
