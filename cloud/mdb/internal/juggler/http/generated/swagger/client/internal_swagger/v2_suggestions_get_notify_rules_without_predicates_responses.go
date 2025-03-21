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

// V2SuggestionsGetNotifyRulesWithoutPredicatesReader is a Reader for the V2SuggestionsGetNotifyRulesWithoutPredicates structure.
type V2SuggestionsGetNotifyRulesWithoutPredicatesReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *V2SuggestionsGetNotifyRulesWithoutPredicatesReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewV2SuggestionsGetNotifyRulesWithoutPredicatesOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	case 400:
		result := NewV2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return nil, result
	default:
		result := NewV2SuggestionsGetNotifyRulesWithoutPredicatesDefault(response.Code())
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		if response.Code()/100 == 2 {
			return result, nil
		}
		return nil, result
	}
}

// NewV2SuggestionsGetNotifyRulesWithoutPredicatesOK creates a V2SuggestionsGetNotifyRulesWithoutPredicatesOK with default headers values
func NewV2SuggestionsGetNotifyRulesWithoutPredicatesOK() *V2SuggestionsGetNotifyRulesWithoutPredicatesOK {
	return &V2SuggestionsGetNotifyRulesWithoutPredicatesOK{}
}

/* V2SuggestionsGetNotifyRulesWithoutPredicatesOK describes a response with status code 200, with default header values.

V2SuggestionsGetNotifyRulesWithoutPredicatesOK v2 suggestions get notify rules without predicates o k
*/
type V2SuggestionsGetNotifyRulesWithoutPredicatesOK struct {
	Payload *models.V2SuggestionsGetNotifyRulesWithoutPredicatesOKBody
}

func (o *V2SuggestionsGetNotifyRulesWithoutPredicatesOK) Error() string {
	return fmt.Sprintf("[POST /v2/suggestions/get_notify_rules_without_predicates][%d] v2SuggestionsGetNotifyRulesWithoutPredicatesOK  %+v", 200, o.Payload)
}
func (o *V2SuggestionsGetNotifyRulesWithoutPredicatesOK) GetPayload() *models.V2SuggestionsGetNotifyRulesWithoutPredicatesOKBody {
	return o.Payload
}

func (o *V2SuggestionsGetNotifyRulesWithoutPredicatesOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2SuggestionsGetNotifyRulesWithoutPredicatesOKBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewV2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest creates a V2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest with default headers values
func NewV2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest() *V2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest {
	return &V2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest{}
}

/* V2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest describes a response with status code 400, with default header values.

V2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest v2 suggestions get notify rules without predicates bad request
*/
type V2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest struct {
	Payload *models.V2SuggestionsGetNotifyRulesWithoutPredicatesBadRequestBody
}

func (o *V2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest) Error() string {
	return fmt.Sprintf("[POST /v2/suggestions/get_notify_rules_without_predicates][%d] v2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest  %+v", 400, o.Payload)
}
func (o *V2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest) GetPayload() *models.V2SuggestionsGetNotifyRulesWithoutPredicatesBadRequestBody {
	return o.Payload
}

func (o *V2SuggestionsGetNotifyRulesWithoutPredicatesBadRequest) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2SuggestionsGetNotifyRulesWithoutPredicatesBadRequestBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewV2SuggestionsGetNotifyRulesWithoutPredicatesDefault creates a V2SuggestionsGetNotifyRulesWithoutPredicatesDefault with default headers values
func NewV2SuggestionsGetNotifyRulesWithoutPredicatesDefault(code int) *V2SuggestionsGetNotifyRulesWithoutPredicatesDefault {
	return &V2SuggestionsGetNotifyRulesWithoutPredicatesDefault{
		_statusCode: code,
	}
}

/* V2SuggestionsGetNotifyRulesWithoutPredicatesDefault describes a response with status code -1, with default header values.

V2SuggestionsGetNotifyRulesWithoutPredicatesDefault v2 suggestions get notify rules without predicates default
*/
type V2SuggestionsGetNotifyRulesWithoutPredicatesDefault struct {
	_statusCode int

	Payload *models.V2SuggestionsGetNotifyRulesWithoutPredicatesDefaultBody
}

// Code gets the status code for the v2 suggestions get notify rules without predicates default response
func (o *V2SuggestionsGetNotifyRulesWithoutPredicatesDefault) Code() int {
	return o._statusCode
}

func (o *V2SuggestionsGetNotifyRulesWithoutPredicatesDefault) Error() string {
	return fmt.Sprintf("[POST /v2/suggestions/get_notify_rules_without_predicates][%d] /v2/suggestions/get_notify_rules_without_predicates default  %+v", o._statusCode, o.Payload)
}
func (o *V2SuggestionsGetNotifyRulesWithoutPredicatesDefault) GetPayload() *models.V2SuggestionsGetNotifyRulesWithoutPredicatesDefaultBody {
	return o.Payload
}

func (o *V2SuggestionsGetNotifyRulesWithoutPredicatesDefault) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.V2SuggestionsGetNotifyRulesWithoutPredicatesDefaultBody)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
