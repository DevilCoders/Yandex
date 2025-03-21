// Code generated by go-swagger; DO NOT EDIT.

package commands

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"net/http"

	"github.com/go-openapi/runtime"

	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/models"
)

// GetJobOKCode is the HTTP code returned for type GetJobOK
const GetJobOKCode int = 200

/*GetJobOK Job

swagger:response getJobOK
*/
type GetJobOK struct {

	/*
	  In: Body
	*/
	Payload *models.JobResp `json:"body,omitempty"`
}

// NewGetJobOK creates GetJobOK with default headers values
func NewGetJobOK() *GetJobOK {

	return &GetJobOK{}
}

// WithPayload adds the payload to the get job o k response
func (o *GetJobOK) WithPayload(payload *models.JobResp) *GetJobOK {
	o.Payload = payload
	return o
}

// SetPayload sets the payload to the get job o k response
func (o *GetJobOK) SetPayload(payload *models.JobResp) {
	o.Payload = payload
}

// WriteResponse to the client
func (o *GetJobOK) WriteResponse(rw http.ResponseWriter, producer runtime.Producer) {

	rw.WriteHeader(200)
	if o.Payload != nil {
		payload := o.Payload
		if err := producer.Produce(rw, payload); err != nil {
			panic(err) // let the recovery middleware deal with this
		}
	}
}

/*GetJobDefault Error

swagger:response getJobDefault
*/
type GetJobDefault struct {
	_statusCode int

	/*
	  In: Body
	*/
	Payload *models.Error `json:"body,omitempty"`
}

// NewGetJobDefault creates GetJobDefault with default headers values
func NewGetJobDefault(code int) *GetJobDefault {
	if code <= 0 {
		code = 500
	}

	return &GetJobDefault{
		_statusCode: code,
	}
}

// WithStatusCode adds the status to the get job default response
func (o *GetJobDefault) WithStatusCode(code int) *GetJobDefault {
	o._statusCode = code
	return o
}

// SetStatusCode sets the status to the get job default response
func (o *GetJobDefault) SetStatusCode(code int) {
	o._statusCode = code
}

// WithPayload adds the payload to the get job default response
func (o *GetJobDefault) WithPayload(payload *models.Error) *GetJobDefault {
	o.Payload = payload
	return o
}

// SetPayload sets the payload to the get job default response
func (o *GetJobDefault) SetPayload(payload *models.Error) {
	o.Payload = payload
}

// WriteResponse to the client
func (o *GetJobDefault) WriteResponse(rw http.ResponseWriter, producer runtime.Producer) {

	rw.WriteHeader(o._statusCode)
	if o.Payload != nil {
		payload := o.Payload
		if err := producer.Produce(rw, payload); err != nil {
			panic(err) // let the recovery middleware deal with this
		}
	}
}
