// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"

	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
)

// V2DashboardsSetDashboardParamsBodyComponentsItemsLinksItems v2 dashboards set dashboard params body components items links items
//
// swagger:model v2DashboardsSetDashboardParamsBodyComponentsItemsLinksItems
type V2DashboardsSetDashboardParamsBodyComponentsItemsLinksItems struct {

	// title
	Title string `json:"title,omitempty"`

	// url
	URL string `json:"url,omitempty"`
}

// Validate validates this v2 dashboards set dashboard params body components items links items
func (m *V2DashboardsSetDashboardParamsBodyComponentsItemsLinksItems) Validate(formats strfmt.Registry) error {
	return nil
}

// ContextValidate validates this v2 dashboards set dashboard params body components items links items based on context it is used
func (m *V2DashboardsSetDashboardParamsBodyComponentsItemsLinksItems) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *V2DashboardsSetDashboardParamsBodyComponentsItemsLinksItems) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2DashboardsSetDashboardParamsBodyComponentsItemsLinksItems) UnmarshalBinary(b []byte) error {
	var res V2DashboardsSetDashboardParamsBodyComponentsItemsLinksItems
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
