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

// V2HistoryGetNotificationRollupsOKBodyItemsItemsRollup v2 history get notification rollups o k body items items rollup
//
// swagger:model v2HistoryGetNotificationRollupsOKBodyItemsItemsRollup
type V2HistoryGetNotificationRollupsOKBodyItemsItemsRollup struct {

	// counts
	Counts []*V2HistoryGetNotificationRollupsOKBodyItemsItemsRollupCountsItems `json:"counts"`

	// start time
	StartTime float64 `json:"start_time,omitempty"`
}

// Validate validates this v2 history get notification rollups o k body items items rollup
func (m *V2HistoryGetNotificationRollupsOKBodyItemsItemsRollup) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateCounts(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *V2HistoryGetNotificationRollupsOKBodyItemsItemsRollup) validateCounts(formats strfmt.Registry) error {
	if swag.IsZero(m.Counts) { // not required
		return nil
	}

	for i := 0; i < len(m.Counts); i++ {
		if swag.IsZero(m.Counts[i]) { // not required
			continue
		}

		if m.Counts[i] != nil {
			if err := m.Counts[i].Validate(formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("counts" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// ContextValidate validate this v2 history get notification rollups o k body items items rollup based on the context it is used
func (m *V2HistoryGetNotificationRollupsOKBodyItemsItemsRollup) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateCounts(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *V2HistoryGetNotificationRollupsOKBodyItemsItemsRollup) contextValidateCounts(ctx context.Context, formats strfmt.Registry) error {

	for i := 0; i < len(m.Counts); i++ {

		if m.Counts[i] != nil {
			if err := m.Counts[i].ContextValidate(ctx, formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("counts" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// MarshalBinary interface implementation
func (m *V2HistoryGetNotificationRollupsOKBodyItemsItemsRollup) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2HistoryGetNotificationRollupsOKBodyItemsItemsRollup) UnmarshalBinary(b []byte) error {
	var res V2HistoryGetNotificationRollupsOKBodyItemsItemsRollup
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
