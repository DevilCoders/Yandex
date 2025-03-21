// Code generated by go-swagger; DO NOT EDIT.

package common

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"net/http"

	"github.com/go-openapi/runtime"

	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/models"
)

// PingOKCode is the HTTP code returned for type PingOK
const PingOKCode int = 200

/*PingOK Service is alive and well

swagger:response pingOK
*/
type PingOK struct {
}

// NewPingOK creates PingOK with default headers values
func NewPingOK() *PingOK {

	return &PingOK{}
}

// WriteResponse to the client
func (o *PingOK) WriteResponse(rw http.ResponseWriter, producer runtime.Producer) {

	rw.Header().Del(runtime.HeaderContentType) //Remove Content-Type on empty responses

	rw.WriteHeader(200)
}

/*PingDefault Error

swagger:response pingDefault
*/
type PingDefault struct {
	_statusCode int

	/*
	  In: Body
	*/
	Payload *models.Error `json:"body,omitempty"`
}

// NewPingDefault creates PingDefault with default headers values
func NewPingDefault(code int) *PingDefault {
	if code <= 0 {
		code = 500
	}

	return &PingDefault{
		_statusCode: code,
	}
}

// WithStatusCode adds the status to the ping default response
func (o *PingDefault) WithStatusCode(code int) *PingDefault {
	o._statusCode = code
	return o
}

// SetStatusCode sets the status to the ping default response
func (o *PingDefault) SetStatusCode(code int) {
	o._statusCode = code
}

// WithPayload adds the payload to the ping default response
func (o *PingDefault) WithPayload(payload *models.Error) *PingDefault {
	o.Payload = payload
	return o
}

// SetPayload sets the payload to the ping default response
func (o *PingDefault) SetPayload(payload *models.Error) {
	o.Payload = payload
}

// WriteResponse to the client
func (o *PingDefault) WriteResponse(rw http.ResponseWriter, producer runtime.Producer) {

	rw.WriteHeader(o._statusCode)
	if o.Payload != nil {
		payload := o.Payload
		if err := producer.Produce(rw, payload); err != nil {
			panic(err) // let the recovery middleware deal with this
		}
	}
}
