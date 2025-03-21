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

// GetAPICertificateIDDownloadPfxReader is a Reader for the GetAPICertificateIDDownloadPfx structure.
type GetAPICertificateIDDownloadPfxReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *GetAPICertificateIDDownloadPfxReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 403:
		result := NewGetAPICertificateIDDownloadPfxForbidden()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	case 404:
		result := NewGetAPICertificateIDDownloadPfxNotFound()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	case 500:
		result := NewGetAPICertificateIDDownloadPfxInternalServerError()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	case 501:
		result := NewGetAPICertificateIDDownloadPfxNotImplemented()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	default:
		return nil, runtime.NewAPIError("response status code does not match any response statuses defined for this endpoint in the swagger spec", response, response.Code())
	}
}

// NewGetAPICertificateIDDownloadPfxForbidden creates a GetAPICertificateIDDownloadPfxForbidden with default headers values
func NewGetAPICertificateIDDownloadPfxForbidden() *GetAPICertificateIDDownloadPfxForbidden {
	return &GetAPICertificateIDDownloadPfxForbidden{}
}

/* GetAPICertificateIDDownloadPfxForbidden describes a response with status code 403, with default header values.

Forbidden
*/
type GetAPICertificateIDDownloadPfxForbidden struct {
	Payload *models.RequestsErrorResponse
}

func (o *GetAPICertificateIDDownloadPfxForbidden) Error() string {
	return fmt.Sprintf("[GET /api/certificate/{id}/download.pfx][%d] getApiCertificateIdDownloadPfxForbidden  %+v", 403, o.Payload)
}
func (o *GetAPICertificateIDDownloadPfxForbidden) GetPayload() *models.RequestsErrorResponse {
	return o.Payload
}

func (o *GetAPICertificateIDDownloadPfxForbidden) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.RequestsErrorResponse)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewGetAPICertificateIDDownloadPfxNotFound creates a GetAPICertificateIDDownloadPfxNotFound with default headers values
func NewGetAPICertificateIDDownloadPfxNotFound() *GetAPICertificateIDDownloadPfxNotFound {
	return &GetAPICertificateIDDownloadPfxNotFound{}
}

/* GetAPICertificateIDDownloadPfxNotFound describes a response with status code 404, with default header values.

Not Found
*/
type GetAPICertificateIDDownloadPfxNotFound struct {
	Payload *models.RequestsErrorResponse
}

func (o *GetAPICertificateIDDownloadPfxNotFound) Error() string {
	return fmt.Sprintf("[GET /api/certificate/{id}/download.pfx][%d] getApiCertificateIdDownloadPfxNotFound  %+v", 404, o.Payload)
}
func (o *GetAPICertificateIDDownloadPfxNotFound) GetPayload() *models.RequestsErrorResponse {
	return o.Payload
}

func (o *GetAPICertificateIDDownloadPfxNotFound) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.RequestsErrorResponse)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewGetAPICertificateIDDownloadPfxInternalServerError creates a GetAPICertificateIDDownloadPfxInternalServerError with default headers values
func NewGetAPICertificateIDDownloadPfxInternalServerError() *GetAPICertificateIDDownloadPfxInternalServerError {
	return &GetAPICertificateIDDownloadPfxInternalServerError{}
}

/* GetAPICertificateIDDownloadPfxInternalServerError describes a response with status code 500, with default header values.

Internal Server Error
*/
type GetAPICertificateIDDownloadPfxInternalServerError struct {
	Payload *models.RequestsErrorResponse
}

func (o *GetAPICertificateIDDownloadPfxInternalServerError) Error() string {
	return fmt.Sprintf("[GET /api/certificate/{id}/download.pfx][%d] getApiCertificateIdDownloadPfxInternalServerError  %+v", 500, o.Payload)
}
func (o *GetAPICertificateIDDownloadPfxInternalServerError) GetPayload() *models.RequestsErrorResponse {
	return o.Payload
}

func (o *GetAPICertificateIDDownloadPfxInternalServerError) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.RequestsErrorResponse)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewGetAPICertificateIDDownloadPfxNotImplemented creates a GetAPICertificateIDDownloadPfxNotImplemented with default headers values
func NewGetAPICertificateIDDownloadPfxNotImplemented() *GetAPICertificateIDDownloadPfxNotImplemented {
	return &GetAPICertificateIDDownloadPfxNotImplemented{}
}

/* GetAPICertificateIDDownloadPfxNotImplemented describes a response with status code 501, with default header values.

Not Implemented
*/
type GetAPICertificateIDDownloadPfxNotImplemented struct {
	Payload *models.RequestsErrorResponse
}

func (o *GetAPICertificateIDDownloadPfxNotImplemented) Error() string {
	return fmt.Sprintf("[GET /api/certificate/{id}/download.pfx][%d] getApiCertificateIdDownloadPfxNotImplemented  %+v", 501, o.Payload)
}
func (o *GetAPICertificateIDDownloadPfxNotImplemented) GetPayload() *models.RequestsErrorResponse {
	return o.Payload
}

func (o *GetAPICertificateIDDownloadPfxNotImplemented) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.RequestsErrorResponse)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
