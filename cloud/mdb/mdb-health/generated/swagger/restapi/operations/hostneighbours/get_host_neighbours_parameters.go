// Code generated by go-swagger; DO NOT EDIT.

package hostneighbours

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"net/http"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/runtime"
	"github.com/go-openapi/runtime/middleware"
	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
)

// NewGetHostNeighboursParams creates a new GetHostNeighboursParams object
//
// There are no default values defined in the spec.
func NewGetHostNeighboursParams() GetHostNeighboursParams {

	return GetHostNeighboursParams{}
}

// GetHostNeighboursParams contains all the bound params for the get host neighbours operation
// typically these are obtained from a http.Request
//
// swagger:parameters GetHostNeighbours
type GetHostNeighboursParams struct {

	// HTTP Request Object
	HTTPRequest *http.Request `json:"-"`

	/*Unique request ID (must be generated for each separate request, even retries)
	  In: header
	*/
	XRequestID *string
	/*List of fqdns to retrieve data for
	  Required: true
	  In: query
	  Collection Format: csv
	*/
	Fqdns []string
}

// BindRequest both binds and validates a request, it assumes that complex things implement a Validatable(strfmt.Registry) error interface
// for simple values it will use straight method calls.
//
// To ensure default values, the struct must have been initialized with NewGetHostNeighboursParams() beforehand.
func (o *GetHostNeighboursParams) BindRequest(r *http.Request, route *middleware.MatchedRoute) error {
	var res []error

	o.HTTPRequest = r

	qs := runtime.Values(r.URL.Query())

	if err := o.bindXRequestID(r.Header[http.CanonicalHeaderKey("X-Request-Id")], true, route.Formats); err != nil {
		res = append(res, err)
	}

	qFqdns, qhkFqdns, _ := qs.GetOK("fqdns")
	if err := o.bindFqdns(qFqdns, qhkFqdns, route.Formats); err != nil {
		res = append(res, err)
	}
	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

// bindXRequestID binds and validates parameter XRequestID from header.
func (o *GetHostNeighboursParams) bindXRequestID(rawData []string, hasKey bool, formats strfmt.Registry) error {
	var raw string
	if len(rawData) > 0 {
		raw = rawData[len(rawData)-1]
	}

	// Required: false

	if raw == "" { // empty values pass all other validations
		return nil
	}
	o.XRequestID = &raw

	return nil
}

// bindFqdns binds and validates array parameter Fqdns from query.
//
// Arrays are parsed according to CollectionFormat: "csv" (defaults to "csv" when empty).
func (o *GetHostNeighboursParams) bindFqdns(rawData []string, hasKey bool, formats strfmt.Registry) error {
	if !hasKey {
		return errors.Required("fqdns", "query", rawData)
	}
	var qvFqdns string
	if len(rawData) > 0 {
		qvFqdns = rawData[len(rawData)-1]
	}

	// CollectionFormat: csv
	fqdnsIC := swag.SplitByFormat(qvFqdns, "csv")
	if len(fqdnsIC) == 0 {
		return errors.Required("fqdns", "query", fqdnsIC)
	}

	var fqdnsIR []string
	for _, fqdnsIV := range fqdnsIC {
		fqdnsI := fqdnsIV

		fqdnsIR = append(fqdnsIR, fqdnsI)
	}

	o.Fqdns = fqdnsIR

	return nil
}
