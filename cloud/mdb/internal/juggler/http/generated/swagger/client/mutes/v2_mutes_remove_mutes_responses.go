// Code generated by go-swagger; DO NOT EDIT.

package mutes

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"
	"io"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/models"
)

// V2MutesRemoveMutesReader is a Reader for the V2MutesRemoveMutes structure.
type V2MutesRemoveMutesReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *V2MutesRemoveMutesReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewV2MutesRemoveMutesOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	case 400:
		result := NewV2MutesRemoveMutesBadRequest()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	default:
		result := NewV2MutesRemoveMutesDefault(response.Code())
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		if response.Code()/100 == 2 {
			return result, nil
		}
		return nil, result
	}
}

// NewV2MutesRemoveMutesOK creates a V2MutesRemoveMutesOK with default headers values
func NewV2MutesRemoveMutesOK() *V2MutesRemoveMutesOK {
	return &V2MutesRemoveMutesOK{}
}

/* V2MutesRemoveMutesOK describes a response with status code 200, with default header values.

V2MutesRemoveMutesOK v2 mutes remove mutes o k
*/
type V2MutesRemoveMutesOK struct {
	Payload *models.V2MutesRemoveMutesOKBody
}

func (o *V2MutesRemoveMutesOK) Error() string {
	return fmt.Sprintf("[POST /v2/mutes/remove_mutes][%d] v2MutesRemoveMutesOK  %+v", 200, o.Payload)
}
func (o *V2MutesRemoveMutesOK) GetPayload() *models.V2MutesRemoveMutesOKBody {
	return o.Payload
}

func (o *V2MutesRemoveMutesOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2MutesRemoveMutesOKBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewV2MutesRemoveMutesBadRequest creates a V2MutesRemoveMutesBadRequest with default headers values
func NewV2MutesRemoveMutesBadRequest() *V2MutesRemoveMutesBadRequest {
	return &V2MutesRemoveMutesBadRequest{}
}

/* V2MutesRemoveMutesBadRequest describes a response with status code 400, with default header values.

V2MutesRemoveMutesBadRequest v2 mutes remove mutes bad request
*/
type V2MutesRemoveMutesBadRequest struct {
	Payload *models.V2MutesRemoveMutesBadRequestBody
}

func (o *V2MutesRemoveMutesBadRequest) Error() string {
	return fmt.Sprintf("[POST /v2/mutes/remove_mutes][%d] v2MutesRemoveMutesBadRequest  %+v", 400, o.Payload)
}
func (o *V2MutesRemoveMutesBadRequest) GetPayload() *models.V2MutesRemoveMutesBadRequestBody {
	return o.Payload
}

func (o *V2MutesRemoveMutesBadRequest) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2MutesRemoveMutesBadRequestBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewV2MutesRemoveMutesDefault creates a V2MutesRemoveMutesDefault with default headers values
func NewV2MutesRemoveMutesDefault(code int) *V2MutesRemoveMutesDefault {
	return &V2MutesRemoveMutesDefault{
		_statusCode: code,
	}
}

/* V2MutesRemoveMutesDefault describes a response with status code -1, with default header values.

V2MutesRemoveMutesDefault v2 mutes remove mutes default
*/
type V2MutesRemoveMutesDefault struct {
	_statusCode int

	Payload *models.V2MutesRemoveMutesDefaultBody
}

// Code gets the status code for the v2 mutes remove mutes default response
func (o *V2MutesRemoveMutesDefault) Code() int {
	return o._statusCode
}

func (o *V2MutesRemoveMutesDefault) Error() string {
	return fmt.Sprintf("[POST /v2/mutes/remove_mutes][%d] /v2/mutes/remove_mutes default  %+v", o._statusCode, o.Payload)
}
func (o *V2MutesRemoveMutesDefault) GetPayload() *models.V2MutesRemoveMutesDefaultBody {
	return o.Payload
}

func (o *V2MutesRemoveMutesDefault) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2MutesRemoveMutesDefaultBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
