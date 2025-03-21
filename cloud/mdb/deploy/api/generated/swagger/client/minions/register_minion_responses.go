// Code generated by go-swagger; DO NOT EDIT.

package minions

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"
	"io"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/models"
)

// RegisterMinionReader is a Reader for the RegisterMinion structure.
type RegisterMinionReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *RegisterMinionReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewRegisterMinionOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	default:
		result := NewRegisterMinionDefault(response.Code())
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		if response.Code()/100 == 2 {
			return result, nil
		}
		return nil, result
	}
}

// NewRegisterMinionOK creates a RegisterMinionOK with default headers values
func NewRegisterMinionOK() *RegisterMinionOK {
	return &RegisterMinionOK{}
}

/* RegisterMinionOK describes a response with status code 200, with default header values.

Registered minion
*/
type RegisterMinionOK struct {
	Payload *models.MinionResp
}

func (o *RegisterMinionOK) Error() string {
	return fmt.Sprintf("[POST /v1/minions/{fqdn}/register][%d] registerMinionOK  %+v", 200, o.Payload)
}
func (o *RegisterMinionOK) GetPayload() *models.MinionResp {
	return o.Payload
}

func (o *RegisterMinionOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.MinionResp)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

// NewRegisterMinionDefault creates a RegisterMinionDefault with default headers values
func NewRegisterMinionDefault(code int) *RegisterMinionDefault {
	return &RegisterMinionDefault{
		_statusCode: code,
	}
}

/* RegisterMinionDefault describes a response with status code -1, with default header values.

Error
*/
type RegisterMinionDefault struct {
	_statusCode int

	Payload *models.Error
}

// Code gets the status code for the register minion default response
func (o *RegisterMinionDefault) Code() int {
	return o._statusCode
}

func (o *RegisterMinionDefault) Error() string {
	return fmt.Sprintf("[POST /v1/minions/{fqdn}/register][%d] RegisterMinion default  %+v", o._statusCode, o.Payload)
}
func (o *RegisterMinionDefault) GetPayload() *models.Error {
	return o.Payload
}

func (o *RegisterMinionDefault) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.Error)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
