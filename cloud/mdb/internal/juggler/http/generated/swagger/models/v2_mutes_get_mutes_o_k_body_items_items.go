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

// V2MutesGetMutesOKBodyItemsItems v2 mutes get mutes o k body items items
//
// swagger:model v2MutesGetMutesOKBodyItemsItems
type V2MutesGetMutesOKBodyItemsItems struct {

	// description
	Description string `json:"description,omitempty"`

	// end time
	EndTime float64 `json:"end_time,omitempty"`

	// filters
	Filters []*V2MutesGetMutesOKBodyItemsItemsFiltersItems `json:"filters"`

	// mute id
	MuteID string `json:"mute_id,omitempty"`

	// start time
	StartTime float64 `json:"start_time,omitempty"`

	// startrek ticket
	StartrekTicket string `json:"startrek_ticket,omitempty"`

	// user
	User string `json:"user,omitempty"`
}

// Validate validates this v2 mutes get mutes o k body items items
func (m *V2MutesGetMutesOKBodyItemsItems) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateFilters(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *V2MutesGetMutesOKBodyItemsItems) validateFilters(formats strfmt.Registry) error {
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

// ContextValidate validate this v2 mutes get mutes o k body items items based on the context it is used
func (m *V2MutesGetMutesOKBodyItemsItems) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateFilters(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *V2MutesGetMutesOKBodyItemsItems) contextValidateFilters(ctx context.Context, formats strfmt.Registry) error {

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
func (m *V2MutesGetMutesOKBodyItemsItems) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2MutesGetMutesOKBodyItemsItems) UnmarshalBinary(b []byte) error {
	var res V2MutesGetMutesOKBodyItemsItems
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
