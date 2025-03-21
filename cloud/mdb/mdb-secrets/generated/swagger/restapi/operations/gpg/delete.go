// Code generated by go-swagger; DO NOT EDIT.

package gpg

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the generate command

import (
	"net/http"

	"github.com/go-openapi/runtime/middleware"
)

// DeleteHandlerFunc turns a function with the right signature into a delete handler
type DeleteHandlerFunc func(DeleteParams) middleware.Responder

// Handle executing the request and returning a response
func (fn DeleteHandlerFunc) Handle(params DeleteParams) middleware.Responder {
	return fn(params)
}

// DeleteHandler interface for that can handle valid delete params
type DeleteHandler interface {
	Handle(DeleteParams) middleware.Responder
}

// NewDelete creates a new http.Handler for the delete operation
func NewDelete(ctx *middleware.Context, handler DeleteHandler) *Delete {
	return &Delete{Context: ctx, Handler: handler}
}

/* Delete swagger:route DELETE /v1/gpg gpg delete

Delete gpg key for cluster

*/
type Delete struct {
	Context *middleware.Context
	Handler DeleteHandler
}

func (o *Delete) ServeHTTP(rw http.ResponseWriter, r *http.Request) {
	route, rCtx, _ := o.Context.RouteInfo(r)
	if rCtx != nil {
		*r = *rCtx
	}
	var Params = NewDeleteParams()
	if err := o.Context.BindValidRequest(r, route, &Params); err != nil { // bind params
		o.Context.Respond(rw, r, route.Produces, route, err)
		return
	}

	res := o.Handler.Handle(Params) // actually handle the request
	o.Context.Respond(rw, r, route.Produces, route, res)

}
