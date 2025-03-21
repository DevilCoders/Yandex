// Code generated by go-swagger; DO NOT EDIT.

package commands

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
)

// NewGetJobResultsListParams creates a new GetJobResultsListParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewGetJobResultsListParams() *GetJobResultsListParams {
	return &GetJobResultsListParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewGetJobResultsListParamsWithTimeout creates a new GetJobResultsListParams object
// with the ability to set a timeout on a request.
func NewGetJobResultsListParamsWithTimeout(timeout time.Duration) *GetJobResultsListParams {
	return &GetJobResultsListParams{
		timeout: timeout,
	}
}

// NewGetJobResultsListParamsWithContext creates a new GetJobResultsListParams object
// with the ability to set a context for a request.
func NewGetJobResultsListParamsWithContext(ctx context.Context) *GetJobResultsListParams {
	return &GetJobResultsListParams{
		Context: ctx,
	}
}

// NewGetJobResultsListParamsWithHTTPClient creates a new GetJobResultsListParams object
// with the ability to set a custom HTTPClient for a request.
func NewGetJobResultsListParamsWithHTTPClient(client *http.Client) *GetJobResultsListParams {
	return &GetJobResultsListParams{
		HTTPClient: client,
	}
}

/* GetJobResultsListParams contains all the parameters to send to the API endpoint
   for the get job results list operation.

   Typically these are written to a http.Request.
*/
type GetJobResultsListParams struct {

	/* Authorization.

	   OAuth token. It is not in security section because we also use cookies and you can't specify those in swagger 2.0.
	*/
	Authorization *string

	/* XRequestID.

	   Unique request ID (must be generated for each separate request, even retries)
	*/
	XRequestID *string

	/* ExtJobID.

	   Ext job ID
	*/
	ExtJobID *string

	/* Fqdn.

	   fqdn of whatever
	*/
	Fqdn *string

	/* JobID.

	   DEPRECATED! NOT A JOB ID. WILL BE USED AS ExtJobID
	*/
	JobID *string

	/* JobResultStatus.

	   Job result status
	*/
	JobResultStatus *string

	/* PageSize.

	   Number of results per page

	   Format: int64
	   Default: 100
	*/
	PageSize *int64

	/* PageToken.

	   Token to request the next page in listing
	*/
	PageToken *string

	/* SortOrder.

	   Sorting order for listings
	*/
	SortOrder *string

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the get job results list params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *GetJobResultsListParams) WithDefaults() *GetJobResultsListParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the get job results list params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *GetJobResultsListParams) SetDefaults() {
	var (
		pageSizeDefault = int64(100)
	)

	val := GetJobResultsListParams{
		PageSize: &pageSizeDefault,
	}

	val.timeout = o.timeout
	val.Context = o.Context
	val.HTTPClient = o.HTTPClient
	*o = val
}

