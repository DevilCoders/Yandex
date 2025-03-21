// Code generated by go-swagger; DO NOT EDIT.

package dns

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"
	"io"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/mdb-dns/generated/swagger/models"
)

// UpdatePrimaryDNSReader is a Reader for the UpdatePrimaryDNS structure.
type UpdatePrimaryDNSReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *UpdatePrimaryDNSReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 202:
		result := NewUpdatePrimaryDNSAccepted()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	case 204:
		result := NewUpdatePrimaryDNSNoContent()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	case 400:
		result := NewUpdatePrimaryDNSBadRequest()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	case 403:
		result := NewUpdatePrimaryDNSForbidden()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	case 500:
		result := NewUpdatePrimaryDNSInternalServerError()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	case 503:
		result := NewUpdatePrimaryDNSServiceUnavailable()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	case 504:
		result := NewUpdatePrimaryDNSGatewayTimeout()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	default:
		return nil, runtime.NewAPIError("response status code does not match any response statuses defined for this endpoint in the swagger spec", response, response.Code())
	}
}

// NewUpdatePrimaryDNSAccepted creates a UpdatePrimaryDNSAccepted with default headers values
func NewUpdatePrimaryDNSAccepted() *UpdatePrimaryDNSAccepted {
	return &UpdatePrimaryDNSAccepted{}
}

/* UpdatePrimaryDNSAccepted describes a response with status code 202, with default header values.

DNS names applied to update
*/
type UpdatePrimaryDNSAccepted struct {
}

func (o *UpdatePrimaryDNSAccepted) Error() string {
	return fmt.Sprintf("[PUT /v1/dns][%d] updatePrimaryDnsAccepted ", 202)
}

func (o *UpdatePrimaryDNSAccepted) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	return nil
}

// NewUpdatePrimaryDNSNoContent creates a UpdatePrimaryDNSNoContent with default headers values
func NewUpdatePrimaryDNSNoContent() *UpdatePrimaryDNSNoContent {
	return &UpdatePrimaryDNSNoContent{}
}

/* UpdatePrimaryDNSNoContent describes a response with status code 204, with default header values.

No update required for cid
*/
type UpdatePrimaryDNSNoContent struct {
}

func (o *UpdatePrimaryDNSNoContent) Error() string {
	return fmt.Sprintf("[PUT /v1/dns][%d] updatePrimaryDnsNoContent ", 204)
}

func (o *UpdatePrimaryDNSNoContent) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	return nil
}

// NewUpdatePrimaryDNSBadRequest creates a UpdatePrimaryDNSBadRequest with default headers values
func NewUpdatePrimaryDNSBadRequest() *UpdatePrimaryDNSBadRequest {
	return &UpdatePrimaryDNSBadRequest{}
}

/* UpdatePrimaryDNSBadRequest describes a response with status code 400, with default header values.

Invalid data supplied
*/
type UpdatePrimaryDNSBadRequest struct {
	Payload *models.Error
}

func (o *UpdatePrimaryDNSBadRequest) Error() string {
	return fmt.Sprintf("[PUT /v1/dns][%d] updatePrimaryDnsBadRequest  %+v", 400, o.Payload)
}
func (o *UpdatePrimaryDNSBadRequest) GetPayload() *models.Error {
	return o.Payload
}

func (o *UpdatePrimaryDNSBadRequest) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.Error)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewUpdatePrimaryDNSForbidden creates a UpdatePrimaryDNSForbidden with default headers values
func NewUpdatePrimaryDNSForbidden() *UpdatePrimaryDNSForbidden {
	return &UpdatePrimaryDNSForbidden{}
}

/* UpdatePrimaryDNSForbidden describes a response with status code 403, with default header values.

Invalid data signature
*/
type UpdatePrimaryDNSForbidden struct {
	Payload *models.Error
}

func (o *UpdatePrimaryDNSForbidden) Error() string {
	return fmt.Sprintf("[PUT /v1/dns][%d] updatePrimaryDnsForbidden  %+v", 403, o.Payload)
}
func (o *UpdatePrimaryDNSForbidden) GetPayload() *models.Error {
	return o.Payload
}

func (o *UpdatePrimaryDNSForbidden) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.Error)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewUpdatePrimaryDNSInternalServerError creates a UpdatePrimaryDNSInternalServerError with default headers values
func NewUpdatePrimaryDNSInternalServerError() *UpdatePrimaryDNSInternalServerError {
	return &UpdatePrimaryDNSInternalServerError{}
}

/* UpdatePrimaryDNSInternalServerError describes a response with status code 500, with default header values.

Internal server error
*/
type UpdatePrimaryDNSInternalServerError struct {
	Payload *models.Error
}

func (o *UpdatePrimaryDNSInternalServerError) Error() string {
	return fmt.Sprintf("[PUT /v1/dns][%d] updatePrimaryDnsInternalServerError  %+v", 500, o.Payload)
}
func (o *UpdatePrimaryDNSInternalServerError) GetPayload() *models.Error {
	return o.Payload
}

func (o *UpdatePrimaryDNSInternalServerError) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.Error)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewUpdatePrimaryDNSServiceUnavailable creates a UpdatePrimaryDNSServiceUnavailable with default headers values
func NewUpdatePrimaryDNSServiceUnavailable() *UpdatePrimaryDNSServiceUnavailable {
	return &UpdatePrimaryDNSServiceUnavailable{}
}

/* UpdatePrimaryDNSServiceUnavailable describes a response with status code 503, with default header values.

Service unavailable (database or other dependency is offline)
*/
type UpdatePrimaryDNSServiceUnavailable struct {
	Payload *models.Error
}

func (o *UpdatePrimaryDNSServiceUnavailable) Error() string {
	return fmt.Sprintf("[PUT /v1/dns][%d] updatePrimaryDnsServiceUnavailable  %+v", 503, o.Payload)
}
func (o *UpdatePrimaryDNSServiceUnavailable) GetPayload() *models.Error {
	return o.Payload
}

func (o *UpdatePrimaryDNSServiceUnavailable) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.Error)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewUpdatePrimaryDNSGatewayTimeout creates a UpdatePrimaryDNSGatewayTimeout with default headers values
func NewUpdatePrimaryDNSGatewayTimeout() *UpdatePrimaryDNSGatewayTimeout {
	return &UpdatePrimaryDNSGatewayTimeout{}
}

/* UpdatePrimaryDNSGatewayTimeout describes a response with status code 504, with default header values.

Gateway Timeout (resolve current cid cname record failed)
*/
type UpdatePrimaryDNSGatewayTimeout struct {
	Payload *models.Error
}

func (o *UpdatePrimaryDNSGatewayTimeout) Error() string {
	return fmt.Sprintf("[PUT /v1/dns][%d] updatePrimaryDnsGatewayTimeout  %+v", 504, o.Payload)
}
func (o *UpdatePrimaryDNSGatewayTimeout) GetPayload() *models.Error {
	return o.Payload
}

func (o *UpdatePrimaryDNSGatewayTimeout) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.Error)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
