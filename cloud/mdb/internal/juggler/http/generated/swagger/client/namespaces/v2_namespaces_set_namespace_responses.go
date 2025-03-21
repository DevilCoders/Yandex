// Code generated by go-swagger; DO NOT EDIT.

package namespaces

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"
	"io"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/models"
)

// V2NamespacesSetNamespaceReader is a Reader for the V2NamespacesSetNamespace structure.
type V2NamespacesSetNamespaceReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *V2NamespacesSetNamespaceReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewV2NamespacesSetNamespaceOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	case 400:
		result := NewV2NamespacesSetNamespaceBadRequest()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	default:
		result := NewV2NamespacesSetNamespaceDefault(response.Code())
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		if response.Code()/100 == 2 {
			return result, nil
		}
		return nil, result
	}
}

// NewV2NamespacesSetNamespaceOK creates a V2NamespacesSetNamespaceOK with default headers values
func NewV2NamespacesSetNamespaceOK() *V2NamespacesSetNamespaceOK {
	return &V2NamespacesSetNamespaceOK{}
}

/* V2NamespacesSetNamespaceOK describes a response with status code 200, with default header values.

V2NamespacesSetNamespaceOK v2 namespaces set namespace o k
*/
type V2NamespacesSetNamespaceOK struct {
	Payload *models.V2NamespacesSetNamespaceOKBody
}

func (o *V2NamespacesSetNamespaceOK) Error() string {
	return fmt.Sprintf("[POST /v2/namespaces/set_namespace][%d] v2NamespacesSetNamespaceOK  %+v", 200, o.Payload)
}
func (o *V2NamespacesSetNamespaceOK) GetPayload() *models.V2NamespacesSetNamespaceOKBody {
	return o.Payload
}

func (o *V2NamespacesSetNamespaceOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2NamespacesSetNamespaceOKBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewV2NamespacesSetNamespaceBadRequest creates a V2NamespacesSetNamespaceBadRequest with default headers values
func NewV2NamespacesSetNamespaceBadRequest() *V2NamespacesSetNamespaceBadRequest {
	return &V2NamespacesSetNamespaceBadRequest{}
}

/* V2NamespacesSetNamespaceBadRequest describes a response with status code 400, with default header values.

V2NamespacesSetNamespaceBadRequest v2 namespaces set namespace bad request
*/
type V2NamespacesSetNamespaceBadRequest struct {
	Payload *models.V2NamespacesSetNamespaceBadRequestBody
}

func (o *V2NamespacesSetNamespaceBadRequest) Error() string {
	return fmt.Sprintf("[POST /v2/namespaces/set_namespace][%d] v2NamespacesSetNamespaceBadRequest  %+v", 400, o.Payload)
}
func (o *V2NamespacesSetNamespaceBadRequest) GetPayload() *models.V2NamespacesSetNamespaceBadRequestBody {
	return o.Payload
}

func (o *V2NamespacesSetNamespaceBadRequest) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2NamespacesSetNamespaceBadRequestBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewV2NamespacesSetNamespaceDefault creates a V2NamespacesSetNamespaceDefault with default headers values
func NewV2NamespacesSetNamespaceDefault(code int) *V2NamespacesSetNamespaceDefault {
	return &V2NamespacesSetNamespaceDefault{
		_statusCode: code,
	}
}

/* V2NamespacesSetNamespaceDefault describes a response with status code -1, with default header values.

V2NamespacesSetNamespaceDefault v2 namespaces set namespace default
*/
type V2NamespacesSetNamespaceDefault struct {
	_statusCode int

	Payload *models.V2NamespacesSetNamespaceDefaultBody
}

// Code gets the status code for the v2 namespaces set namespace default response
func (o *V2NamespacesSetNamespaceDefault) Code() int {
	return o._statusCode
}

func (o *V2NamespacesSetNamespaceDefault) Error() string {
	return fmt.Sprintf("[POST /v2/namespaces/set_namespace][%d] /v2/namespaces/set_namespace default  %+v", o._statusCode, o.Payload)
}
func (o *V2NamespacesSetNamespaceDefault) GetPayload() *models.V2NamespacesSetNamespaceDefaultBody {
	return o.Payload
}

func (o *V2NamespacesSetNamespaceDefault) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2NamespacesSetNamespaceDefaultBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
