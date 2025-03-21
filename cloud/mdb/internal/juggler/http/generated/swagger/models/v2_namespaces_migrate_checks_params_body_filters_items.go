// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"

	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
)

// V2NamespacesMigrateChecksParamsBodyFiltersItems v2 namespaces migrate checks params body filters items
//
// swagger:model v2NamespacesMigrateChecksParamsBodyFiltersItems
type V2NamespacesMigrateChecksParamsBodyFiltersItems struct {

	// abc service
	AbcService string `json:"abc_service,omitempty"`

	// address
	Address string `json:"address,omitempty"`

	// host
	Host string `json:"host,omitempty"`

	// instance
	Instance string `json:"instance,omitempty"`

	// match raw events
	MatchRawEvents string `json:"match_raw_events,omitempty"`

	// method
	Method string `json:"method,omitempty"`

	// namespace
	Namespace string `json:"namespace,omitempty"`

	// owners
	Owners []string `json:"owners"`

	// recipients
	Recipients []string `json:"recipients"`

	// responsibles
	Responsibles []string `json:"responsibles"`

	// service
	Service string `json:"service,omitempty"`

	// tags
	Tags []string `json:"tags"`
}

// Validate validates this v2 namespaces migrate checks params body filters items
func (m *V2NamespacesMigrateChecksParamsBodyFiltersItems) Validate(formats strfmt.Registry) error {
	return nil
}

// ContextValidate validates this v2 namespaces migrate checks params body filters items based on context it is used
func (m *V2NamespacesMigrateChecksParamsBodyFiltersItems) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *V2NamespacesMigrateChecksParamsBodyFiltersItems) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2NamespacesMigrateChecksParamsBodyFiltersItems) UnmarshalBinary(b []byte) error {
	var res V2NamespacesMigrateChecksParamsBodyFiltersItems
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
