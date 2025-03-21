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

// V2BookmarksGetBookmarksReader is a Reader for the V2BookmarksGetBookmarks structure.
type V2BookmarksGetBookmarksReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *V2BookmarksGetBookmarksReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewV2BookmarksGetBookmarksOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	case 400:
		result := NewV2BookmarksGetBookmarksBadRequest()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	default:
		result := NewV2BookmarksGetBookmarksDefault(response.Code())
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		if response.Code()/100 == 2 {
			return result, nil
		}
		return nil, result
	}
}

// NewV2BookmarksGetBookmarksOK creates a V2BookmarksGetBookmarksOK with default headers values
func NewV2BookmarksGetBookmarksOK() *V2BookmarksGetBookmarksOK {
	return &V2BookmarksGetBookmarksOK{}
}

/* V2BookmarksGetBookmarksOK describes a response with status code 200, with default header values.

V2BookmarksGetBookmarksOK v2 bookmarks get bookmarks o k
*/
type V2BookmarksGetBookmarksOK struct {
	Payload *models.V2BookmarksGetBookmarksOKBody
}

func (o *V2BookmarksGetBookmarksOK) Error() string {
	return fmt.Sprintf("[POST /v2/bookmarks/get_bookmarks][%d] v2BookmarksGetBookmarksOK  %+v", 200, o.Payload)
}
func (o *V2BookmarksGetBookmarksOK) GetPayload() *models.V2BookmarksGetBookmarksOKBody {
	return o.Payload
}

func (o *V2BookmarksGetBookmarksOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2BookmarksGetBookmarksOKBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewV2BookmarksGetBookmarksBadRequest creates a V2BookmarksGetBookmarksBadRequest with default headers values
func NewV2BookmarksGetBookmarksBadRequest() *V2BookmarksGetBookmarksBadRequest {
	return &V2BookmarksGetBookmarksBadRequest{}
}

/* V2BookmarksGetBookmarksBadRequest describes a response with status code 400, with default header values.

V2BookmarksGetBookmarksBadRequest v2 bookmarks get bookmarks bad request
*/
type V2BookmarksGetBookmarksBadRequest struct {
	Payload *models.V2BookmarksGetBookmarksBadRequestBody
}

func (o *V2BookmarksGetBookmarksBadRequest) Error() string {
	return fmt.Sprintf("[POST /v2/bookmarks/get_bookmarks][%d] v2BookmarksGetBookmarksBadRequest  %+v", 400, o.Payload)
}
func (o *V2BookmarksGetBookmarksBadRequest) GetPayload() *models.V2BookmarksGetBookmarksBadRequestBody {
	return o.Payload
}

func (o *V2BookmarksGetBookmarksBadRequest) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2BookmarksGetBookmarksBadRequestBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewV2BookmarksGetBookmarksDefault creates a V2BookmarksGetBookmarksDefault with default headers values
func NewV2BookmarksGetBookmarksDefault(code int) *V2BookmarksGetBookmarksDefault {
	return &V2BookmarksGetBookmarksDefault{
		_statusCode: code,
	}
}

/* V2BookmarksGetBookmarksDefault describes a response with status code -1, with default header values.

V2BookmarksGetBookmarksDefault v2 bookmarks get bookmarks default
*/
type V2BookmarksGetBookmarksDefault struct {
	_statusCode int

	Payload *models.V2BookmarksGetBookmarksDefaultBody
}

// Code gets the status code for the v2 bookmarks get bookmarks default response
func (o *V2BookmarksGetBookmarksDefault) Code() int {
	return o._statusCode
}

func (o *V2BookmarksGetBookmarksDefault) Error() string {
	return fmt.Sprintf("[POST /v2/bookmarks/get_bookmarks][%d] /v2/bookmarks/get_bookmarks default  %+v", o._statusCode, o.Payload)
}
func (o *V2BookmarksGetBookmarksDefault) GetPayload() *models.V2BookmarksGetBookmarksDefaultBody {
	return o.Payload
}

func (o *V2BookmarksGetBookmarksDefault) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2BookmarksGetBookmarksDefaultBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
