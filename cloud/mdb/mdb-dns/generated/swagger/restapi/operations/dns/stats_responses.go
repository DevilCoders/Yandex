// Code generated by go-swagger; DO NOT EDIT.

package dns

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"net/http"

	"github.com/go-openapi/runtime"

	"a.yandex-team.ru/cloud/mdb/mdb-dns/generated/swagger/models"
)

// StatsOKCode is the HTTP code returned for type StatsOK
const StatsOKCode int = 200

/*StatsOK Reports service stats

swagger:response statsOK
*/
type StatsOK struct {

	/*
	  In: Body
	*/
	Payload models.Stats `json:"body,omitempty"`
}

// NewStatsOK creates StatsOK with default headers values
func NewStatsOK() *StatsOK {

	return &StatsOK{}
}

// WithPayload adds the payload to the stats o k response
func (o *StatsOK) WithPayload(payload models.Stats) *StatsOK {
	o.Payload = payload
	return o
}

// SetPayload sets the payload to the stats o k response
func (o *StatsOK) SetPayload(payload models.Stats) {
	o.Payload = payload
}

// WriteResponse to the client
func (o *StatsOK) WriteResponse(rw http.ResponseWriter, producer runtime.Producer) {

	rw.WriteHeader(200)
	payload := o.Payload
	if payload == nil {
		// return empty array
		payload = models.Stats{}
	}

	if err := producer.Produce(rw, payload); err != nil {
		panic(err) // let the recovery middleware deal with this
	}
}

// StatsInternalServerErrorCode is the HTTP code returned for type StatsInternalServerError
const StatsInternalServerErrorCode int = 500

/*StatsInternalServerError Failed to gather metrics

swagger:response statsInternalServerError
*/
type StatsInternalServerError struct {

	/*
	  In: Body
	*/
	Payload *models.Error `json:"body,omitempty"`
}

// NewStatsInternalServerError creates StatsInternalServerError with default headers values
func NewStatsInternalServerError() *StatsInternalServerError {

	return &StatsInternalServerError{}
}

// WithPayload adds the payload to the stats internal server error response
func (o *StatsInternalServerError) WithPayload(payload *models.Error) *StatsInternalServerError {
	o.Payload = payload
	return o
}

// SetPayload sets the payload to the stats internal server error response
func (o *StatsInternalServerError) SetPayload(payload *models.Error) {
	o.Payload = payload
}

// WriteResponse to the client
func (o *StatsInternalServerError) WriteResponse(rw http.ResponseWriter, producer runtime.Producer) {

	rw.WriteHeader(500)
	if o.Payload != nil {
		payload := o.Payload
		if err := producer.Produce(rw, payload); err != nil {
			panic(err) // let the recovery middleware deal with this
		}
	}
}
