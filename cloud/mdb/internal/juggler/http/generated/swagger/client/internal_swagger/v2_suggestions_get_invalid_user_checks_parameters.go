// Code generated by go-swagger; DO NOT EDIT.

package internal_swagger

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"
	"net/http"
	"time"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/runtime"
	cr "github.com/go-openapi/runtime/client"
	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/models"
)

// NewV2SuggestionsGetInvalidUserChecksParams creates a new V2SuggestionsGetInvalidUserChecksParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewV2SuggestionsGetInvalidUserChecksParams() *V2SuggestionsGetInvalidUserChecksParams {
	return &V2SuggestionsGetInvalidUserChecksParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewV2SuggestionsGetInvalidUserChecksParamsWithTimeout creates a new V2SuggestionsGetInvalidUserChecksParams object
// with the ability to set a timeout on a request.
func NewV2SuggestionsGetInvalidUserChecksParamsWithTimeout(timeout time.Duration) *V2SuggestionsGetInvalidUserChecksParams {
	return &V2SuggestionsGetInvalidUserChecksParams{
		timeout: timeout,
	}
}

// NewV2SuggestionsGetInvalidUserChecksParamsWithContext creates a new V2SuggestionsGetInvalidUserChecksParams object
// with the ability to set a context for a request.
func NewV2SuggestionsGetInvalidUserChecksParamsWithContext(ctx context.Context) *V2SuggestionsGetInvalidUserChecksParams {
	return &V2SuggestionsGetInvalidUserChecksParams{
		Context: ctx,
	}
}

// NewV2SuggestionsGetInvalidUserChecksParamsWithHTTPClient creates a new V2SuggestionsGetInvalidUserChecksParams object
// with the ability to set a custom HTTPClient for a request.
func NewV2SuggestionsGetInvalidUserChecksParamsWithHTTPClient(client *http.Client) *V2SuggestionsGetInvalidUserChecksParams {
	return &V2SuggestionsGetInvalidUserChecksParams{
		HTTPClient: client,
	}
}

/* V2SuggestionsGetInvalidUserChecksParams contains all the parameters to send to the API endpoint
   for the v2 suggestions get invalid user checks operation.

   Typically these are written to a http.Request.
*/
type V2SuggestionsGetInvalidUserChecksParams struct {

	// TGetInvalidChecksRequest.
	TGetInvalidChecksRequest *models.V2SuggestionsGetInvalidUserChecksParamsBody

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the v2 suggestions get invalid user checks params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *V2SuggestionsGetInvalidUserChecksParams) WithDefaults() *V2SuggestionsGetInvalidUserChecksParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the v2 suggestions get invalid user checks params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *V2SuggestionsGetInvalidUserChecksParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the v2 suggestions get invalid user checks params
func (o *V2SuggestionsGetInvalidUserChecksParams) WithTimeout(timeout time.Duration) *V2SuggestionsGetInvalidUserChecksParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the v2 suggestions get invalid user checks params
func (o *V2SuggestionsGetInvalidUserChecksParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the v2 suggestions get invalid user checks params
func (o *V2SuggestionsGetInvalidUserChecksParams) WithContext(ctx context.Context) *V2SuggestionsGetInvalidUserChecksParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the v2 suggestions get invalid user checks params
func (o *V2SuggestionsGetInvalidUserChecksParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the v2 suggestions get invalid user checks params
func (o *V2SuggestionsGetInvalidUserChecksParams) WithHTTPClient(client *http.Client) *V2SuggestionsGetInvalidUserChecksParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the v2 suggestions get invalid user checks params
func (o *V2SuggestionsGetInvalidUserChecksParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithTGetInvalidChecksRequest adds the tGetInvalidChecksRequest to the v2 suggestions get invalid user checks params
func (o *V2SuggestionsGetInvalidUserChecksParams) WithTGetInvalidChecksRequest(tGetInvalidChecksRequest *models.V2SuggestionsGetInvalidUserChecksParamsBody) *V2SuggestionsGetInvalidUserChecksParams {
	o.SetTGetInvalidChecksRequest(tGetInvalidChecksRequest)
	return o
}

// SetTGetInvalidChecksRequest adds the tGetInvalidChecksRequest to the v2 suggestions get invalid user checks params
func (o *V2SuggestionsGetInvalidUserChecksParams) SetTGetInvalidChecksRequest(tGetInvalidChecksRequest *models.V2SuggestionsGetInvalidUserChecksParamsBody) {
	o.TGetInvalidChecksRequest = tGetInvalidChecksRequest
}

// WriteToRequest writes these params to a swagger request
func (o *V2SuggestionsGetInvalidUserChecksParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error
	if o.TGetInvalidChecksRequest != nil {
		if err := r.SetBodyParam(o.TGetInvalidChecksRequest); err != nil {
			return err
		}
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
