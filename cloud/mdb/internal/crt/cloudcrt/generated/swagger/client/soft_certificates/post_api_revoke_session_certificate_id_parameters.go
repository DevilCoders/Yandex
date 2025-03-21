// Code generated by go-swagger; DO NOT EDIT.

package soft_certificates

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
	"github.com/go-openapi/swag"

	"a.yandex-team.ru/cloud/mdb/internal/crt/cloudcrt/generated/swagger/models"
)

// NewPostAPIRevokeSessionCertificateIDParams creates a new PostAPIRevokeSessionCertificateIDParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewPostAPIRevokeSessionCertificateIDParams() *PostAPIRevokeSessionCertificateIDParams {
	return &PostAPIRevokeSessionCertificateIDParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewPostAPIRevokeSessionCertificateIDParamsWithTimeout creates a new PostAPIRevokeSessionCertificateIDParams object
// with the ability to set a timeout on a request.
func NewPostAPIRevokeSessionCertificateIDParamsWithTimeout(timeout time.Duration) *PostAPIRevokeSessionCertificateIDParams {
	return &PostAPIRevokeSessionCertificateIDParams{
		timeout: timeout,
	}
}

// NewPostAPIRevokeSessionCertificateIDParamsWithContext creates a new PostAPIRevokeSessionCertificateIDParams object
// with the ability to set a context for a request.
func NewPostAPIRevokeSessionCertificateIDParamsWithContext(ctx context.Context) *PostAPIRevokeSessionCertificateIDParams {
	return &PostAPIRevokeSessionCertificateIDParams{
		Context: ctx,
	}
}

// NewPostAPIRevokeSessionCertificateIDParamsWithHTTPClient creates a new PostAPIRevokeSessionCertificateIDParams object
// with the ability to set a custom HTTPClient for a request.
func NewPostAPIRevokeSessionCertificateIDParamsWithHTTPClient(client *http.Client) *PostAPIRevokeSessionCertificateIDParams {
	return &PostAPIRevokeSessionCertificateIDParams{
		HTTPClient: client,
	}
}

/* PostAPIRevokeSessionCertificateIDParams contains all the parameters to send to the API endpoint
   for the post API revoke session certificate ID operation.

   Typically these are written to a http.Request.
*/
type PostAPIRevokeSessionCertificateIDParams struct {

	/* ID.

	   SoftСertificate ID
	*/
	ID int64

	/* Sessionservicereq.

	   Revoke SoftСertificate by ID
	*/
	Sessionservicereq *models.RequestsSessionServiceReq

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the post API revoke session certificate ID params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *PostAPIRevokeSessionCertificateIDParams) WithDefaults() *PostAPIRevokeSessionCertificateIDParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the post API revoke session certificate ID params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *PostAPIRevokeSessionCertificateIDParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the post API revoke session certificate ID params
func (o *PostAPIRevokeSessionCertificateIDParams) WithTimeout(timeout time.Duration) *PostAPIRevokeSessionCertificateIDParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the post API revoke session certificate ID params
func (o *PostAPIRevokeSessionCertificateIDParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the post API revoke session certificate ID params
func (o *PostAPIRevokeSessionCertificateIDParams) WithContext(ctx context.Context) *PostAPIRevokeSessionCertificateIDParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the post API revoke session certificate ID params
func (o *PostAPIRevokeSessionCertificateIDParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the post API revoke session certificate ID params
func (o *PostAPIRevokeSessionCertificateIDParams) WithHTTPClient(client *http.Client) *PostAPIRevokeSessionCertificateIDParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the post API revoke session certificate ID params
func (o *PostAPIRevokeSessionCertificateIDParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithID adds the id to the post API revoke session certificate ID params
func (o *PostAPIRevokeSessionCertificateIDParams) WithID(id int64) *PostAPIRevokeSessionCertificateIDParams {
	o.SetID(id)
	return o
}

// SetID adds the id to the post API revoke session certificate ID params
func (o *PostAPIRevokeSessionCertificateIDParams) SetID(id int64) {
	o.ID = id
}

// WithSessionservicereq adds the sessionservicereq to the post API revoke session certificate ID params
func (o *PostAPIRevokeSessionCertificateIDParams) WithSessionservicereq(sessionservicereq *models.RequestsSessionServiceReq) *PostAPIRevokeSessionCertificateIDParams {
	o.SetSessionservicereq(sessionservicereq)
	return o
}

// SetSessionservicereq adds the sessionservicereq to the post API revoke session certificate ID params
func (o *PostAPIRevokeSessionCertificateIDParams) SetSessionservicereq(sessionservicereq *models.RequestsSessionServiceReq) {
	o.Sessionservicereq = sessionservicereq
}

// WriteToRequest writes these params to a swagger request
func (o *PostAPIRevokeSessionCertificateIDParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error

	// path param id
	if err := r.SetPathParam("id", swag.FormatInt64(o.ID)); err != nil {
		return err
	}
	if o.Sessionservicereq != nil {
		if err := r.SetBodyParam(o.Sessionservicereq); err != nil {
			return err
		}
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
