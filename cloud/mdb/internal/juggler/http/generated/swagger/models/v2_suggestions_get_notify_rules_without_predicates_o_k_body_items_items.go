// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"

	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
)

// V2SuggestionsGetNotifyRulesWithoutPredicatesOKBodyItemsItems v2 suggestions get notify rules without predicates o k body items items
//
// swagger:model v2SuggestionsGetNotifyRulesWithoutPredicatesOKBodyItemsItems
type V2SuggestionsGetNotifyRulesWithoutPredicatesOKBodyItemsItems struct {

	// id
	ID string `json:"id,omitempty"`

	// namespace
	Namespace string `json:"namespace,omitempty"`

	// selector
	Selector string `json:"selector,omitempty"`
}

// Validate validates this v2 suggestions get notify rules without predicates o k body items items
func (m *V2SuggestionsGetNotifyRulesWithoutPredicatesOKBodyItemsItems) Validate(formats strfmt.Registry) error {
	return nil
}

// ContextValidate validates this v2 suggestions get notify rules without predicates o k body items items based on context it is used
func (m *V2SuggestionsGetNotifyRulesWithoutPredicatesOKBodyItemsItems) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *V2SuggestionsGetNotifyRulesWithoutPredicatesOKBodyItemsItems) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *V2SuggestionsGetNotifyRulesWithoutPredicatesOKBodyItemsItems) UnmarshalBinary(b []byte) error {
	var res V2SuggestionsGetNotifyRulesWithoutPredicatesOKBodyItemsItems
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
