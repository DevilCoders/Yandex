// Code generated by go-swagger; DO NOT EDIT.

package masters

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

// NewGetMasterMinionsChangesParams creates a new GetMasterMinionsChangesParams object
// with the default values initialized.
func NewGetMasterMinionsChangesParams() GetMasterMinionsChangesParams {

	var (
		// initialize parameters with default values

		pageSizeDefault = int64(100)
	)

	return GetMasterMinionsChangesParams{
		PageSize: &pageSizeDefault,
	}
}

// GetMasterMinionsChangesParams contains all the bound params for the get master minions changes operation
// typically these are obtained from a http.Request
//
// swagger:parameters GetMasterMinionsChanges
type GetMasterMinionsChangesParams struct {

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
	/*fqdn of whatever
	  Required: true
	  In: path
	*/
	Fqdn string
	/*Timestamp in Unix Time starting from which to return changes
	  In: query
	*/
	FromTimestamp *int64
	/*Number of results per page
	  In: query
	  Default: 100
	*/
	PageSize *int64
	/*Token to request the next page in listing
	  In: query
	*/
	PageToken *string
}

// BindRequest both binds and validates a request, it assumes that complex things implement a Validatable(strfmt.Registry) error interface
// for simple values it will use straight method calls.
//
// To ensure default values, the struct must have been initialized with NewGetMasterMinionsChangesParams() beforehand.
func (o *GetMasterMinionsChangesParams) BindRequest(r *http.Request, route *middleware.MatchedRoute) error {
	var res []error

	o.HTTPRequest = r

	qs := runtime.Values(r.URL.Query())

	if err := o.bindAuthorization(r.Header[http.CanonicalHeaderKey("Authorization")], true, route.Formats); err != nil {
		res = append(res, err)
	}

	if err := o.bindXRequestID(r.Header[http.CanonicalHeaderKey("X-Request-Id")], true, route.Formats); err != nil {
		res = append(res, err)
	}

	rFqdn, rhkFqdn, _ := route.Params.GetOK("fqdn")
	if err := o.bindFqdn(rFqdn, rhkFqdn, route.Formats); err != nil {
		res = append(res, err)
	}

	qFromTimestamp, qhkFromTimestamp, _ := qs.GetOK("fromTimestamp")
	if err := o.bindFromTimestamp(qFromTimestamp, qhkFromTimestamp, route.Formats); err != nil {
		res = append(res, err)
	}

	qPageSize, qhkPageSize, _ := qs.GetOK("pageSize")
	if err := o.bindPageSize(qPageSize, qhkPageSize, route.Formats); err != nil {
		res = append(res, err)
	}

	qPageToken, qhkPageToken, _ := qs.GetOK("pageToken")
	if err := o.bindPageToken(qPageToken, qhkPageToken, route.Formats); err != nil {
		res = append(res, err)
	}
	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

// bindAuthorization binds and validates parameter Authorization from header.
func (o *GetMasterMinionsChangesParams) bindAuthorization(rawData []string, hasKey bool, formats strfmt.Registry) error {
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
func (o *GetMasterMinionsChangesParams) bindXRequestID(rawData []string, hasKey bool, formats strfmt.Registry) error {
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

// bindFqdn binds and validates parameter Fqdn from path.
func (o *GetMasterMinionsChangesParams) bindFqdn(rawData []string, hasKey bool, formats strfmt.Registry) error {
	var raw string
	if len(rawData) > 0 {
		raw = rawData[len(rawData)-1]
	}

	// Required: true
	// Parameter is provided by construction from the route
	o.Fqdn = raw

	return nil
}

// bindFromTimestamp binds and validates parameter FromTimestamp from query.
func (o *GetMasterMinionsChangesParams) bindFromTimestamp(rawData []string, hasKey bool, formats strfmt.Registry) error {
	var raw string
	if len(rawData) > 0 {
		raw = rawData[len(rawData)-1]
	}

	// Required: false
	// AllowEmptyValue: false

	if raw == "" { // empty values pass all other validations
		return nil
	}

	value, err := swag.ConvertInt64(raw)
	if err != nil {
		return errors.InvalidType("fromTimestamp", "query", "int64", raw)
	}
	o.FromTimestamp = &value

	return nil
}

// bindPageSize binds and validates parameter PageSize from query.
func (o *GetMasterMinionsChangesParams) bindPageSize(rawData []string, hasKey bool, formats strfmt.Registry) error {
	var raw string
	if len(rawData) > 0 {
		raw = rawData[len(rawData)-1]
	}

	// Required: false
	// AllowEmptyValue: false

	if raw == "" { // empty values pass all other validations
		// Default values have been previously initialized by NewGetMasterMinionsChangesParams()
		return nil
	}

	value, err := swag.ConvertInt64(raw)
	if err != nil {
		return errors.InvalidType("pageSize", "query", "int64", raw)
	}
	o.PageSize = &value

	return nil
}

// bindPageToken binds and validates parameter PageToken from query.
func (o *GetMasterMinionsChangesParams) bindPageToken(rawData []string, hasKey bool, formats strfmt.Registry) error {
	var raw string
	if len(rawData) > 0 {
		raw = rawData[len(rawData)-1]
	}

	// Required: false
	// AllowEmptyValue: false

	if raw == "" { // empty values pass all other validations
		return nil
	}
	o.PageToken = &raw

	return nil
}
