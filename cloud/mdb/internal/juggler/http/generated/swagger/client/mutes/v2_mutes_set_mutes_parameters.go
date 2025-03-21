// Code generated by go-swagger; DO NOT EDIT.

package mutes

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

// NewV2MutesSetMutesParams creates a new V2MutesSetMutesParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewV2MutesSetMutesParams() *V2MutesSetMutesParams {
	return &V2MutesSetMutesParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewV2MutesSetMutesParamsWithTimeout creates a new V2MutesSetMutesParams object
// with the ability to set a timeout on a request.
func NewV2MutesSetMutesParamsWithTimeout(timeout time.Duration) *V2MutesSetMutesParams {
	return &V2MutesSetMutesParams{
		timeout: timeout,
	}
}

// NewV2MutesSetMutesParamsWithContext creates a new V2MutesSetMutesParams object
// with the ability to set a context for a request.
func NewV2MutesSetMutesParamsWithContext(ctx context.Context) *V2MutesSetMutesParams {
	return &V2MutesSetMutesParams{
		Context: ctx,
	}
}

// NewV2MutesSetMutesParamsWithHTTPClient creates a new V2MutesSetMutesParams object
// with the ability to set a custom HTTPClient for a request.
func NewV2MutesSetMutesParamsWithHTTPClient(client *http.Client) *V2MutesSetMutesParams {
	return &V2MutesSetMutesParams{
		HTTPClient: client,
	}
}

/* V2MutesSetMutesParams contains all the parameters to send to the API endpoint
   for the v2 mutes set mutes operation.

   Typically these are written to a http.Request.
*/
type V2MutesSetMutesParams struct {

	// SetMuteRequest.
	SetMuteRequest *models.V2MutesSetMutesParamsBody

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the v2 mutes set mutes params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *V2MutesSetMutesParams) WithDefaults() *V2MutesSetMutesParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the v2 mutes set mutes params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *V2MutesSetMutesParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the v2 mutes set mutes params
func (o *V2MutesSetMutesParams) WithTimeout(timeout time.Duration) *V2MutesSetMutesParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the v2 mutes set mutes params
func (o *V2MutesSetMutesParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the v2 mutes set mutes params
func (o *V2MutesSetMutesParams) WithContext(ctx context.Context) *V2MutesSetMutesParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the v2 mutes set mutes params
func (o *V2MutesSetMutesParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the v2 mutes set mutes params
func (o *V2MutesSetMutesParams) WithHTTPClient(client *http.Client) *V2MutesSetMutesParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the v2 mutes set mutes params
func (o *V2MutesSetMutesParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithSetMuteRequest adds the setMuteRequest to the v2 mutes set mutes params
func (o *V2MutesSetMutesParams) WithSetMuteRequest(setMuteRequest *models.V2MutesSetMutesParamsBody) *V2MutesSetMutesParams {
	o.SetSetMuteRequest(setMuteRequest)
	return o
}

// SetSetMuteRequest adds the setMuteRequest to the v2 mutes set mutes params
func (o *V2MutesSetMutesParams) SetSetMuteRequest(setMuteRequest *models.V2MutesSetMutesParamsBody) {
	o.SetMuteRequest = setMuteRequest
}

// WriteToRequest writes these params to a swagger request
func (o *V2MutesSetMutesParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error
	if o.SetMuteRequest != nil {
		if err := r.SetBodyParam(o.SetMuteRequest); err != nil {
			return err
		}
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
