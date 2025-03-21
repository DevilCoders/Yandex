// Code generated by go-swagger; DO NOT EDIT.

package masters

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the generate command

import (
	"net/http"

	"github.com/go-openapi/runtime/middleware"
)

// DeleteMasterHandlerFunc turns a function with the right signature into a delete master handler
type DeleteMasterHandlerFunc func(DeleteMasterParams) middleware.Responder

// Handle executing the request and returning a response
func (fn DeleteMasterHandlerFunc) Handle(params DeleteMasterParams) middleware.Responder {
	return fn(params)
}

// DeleteMasterHandler interface for that can handle valid delete master params
type DeleteMasterHandler interface {
	Handle(DeleteMasterParams) middleware.Responder
}

// NewDeleteMaster creates a new http.Handler for the delete master operation
func NewDeleteMaster(ctx *middleware.Context, handler DeleteMasterHandler) *DeleteMaster {
	return &DeleteMaster{Context: ctx, Handler: handler}
}

/* DeleteMaster swagger:route DELETE /v1/masters/{fqdn} masters deleteMaster

Deletes master

*/
type DeleteMaster struct {
	Context *middleware.Context
	Handler DeleteMasterHandler
}

func (o *DeleteMaster) ServeHTTP(rw http.ResponseWriter, r *http.Request) {
	route, rCtx, _ := o.Context.RouteInfo(r)
	if rCtx != nil {
		*r = *rCtx
	}
	var Params = NewDeleteMasterParams()
	if err := o.Context.BindValidRequest(r, route, &Params); err != nil { // bind params
		o.Context.Respond(rw, r, route.Produces, route, err)
		return
	}

	res := o.Handler.Handle(Params) // actually handle the request
	o.Context.Respond(rw, r, route.Produces, route, res)

}
