// Code generated by go-swagger; DO NOT EDIT.

package s_s_l_certificates

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"
	"io"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/internal/crt/cloudcrt/generated/swagger/models"
)

// GetAPICertificateIDReader is a Reader for the GetAPICertificateID structure.
type GetAPICertificateIDReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *GetAPICertificateIDReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewGetAPICertificateIDOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	case 403:
		result := NewGetAPICertificateIDForbidden()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	case 404:
		result := NewGetAPICertificateIDNotFound()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	case 500:
		result := NewGetAPICertificateIDInternalServerError()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	default:
		return nil, runtime.NewAPIError("response status code does not match any response statuses defined for this endpoint in the swagger spec", response, response.Code())
	}
}

// NewGetAPICertificateIDOK creates a GetAPICertificateIDOK with default headers values
func NewGetAPICertificateIDOK() *GetAPICertificateIDOK {
	return &GetAPICertificateIDOK{}
}

/* GetAPICertificateIDOK describes a response with status code 200, with default header values.

OK
*/
type GetAPICertificateIDOK struct {
	Payload *models.DbCertInfo
}

func (o *GetAPICertificateIDOK) Error() string {
	return fmt.Sprintf("[GET /api/certificate/{id}/][%d] getApiCertificateIdOK  %+v", 200, o.Payload)
}
func (o *GetAPICertificateIDOK) GetPayload() *models.DbCertInfo {
	return o.Payload
}

func (o *GetAPICertificateIDOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.DbCertInfo)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewGetAPICertificateIDForbidden creates a GetAPICertificateIDForbidden with default headers values
func NewGetAPICertificateIDForbidden() *GetAPICertificateIDForbidden {
	return &GetAPICertificateIDForbidden{}
}

/* GetAPICertificateIDForbidden describes a response with status code 403, with default header values.

Forbidden
*/
type GetAPICertificateIDForbidden struct {
	Payload *models.RequestsErrorResponse
}

func (o *GetAPICertificateIDForbidden) Error() string {
	return fmt.Sprintf("[GET /api/certificate/{id}/][%d] getApiCertificateIdForbidden  %+v", 403, o.Payload)
}
func (o *GetAPICertificateIDForbidden) GetPayload() *models.RequestsErrorResponse {
	return o.Payload
}

func (o *GetAPICertificateIDForbidden) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.RequestsErrorResponse)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewGetAPICertificateIDNotFound creates a GetAPICertificateIDNotFound with default headers values
func NewGetAPICertificateIDNotFound() *GetAPICertificateIDNotFound {
	return &GetAPICertificateIDNotFound{}
}

/* GetAPICertificateIDNotFound describes a response with status code 404, with default header values.

Not Found
*/
type GetAPICertificateIDNotFound struct {
	Payload *models.RequestsErrorResponse
}

func (o *GetAPICertificateIDNotFound) Error() string {
	return fmt.Sprintf("[GET /api/certificate/{id}/][%d] getApiCertificateIdNotFound  %+v", 404, o.Payload)
}
func (o *GetAPICertificateIDNotFound) GetPayload() *models.RequestsErrorResponse {
	return o.Payload
}

func (o *GetAPICertificateIDNotFound) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.RequestsErrorResponse)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewGetAPICertificateIDInternalServerError creates a GetAPICertificateIDInternalServerError with default headers values
func NewGetAPICertificateIDInternalServerError() *GetAPICertificateIDInternalServerError {
	return &GetAPICertificateIDInternalServerError{}
}

/* GetAPICertificateIDInternalServerError describes a response with status code 500, with default header values.

Internal Server Error
*/
type GetAPICertificateIDInternalServerError struct {
	Payload *models.RequestsErrorResponse
}

func (o *GetAPICertificateIDInternalServerError) Error() string {
	return fmt.Sprintf("[GET /api/certificate/{id}/][%d] getApiCertificateIdInternalServerError  %+v", 500, o.Payload)
}
func (o *GetAPICertificateIDInternalServerError) GetPayload() *models.RequestsErrorResponse {
	return o.Payload
}

func (o *GetAPICertificateIDInternalServerError) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.RequestsErrorResponse)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
