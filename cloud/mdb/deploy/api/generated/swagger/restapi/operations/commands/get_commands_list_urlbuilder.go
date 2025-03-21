// Code generated by go-swagger; DO NOT EDIT.

package commands

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the generate command

import (
	"errors"
	"net/url"
	golangswaggerpaths "path"

	"github.com/go-openapi/swag"
)

// GetCommandsListURL generates an URL for the get commands list operation
type GetCommandsListURL struct {
	CommandStatus *string
	Fqdn          *string
	PageSize      *int64
	PageToken     *string
	ShipmentID    *string
	SortOrder     *string

	_basePath string
	// avoid unkeyed usage
	_ struct{}
}

// WithBasePath sets the base path for this url builder, only required when it's different from the
// base path specified in the swagger spec.
// When the value of the base path is an empty string
func (o *GetCommandsListURL) WithBasePath(bp string) *GetCommandsListURL {
	o.SetBasePath(bp)
	return o
}

// SetBasePath sets the base path for this url builder, only required when it's different from the
// base path specified in the swagger spec.
// When the value of the base path is an empty string
func (o *GetCommandsListURL) SetBasePath(bp string) {
	o._basePath = bp
}

// Build a url path and query string
func (o *GetCommandsListURL) Build() (*url.URL, error) {
	var _result url.URL

	var _path = "/v1/commands"

	_basePath := o._basePath
	_result.Path = golangswaggerpaths.Join(_basePath, _path)

	qs := make(url.Values)

	var commandStatusQ string
	if o.CommandStatus != nil {
		commandStatusQ = *o.CommandStatus
	}
	if commandStatusQ != "" {
		qs.Set("commandStatus", commandStatusQ)
	}

	var fqdnQ string
	if o.Fqdn != nil {
		fqdnQ = *o.Fqdn
	}
	if fqdnQ != "" {
		qs.Set("fqdn", fqdnQ)
	}

	var pageSizeQ string
	if o.PageSize != nil {
		pageSizeQ = swag.FormatInt64(*o.PageSize)
	}
	if pageSizeQ != "" {
		qs.Set("pageSize", pageSizeQ)
	}

	var pageTokenQ string
	if o.PageToken != nil {
		pageTokenQ = *o.PageToken
	}
	if pageTokenQ != "" {
		qs.Set("pageToken", pageTokenQ)
	}

	var shipmentIDQ string
	if o.ShipmentID != nil {
		shipmentIDQ = *o.ShipmentID
	}
	if shipmentIDQ != "" {
		qs.Set("shipmentId", shipmentIDQ)
	}

	var sortOrderQ string
	if o.SortOrder != nil {
		sortOrderQ = *o.SortOrder
	}
	if sortOrderQ != "" {
		qs.Set("sortOrder", sortOrderQ)
	}

	_result.RawQuery = qs.Encode()

	return &_result, nil
}

// Must is a helper function to panic when the url builder returns an error
func (o *GetCommandsListURL) Must(u *url.URL, err error) *url.URL {
	if err != nil {
		panic(err)
	}
	if u == nil {
		panic("url can't be nil")
	}
	return u
}

// String returns the string representation of the path with query string
func (o *GetCommandsListURL) String() string {
	return o.Must(o.Build()).String()
}

// BuildFull builds a full url with scheme, host, path and query string
func (o *GetCommandsListURL) BuildFull(scheme, host string) (*url.URL, error) {
	if scheme == "" {
		return nil, errors.New("scheme is required for a full url on GetCommandsListURL")
	}
	if host == "" {
		return nil, errors.New("host is required for a full url on GetCommandsListURL")
	}

	base, err := o.Build()
	if err != nil {
		return nil, err
	}

	base.Scheme = scheme
	base.Host = host
	return base, nil
}

// StringFull returns the string representation of a complete url
func (o *GetCommandsListURL) StringFull(scheme, host string) string {
	return o.Must(o.BuildFull(scheme, host)).String()
}
