// Code generated by go-swagger; DO NOT EDIT.

package internal_swagger

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"
	"io"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/internal/juggler/http/generated/swagger/models"
)

// V2BookmarksRemoveBookmarkReader is a Reader for the V2BookmarksRemoveBookmark structure.
type V2BookmarksRemoveBookmarkReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *V2BookmarksRemoveBookmarkReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewV2BookmarksRemoveBookmarkOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	case 400:
		result := NewV2BookmarksRemoveBookmarkBadRequest()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	default:
		result := NewV2BookmarksRemoveBookmarkDefault(response.Code())
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		if response.Code()/100 == 2 {
			return result, nil
		}
		return nil, result
	}
}

// NewV2BookmarksRemoveBookmarkOK creates a V2BookmarksRemoveBookmarkOK with default headers values
func NewV2BookmarksRemoveBookmarkOK() *V2BookmarksRemoveBookmarkOK {
	return &V2BookmarksRemoveBookmarkOK{}
}

/* V2BookmarksRemoveBookmarkOK describes a response with status code 200, with default header values.

V2BookmarksRemoveBookmarkOK v2 bookmarks remove bookmark o k
*/
type V2BookmarksRemoveBookmarkOK struct {
	Payload *models.V2BookmarksRemoveBookmarkOKBody
}

func (o *V2BookmarksRemoveBookmarkOK) Error() string {
	return fmt.Sprintf("[POST /v2/bookmarks/remove_bookmark][%d] v2BookmarksRemoveBookmarkOK  %+v", 200, o.Payload)
}
func (o *V2BookmarksRemoveBookmarkOK) GetPayload() *models.V2BookmarksRemoveBookmarkOKBody {
	return o.Payload
}

func (o *V2BookmarksRemoveBookmarkOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2BookmarksRemoveBookmarkOKBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewV2BookmarksRemoveBookmarkBadRequest creates a V2BookmarksRemoveBookmarkBadRequest with default headers values
func NewV2BookmarksRemoveBookmarkBadRequest() *V2BookmarksRemoveBookmarkBadRequest {
	return &V2BookmarksRemoveBookmarkBadRequest{}
}

/* V2BookmarksRemoveBookmarkBadRequest describes a response with status code 400, with default header values.

V2BookmarksRemoveBookmarkBadRequest v2 bookmarks remove bookmark bad request
*/
type V2BookmarksRemoveBookmarkBadRequest struct {
	Payload *models.V2BookmarksRemoveBookmarkBadRequestBody
}

func (o *V2BookmarksRemoveBookmarkBadRequest) Error() string {
	return fmt.Sprintf("[POST /v2/bookmarks/remove_bookmark][%d] v2BookmarksRemoveBookmarkBadRequest  %+v", 400, o.Payload)
}
func (o *V2BookmarksRemoveBookmarkBadRequest) GetPayload() *models.V2BookmarksRemoveBookmarkBadRequestBody {
	return o.Payload
}

func (o *V2BookmarksRemoveBookmarkBadRequest) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2BookmarksRemoveBookmarkBadRequestBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewV2BookmarksRemoveBookmarkDefault creates a V2BookmarksRemoveBookmarkDefault with default headers values
func NewV2BookmarksRemoveBookmarkDefault(code int) *V2BookmarksRemoveBookmarkDefault {
	return &V2BookmarksRemoveBookmarkDefault{
		_statusCode: code,
	}
}

/* V2BookmarksRemoveBookmarkDefault describes a response with status code -1, with default header values.

V2BookmarksRemoveBookmarkDefault v2 bookmarks remove bookmark default
*/
type V2BookmarksRemoveBookmarkDefault struct {
	_statusCode int

	Payload *models.V2BookmarksRemoveBookmarkDefaultBody
}

// Code gets the status code for the v2 bookmarks remove bookmark default response
func (o *V2BookmarksRemoveBookmarkDefault) Code() int {
	return o._statusCode
}

func (o *V2BookmarksRemoveBookmarkDefault) Error() string {
	return fmt.Sprintf("[POST /v2/bookmarks/remove_bookmark][%d] /v2/bookmarks/remove_bookmark default  %+v", o._statusCode, o.Payload)
}
func (o *V2BookmarksRemoveBookmarkDefault) GetPayload() *models.V2BookmarksRemoveBookmarkDefaultBody {
	return o.Payload
}

func (o *V2BookmarksRemoveBookmarkDefault) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2BookmarksRemoveBookmarkDefaultBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
