// Code generated by go-swagger; DO NOT EDIT.

package groups

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"net/http"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/runtime/middleware"
	"github.com/go-openapi/strfmt"
)

// NewDeleteGroupParams creates a new DeleteGroupParams object
//
// There are no default values defined in the spec.
func NewDeleteGroupParams() DeleteGroupParams {

	return DeleteGroupParams{}
}

// DeleteGroupParams contains all the bound params for the delete group operation
// typically these are obtained from a http.Request
//
// swagger:parameters DeleteGroup
type DeleteGroupParams struct {

	// HTTP Request Object
	HTTPRequest *http.Request `json:"-"`

	/*OAuth token. It is not in security section because we also use cookies and you can't specify those in swagger 2.0.
	  In: header
	*/
	Authorization *string
	/*Unique request ID (must be generated for each separate request, even retries)
	  In: header
	*/
	XRequestID *string
	/*Group's name
	  Required: true
	  In: path
	*/
	Groupname string
}

// BindRequest both binds and validates a request, it assumes that complex things implement a Validatable(strfmt.Registry) error interface
// for simple values it will use straight method calls.
//
// To ensure default values, the struct must have been initialized with NewDeleteGroupParams() beforehand.
func (o *DeleteGroupParams) BindRequest(r *http.Request, route *middleware.MatchedRoute) error {
	var res []error

	o.HTTPRequest = r

	if err := o.bindAuthorization(r.Header[http.CanonicalHeaderKey("Authorization")], true, route.Formats); err != nil {
		res = append(res, err)
	}

	if err := o.bindXRequestID(r.Header[http.CanonicalHeaderKey("X-Request-Id")], true, route.Formats); err != nil {
		res = append(res, err)
	}

	rGroupname, rhkGroupname, _ := route.Params.GetOK("groupname")
	if err := o.bindGroupname(rGroupname, rhkGroupname, route.Formats); err != nil {
		res = append(res, err)
	}
	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

// bindAuthorization binds and validates parameter Authorization from header.
func (o *DeleteGroupParams) bindAuthorization(rawData []string, hasKey bool, formats strfmt.Registry) error {
	var raw string
	if len(rawData) > 0 {
		raw = rawData[len(rawData)-1]
	}

	// Required: false

	if raw == "" { // empty values pass all other validations
		return nil
	}
	o.Authorization = &raw

	return nil
}

// bindXRequestID binds and validates parameter XRequestID from header.
func (o *DeleteGroupParams) bindXRequestID(rawData []string, hasKey bool, formats strfmt.Registry) error {
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

// bindGroupname binds and validates parameter Groupname from path.
func (o *DeleteGroupParams) bindGroupname(rawData []string, hasKey bool, formats strfmt.Registry) error {
	var raw string
	if len(rawData) > 0 {
		raw = rawData[len(rawData)-1]
	}

	// Required: true
	// Parameter is provided by construction from the route
	o.Groupname = raw

	return nil
}
