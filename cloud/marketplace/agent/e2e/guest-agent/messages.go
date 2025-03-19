package e2e

import (
	"encoding/json"
	"errors"
	"fmt"
	"time"

	"github.com/gofrs/uuid"
)

var errNotFound = errors.New("not found")

type Envelope struct {
	Timestamp int64
	Type      string
	ID        string
}

// NewEnvelope return blank envelope struct.
// Common use is:
//   NewEnvelope().WithType("Log").Wrap(`"this is fine"`)
//
//   {
//     "Timestamp":1621433132,
//     "Type":"log",
//     "ID":"15c07716-c204-4c92-9f58-083c87a5cd5e",
//     "Payload":"this is fine"
//   }
func NewEnvelope() *Envelope {
	return &Envelope{
		Timestamp: time.Now().UTC().Unix(),
		Type:      "Undefined",
		ID:        uuid.Must(uuid.NewV4()).String(),
	}
}

func (e *Envelope) WithTimestamp(t time.Time) *Envelope {
	e.Timestamp = t.UTC().Unix()

	return e
}

func (e *Envelope) WithType(t string) *Envelope {
	e.Type = t

	return e
}

func (e *Envelope) WithID(id fmt.Stringer) *Envelope {
	e.ID = id.String()

	return e
}

func (e *Envelope) Marshal(p interface{}) ([]byte, error) {
	return json.Marshal(e.Wrap(p))
}

func (e *Envelope) Wrap(p interface{}) Message {
	return Message{
		Envelope: *e,
		Payload:  p,
	}
}

type Message struct {
	Envelope
	Payload interface{}
}

type hasField bool

func (f *hasField) UnmarshalJSON(_ []byte) error {
	*f = true

	return nil
}

func UnmarshalEnvelope(d []byte) (*Envelope, error) {
	if err := checkEnvelopeFields(d); err != nil {
		return nil, fmt.Errorf("one of envelope fields %w", err)
	}

	var e Envelope
	if err := json.Unmarshal(d, &e); err != nil {
		return nil, err
	}

	return &e, nil
}

func checkEnvelopeFields(d []byte) (err error) {
	var f struct {
		Timestamp hasField
		Type      hasField
		ID        hasField
	}
	if err = json.Unmarshal(d, &f); err != nil {
		return
	}
	if !f.Timestamp || !f.Type || !f.ID {
		err = errNotFound
	}

	return
}

// unmarshalPayload field into arbitrary provided type v, from message which encoded in d.
func unmarshalPayload(d []byte, v interface{}) error {
	if err := checkPayloadField(d); err != nil {
		return fmt.Errorf("field 'payload' %w", err)
	}

	var r json.RawMessage
	s := struct {
		Payload interface{}
	}{
		&r,
	}

	if err := json.Unmarshal(d, &s); err != nil {
		return err
	}

	return json.Unmarshal(r, &v)
}

func checkPayloadField(d []byte) (err error) {
	var f struct{ Payload hasField }
	if err = json.Unmarshal(d, &f); err != nil {
		return
	}
	if !f.Payload {
		err = errNotFound
	}

	return
}
