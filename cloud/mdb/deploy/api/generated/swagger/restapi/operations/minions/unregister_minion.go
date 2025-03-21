// Code generated by go-swagger; DO NOT EDIT.

package minions

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the generate command

import (
	"net/http"

	"github.com/go-openapi/runtime/middleware"
)

// UnregisterMinionHandlerFunc turns a function with the right signature into a unregister minion handler
type UnregisterMinionHandlerFunc func(UnregisterMinionParams) middleware.Responder

// Handle executing the request and returning a response
func (fn UnregisterMinionHandlerFunc) Handle(params UnregisterMinionParams) middleware.Responder {
	return fn(params)
}

// UnregisterMinionHandler interface for that can handle valid unregister minion params
type UnregisterMinionHandler interface {
	Handle(UnregisterMinionParams) middleware.Responder
}

// NewUnregisterMinion creates a new http.Handler for the unregister minion operation
func NewUnregisterMinion(ctx *middleware.Context, handler UnregisterMinionHandler) *UnregisterMinion {
	return &UnregisterMinion{Context: ctx, Handler: handler}
}

/* UnregisterMinion swagger:route POST /v1/minions/{fqdn}/unregister minions unregisterMinion

Unregisters minion's public key. Minion can be registered again as if it was just created.

*/
type UnregisterMinion struct {
	Context *middleware.Context
	Handler UnregisterMinionHandler
}

func (o *UnregisterMinion) ServeHTTP(rw http.ResponseWriter, r *http.Request) {
	route, rCtx, _ := o.Context.RouteInfo(r)
	if rCtx != nil {
		*r = *rCtx
	}
	var Params = NewUnregisterMinionParams()
	if err := o.Context.BindValidRequest(r, route, &Params); err != nil { // bind params
		o.Context.Respond(rw, r, route.Produces, route, err)
		return
	}

	res := o.Handler.Handle(Params) // actually handle the request
	o.Context.Respond(rw, r, route.Produces, route, res)

}
