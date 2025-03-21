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

// NewV2SuggestionsGetDowntimedUserChecksParams creates a new V2SuggestionsGetDowntimedUserChecksParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewV2SuggestionsGetDowntimedUserChecksParams() *V2SuggestionsGetDowntimedUserChecksParams {
	return &V2SuggestionsGetDowntimedUserChecksParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewV2SuggestionsGetDowntimedUserChecksParamsWithTimeout creates a new V2SuggestionsGetDowntimedUserChecksParams object
// with the ability to set a timeout on a request.
func NewV2SuggestionsGetDowntimedUserChecksParamsWithTimeout(timeout time.Duration) *V2SuggestionsGetDowntimedUserChecksParams {
	return &V2SuggestionsGetDowntimedUserChecksParams{
		timeout: timeout,
	}
}

// NewV2SuggestionsGetDowntimedUserChecksParamsWithContext creates a new V2SuggestionsGetDowntimedUserChecksParams object
// with the ability to set a context for a request.
func NewV2SuggestionsGetDowntimedUserChecksParamsWithContext(ctx context.Context) *V2SuggestionsGetDowntimedUserChecksParams {
	return &V2SuggestionsGetDowntimedUserChecksParams{
		Context: ctx,
	}
}

// NewV2SuggestionsGetDowntimedUserChecksParamsWithHTTPClient creates a new V2SuggestionsGetDowntimedUserChecksParams object
// with the ability to set a custom HTTPClient for a request.
func NewV2SuggestionsGetDowntimedUserChecksParamsWithHTTPClient(client *http.Client) *V2SuggestionsGetDowntimedUserChecksParams {
	return &V2SuggestionsGetDowntimedUserChecksParams{
		HTTPClient: client,
	}
}

/* V2SuggestionsGetDowntimedUserChecksParams contains all the parameters to send to the API endpoint
   for the v2 suggestions get downtimed user checks operation.

   Typically these are written to a http.Request.
*/
type V2SuggestionsGetDowntimedUserChecksParams struct {

	// TGetInvalidChecksRequest.
	TGetInvalidChecksRequest *models.V2SuggestionsGetDowntimedUserChecksParamsBody

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the v2 suggestions get downtimed user checks params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *V2SuggestionsGetDowntimedUserChecksParams) WithDefaults() *V2SuggestionsGetDowntimedUserChecksParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the v2 suggestions get downtimed user checks params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *V2SuggestionsGetDowntimedUserChecksParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the v2 suggestions get downtimed user checks params
func (o *V2SuggestionsGetDowntimedUserChecksParams) WithTimeout(timeout time.Duration) *V2SuggestionsGetDowntimedUserChecksParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the v2 suggestions get downtimed user checks params
func (o *V2SuggestionsGetDowntimedUserChecksParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the v2 suggestions get downtimed user checks params
func (o *V2SuggestionsGetDowntimedUserChecksParams) WithContext(ctx context.Context) *V2SuggestionsGetDowntimedUserChecksParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the v2 suggestions get downtimed user checks params
func (o *V2SuggestionsGetDowntimedUserChecksParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the v2 suggestions get downtimed user checks params
func (o *V2SuggestionsGetDowntimedUserChecksParams) WithHTTPClient(client *http.Client) *V2SuggestionsGetDowntimedUserChecksParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the v2 suggestions get downtimed user checks params
func (o *V2SuggestionsGetDowntimedUserChecksParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithTGetInvalidChecksRequest adds the tGetInvalidChecksRequest to the v2 suggestions get downtimed user checks params
func (o *V2SuggestionsGetDowntimedUserChecksParams) WithTGetInvalidChecksRequest(tGetInvalidChecksRequest *models.V2SuggestionsGetDowntimedUserChecksParamsBody) *V2SuggestionsGetDowntimedUserChecksParams {
	o.SetTGetInvalidChecksRequest(tGetInvalidChecksRequest)
	return o
}

// SetTGetInvalidChecksRequest adds the tGetInvalidChecksRequest to the v2 suggestions get downtimed user checks params
func (o *V2SuggestionsGetDowntimedUserChecksParams) SetTGetInvalidChecksRequest(tGetInvalidChecksRequest *models.V2SuggestionsGetDowntimedUserChecksParamsBody) {
	o.TGetInvalidChecksRequest = tGetInvalidChecksRequest
}

// WriteToRequest writes these params to a swagger request
func (o *V2SuggestionsGetDowntimedUserChecksParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

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
