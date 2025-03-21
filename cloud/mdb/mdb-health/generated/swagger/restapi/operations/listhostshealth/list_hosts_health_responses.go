// Code generated by go-swagger; DO NOT EDIT.

package listhostshealth

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"net/http"

	"github.com/go-openapi/runtime"

	"a.yandex-team.ru/cloud/mdb/mdb-health/generated/swagger/models"
)

// ListHostsHealthOKCode is the HTTP code returned for type ListHostsHealthOK
const ListHostsHealthOKCode int = 200

/*ListHostsHealthOK Data for requested hosts

swagger:response listHostsHealthOK
*/
type ListHostsHealthOK struct {

	/*
	  In: Body
	*/
	Payload *models.HostsHealthResp `json:"body,omitempty"`
}

// NewListHostsHealthOK creates ListHostsHealthOK with default headers values
func NewListHostsHealthOK() *ListHostsHealthOK {

	return &ListHostsHealthOK{}
}

// WithPayload adds the payload to the list hosts health o k response
func (o *ListHostsHealthOK) WithPayload(payload *models.HostsHealthResp) *ListHostsHealthOK {
	o.Payload = payload
	return o
}

// SetPayload sets the payload to the list hosts health o k response
func (o *ListHostsHealthOK) SetPayload(payload *models.HostsHealthResp) {
	o.Payload = payload
}

// WriteResponse to the client
func (o *ListHostsHealthOK) WriteResponse(rw http.ResponseWriter, producer runtime.Producer) {

	rw.WriteHeader(200)
	if o.Payload != nil {
		payload := o.Payload
		if err := producer.Produce(rw, payload); err != nil {
			panic(err) // let the recovery middleware deal with this
		}
	}
}

/*ListHostsHealthDefault Error

swagger:response listHostsHealthDefault
*/
type ListHostsHealthDefault struct {
	_statusCode int

	/*
	  In: Body
	*/
	Payload *models.Error `json:"body,omitempty"`
}

// NewListHostsHealthDefault creates ListHostsHealthDefault with default headers values
func NewListHostsHealthDefault(code int) *ListHostsHealthDefault {
	if code <= 0 {
		code = 500
	}

	return &ListHostsHealthDefault{
		_statusCode: code,
	}
}

// WithStatusCode adds the status to the list hosts health default response
func (o *ListHostsHealthDefault) WithStatusCode(code int) *ListHostsHealthDefault {
	o._statusCode = code
	return o
}

// SetStatusCode sets the status to the list hosts health default response
func (o *ListHostsHealthDefault) SetStatusCode(code int) {
	o._statusCode = code
}

// WithPayload adds the payload to the list hosts health default response
func (o *ListHostsHealthDefault) WithPayload(payload *models.Error) *ListHostsHealthDefault {
	o.Payload = payload
	return o
}

// SetPayload sets the payload to the list hosts health default response
func (o *ListHostsHealthDefault) SetPayload(payload *models.Error) {
	o.Payload = payload
}

// WriteResponse to the client
func (o *ListHostsHealthDefault) WriteResponse(rw http.ResponseWriter, producer runtime.Producer) {

	rw.WriteHeader(o._statusCode)
	if o.Payload != nil {
		payload := o.Payload
		if err := producer.Produce(rw, payload); err != nil {
			panic(err) // let the recovery middleware deal with this
		}
	}
}
