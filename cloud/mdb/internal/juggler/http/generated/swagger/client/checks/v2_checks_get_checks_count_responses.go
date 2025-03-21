// Code generated by go-swagger; DO NOT EDIT.

package checks

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"
	"io"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/models"
)

// V2ChecksGetChecksCountReader is a Reader for the V2ChecksGetChecksCount structure.
type V2ChecksGetChecksCountReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *V2ChecksGetChecksCountReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewV2ChecksGetChecksCountOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	case 400:
		result := NewV2ChecksGetChecksCountBadRequest()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	default:
		result := NewV2ChecksGetChecksCountDefault(response.Code())
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		if response.Code()/100 == 2 {
			return result, nil
		}
		return nil, result
	}
}

// NewV2ChecksGetChecksCountOK creates a V2ChecksGetChecksCountOK with default headers values
func NewV2ChecksGetChecksCountOK() *V2ChecksGetChecksCountOK {
	return &V2ChecksGetChecksCountOK{}
}

/* V2ChecksGetChecksCountOK describes a response with status code 200, with default header values.

V2ChecksGetChecksCountOK v2 checks get checks count o k
*/
type V2ChecksGetChecksCountOK struct {
	Payload *models.V2ChecksGetChecksCountOKBody
}

func (o *V2ChecksGetChecksCountOK) Error() string {
	return fmt.Sprintf("[POST /v2/checks/get_checks_count][%d] v2ChecksGetChecksCountOK  %+v", 200, o.Payload)
}
func (o *V2ChecksGetChecksCountOK) GetPayload() *models.V2ChecksGetChecksCountOKBody {
	return o.Payload
}

func (o *V2ChecksGetChecksCountOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2ChecksGetChecksCountOKBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewV2ChecksGetChecksCountBadRequest creates a V2ChecksGetChecksCountBadRequest with default headers values
func NewV2ChecksGetChecksCountBadRequest() *V2ChecksGetChecksCountBadRequest {
	return &V2ChecksGetChecksCountBadRequest{}
}

/* V2ChecksGetChecksCountBadRequest describes a response with status code 400, with default header values.

V2ChecksGetChecksCountBadRequest v2 checks get checks count bad request
*/
type V2ChecksGetChecksCountBadRequest struct {
	Payload *models.V2ChecksGetChecksCountBadRequestBody
}

func (o *V2ChecksGetChecksCountBadRequest) Error() string {
	return fmt.Sprintf("[POST /v2/checks/get_checks_count][%d] v2ChecksGetChecksCountBadRequest  %+v", 400, o.Payload)
}
func (o *V2ChecksGetChecksCountBadRequest) GetPayload() *models.V2ChecksGetChecksCountBadRequestBody {
	return o.Payload
}

func (o *V2ChecksGetChecksCountBadRequest) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2ChecksGetChecksCountBadRequestBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewV2ChecksGetChecksCountDefault creates a V2ChecksGetChecksCountDefault with default headers values
func NewV2ChecksGetChecksCountDefault(code int) *V2ChecksGetChecksCountDefault {
	return &V2ChecksGetChecksCountDefault{
		_statusCode: code,
	}
}

/* V2ChecksGetChecksCountDefault describes a response with status code -1, with default header values.

V2ChecksGetChecksCountDefault v2 checks get checks count default
*/
type V2ChecksGetChecksCountDefault struct {
	_statusCode int

	Payload *models.V2ChecksGetChecksCountDefaultBody
}

// Code gets the status code for the v2 checks get checks count default response
func (o *V2ChecksGetChecksCountDefault) Code() int {
	return o._statusCode
}

func (o *V2ChecksGetChecksCountDefault) Error() string {
	return fmt.Sprintf("[POST /v2/checks/get_checks_count][%d] /v2/checks/get_checks_count default  %+v", o._statusCode, o.Payload)
}
func (o *V2ChecksGetChecksCountDefault) GetPayload() *models.V2ChecksGetChecksCountDefaultBody {
	return o.Payload
}

func (o *V2ChecksGetChecksCountDefault) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2ChecksGetChecksCountDefaultBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