// WithTimeout adds the timeout to the get job results list params
func (o *GetJobResultsListParams) WithTimeout(timeout time.Duration) *GetJobResultsListParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the get job results list params
func (o *GetJobResultsListParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the get job results list params
func (o *GetJobResultsListParams) WithContext(ctx context.Context) *GetJobResultsListParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the get job results list params
func (o *GetJobResultsListParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the get job results list params
func (o *GetJobResultsListParams) WithHTTPClient(client *http.Client) *GetJobResultsListParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the get job results list params
func (o *GetJobResultsListParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithAuthorization adds the authorization to the get job results list params
func (o *GetJobResultsListParams) WithAuthorization(authorization *string) *GetJobResultsListParams {
	o.SetAuthorization(authorization)
	return o
}

// SetAuthorization adds the authorization to the get job results list params
func (o *GetJobResultsListParams) SetAuthorization(authorization *string) {
	o.Authorization = authorization
}

// WithXRequestID adds the xRequestID to the get job results list params
func (o *GetJobResultsListParams) WithXRequestID(xRequestID *string) *GetJobResultsListParams {
	o.SetXRequestID(xRequestID)
	return o
}

// SetXRequestID adds the xRequestId to the get job results list params
func (o *GetJobResultsListParams) SetXRequestID(xRequestID *string) {
	o.XRequestID = xRequestID
}

// WithExtJobID adds the extJobID to the get job results list params
func (o *GetJobResultsListParams) WithExtJobID(extJobID *string) *GetJobResultsListParams {
	o.SetExtJobID(extJobID)
	return o
}

// SetExtJobID adds the extJobId to the get job results list params
func (o *GetJobResultsListParams) SetExtJobID(extJobID *string) {
	o.ExtJobID = extJobID
}

// WithFqdn adds the fqdn to the get job results list params
func (o *GetJobResultsListParams) WithFqdn(fqdn *string) *GetJobResultsListParams {
	o.SetFqdn(fqdn)
	return o
}

// SetFqdn adds the fqdn to the get job results list params
func (o *GetJobResultsListParams) SetFqdn(fqdn *string) {
	o.Fqdn = fqdn
}

// WithJobID adds the jobID to the get job results list params
func (o *GetJobResultsListParams) WithJobID(jobID *string) *GetJobResultsListParams {
	o.SetJobID(jobID)
	return o
}

// SetJobID adds the jobId to the get job results list params
func (o *GetJobResultsListParams) SetJobID(jobID *string) {
	o.JobID = jobID
}

// WithJobResultStatus adds the jobResultStatus to the get job results list params
func (o *GetJobResultsListParams) WithJobResultStatus(jobResultStatus *string) *GetJobResultsListParams {
	o.SetJobResultStatus(jobResultStatus)
	return o
}

// SetJobResultStatus adds the jobResultStatus to the get job results list params
func (o *GetJobResultsListParams) SetJobResultStatus(jobResultStatus *string) {
	o.JobResultStatus = jobResultStatus
}

// WithPageSize adds the pageSize to the get job results list params
func (o *GetJobResultsListParams) WithPageSize(pageSize *int64) *GetJobResultsListParams {
	o.SetPageSize(pageSize)
	return o
}

// SetPageSize adds the pageSize to the get job results list params
func (o *GetJobResultsListParams) SetPageSize(pageSize *int64) {
	o.PageSize = pageSize
}

// WithPageToken adds the pageToken to the get job results list params
func (o *GetJobResultsListParams) WithPageToken(pageToken *string) *GetJobResultsListParams {
	o.SetPageToken(pageToken)
	return o
}

// SetPageToken adds the pageToken to the get job results list params
func (o *GetJobResultsListParams) SetPageToken(pageToken *string) {
	o.PageToken = pageToken
}

// WithSortOrder adds the sortOrder to the get job results list params
func (o *GetJobResultsListParams) WithSortOrder(sortOrder *string) *GetJobResultsListParams {
	o.SetSortOrder(sortOrder)
	return o
}

// SetSortOrder adds the sortOrder to the get job results list params
func (o *GetJobResultsListParams) SetSortOrder(sortOrder *string) {
	o.SortOrder = sortOrder
}

// WriteToRequest writes these params to a swagger request
func (o *GetJobResultsListParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error

	if o.Authorization != nil {

		// header param Authorization
		if err := r.SetHeaderParam("Authorization", *o.Authorization); err != nil {
			return err
		}
	}

	if o.XRequestID != nil {

		// header param X-Request-Id
		if err := r.SetHeaderParam("X-Request-Id", *o.XRequestID); err != nil {
			return err
		}
	}

	if o.ExtJobID != nil {

		// query param extJobId
		var qrExtJobID string

		if o.ExtJobID != nil {
			qrExtJobID = *o.ExtJobID
		}
		qExtJobID := qrExtJobID
		if qExtJobID != "" {

			if err := r.SetQueryParam("extJobId", qExtJobID); err != nil {
				return err
			}
		}
	}

	if o.Fqdn != nil {

		// query param fqdn
		var qrFqdn string

		if o.Fqdn != nil {
			qrFqdn = *o.Fqdn
		}
		qFqdn := qrFqdn
		if qFqdn != "" {

			if err := r.SetQueryParam("fqdn", qFqdn); err != nil {
				return err
			}
		}
	}

	if o.JobID != nil {

		// query param jobId
		var qrJobID string

		if o.JobID != nil {
			qrJobID = *o.JobID
		}
		qJobID := qrJobID
		if qJobID != "" {

			if err := r.SetQueryParam("jobId", qJobID); err != nil {
				return err
			}
		}
	}

	if o.JobResultStatus != nil {

		// query param jobResultStatus
		var qrJobResultStatus string

		if o.JobResultStatus != nil {
			qrJobResultStatus = *o.JobResultStatus
		}
		qJobResultStatus := qrJobResultStatus
		if qJobResultStatus != "" {

			if err := r.SetQueryParam("jobResultStatus", qJobResultStatus); err != nil {
				return err
			}
		}
	}

	if o.PageSize != nil {

		// query param pageSize
		var qrPageSize int64

		if o.PageSize != nil {
			qrPageSize = *o.PageSize
		}
		qPageSize := swag.FormatInt64(qrPageSize)
		if qPageSize != "" {

			if err := r.SetQueryParam("pageSize", qPageSize); err != nil {
				return err
			}
		}
	}

	if o.PageToken != nil {

		// query param pageToken
		var qrPageToken string

		if o.PageToken != nil {
			qrPageToken = *o.PageToken
		}
		qPageToken := qrPageToken
		if qPageToken != "" {

			if err := r.SetQueryParam("pageToken", qPageToken); err != nil {
				return err
			}
		}
	}

	if o.SortOrder != nil {

		// query param sortOrder
		var qrSortOrder string

		if o.SortOrder != nil {
			qrSortOrder = *o.SortOrder
		}
		qSortOrder := qrSortOrder
		if qSortOrder != "" {

			if err := r.SetQueryParam("sortOrder", qSortOrder); err != nil {
				return err
			}
		}
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
