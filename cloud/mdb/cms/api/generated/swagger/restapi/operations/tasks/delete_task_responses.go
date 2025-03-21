// Code generated by go-swagger; DO NOT EDIT.

package tasks

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"net/http"

	"github.com/go-openapi/runtime"

	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
)

// DeleteTaskNoContentCode is the HTTP code returned for type DeleteTaskNoContent
const DeleteTaskNoContentCode int = 204

/*DeleteTaskNoContent Deleted successfully

swagger:response deleteTaskNoContent
*/
type DeleteTaskNoContent struct {
}

// NewDeleteTaskNoContent creates DeleteTaskNoContent with default headers values
func NewDeleteTaskNoContent() *DeleteTaskNoContent {

	return &DeleteTaskNoContent{}
}

// WriteResponse to the client
func (o *DeleteTaskNoContent) WriteResponse(rw http.ResponseWriter, producer runtime.Producer) {

	rw.Header().Del(runtime.HeaderContentType) //Remove Content-Type on empty responses

	rw.WriteHeader(204)
}

/*DeleteTaskDefault Error

swagger:response deleteTaskDefault
*/
type DeleteTaskDefault struct {
	_statusCode int

	/*
	  In: Body
	*/
	Payload *models.Error `json:"body,omitempty"`
}

// NewDeleteTaskDefault creates DeleteTaskDefault with default headers values
func NewDeleteTaskDefault(code int) *DeleteTaskDefault {
	if code <= 0 {
		code = 500
	}

	return &DeleteTaskDefault{
		_statusCode: code,
	}
}

// WithStatusCode adds the status to the delete task default response
func (o *DeleteTaskDefault) WithStatusCode(code int) *DeleteTaskDefault {
	o._statusCode = code
	return o
}

// SetStatusCode sets the status to the delete task default response
func (o *DeleteTaskDefault) SetStatusCode(code int) {
	o._statusCode = code
}

// WithPayload adds the payload to the delete task default response
func (o *DeleteTaskDefault) WithPayload(payload *models.Error) *DeleteTaskDefault {
	o.Payload = payload
	return o
}

// SetPayload sets the payload to the delete task default response
func (o *DeleteTaskDefault) SetPayload(payload *models.Error) {
	o.Payload = payload
}

// WriteResponse to the client
func (o *DeleteTaskDefault) WriteResponse(rw http.ResponseWriter, producer runtime.Producer) {

	rw.WriteHeader(o._statusCode)
	if o.Payload != nil {
		payload := o.Payload
		if err := producer.Produce(rw, payload); err != nil {
			panic(err) // let the recovery middleware deal with this
		}
	}
}
