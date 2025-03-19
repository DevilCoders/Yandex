package e2e

import (
	"context"
	"encoding/json"
	"errors"
	"time"
)

// windowsInstance embeds computeInstance struct with its methods, we further extend its behaviour.
type windowsInstance struct {
	*computeInstance
}

func newWindowsInstance(ci *computeInstance) *windowsInstance {
	return &windowsInstance{
		computeInstance: ci,
	}
}

// userChangeTimeout is timeout we wait agent to make change to user.
const userChangeTimeout = time.Minute

const windowsUsersMetadataKey = "windows-users"

// updateWindowsUsers return agent's userChangeResponse on our user change request or error if no new responses found.
func (wi *windowsInstance) updateWindowsUsers(r userChangeRequest) (response userChangeResponse, err error) {
	m := NewEnvelope().WithType(UserChangeResponseType).Wrap(r)

	var rq []byte
	if rq, err = json.Marshal(m); err != nil {
		return
	}

	if err = wi.updateMetadata(windowsUsersMetadataKey, string(rq)); err != nil {
		return
	}

	ctx, cancel := context.WithTimeout(context.Background(), userChangeTimeout)
	defer cancel()

	for {
		if err = ctx.Err(); err != nil {
			return
		}

		response, err = wi.lookupUserChangeResponse(m.ID)
		if errors.Is(err, errNotFound) {
			continue
		}

		return
	}
}

func (wi *windowsInstance) lookupUserChangeResponse(id string) (response userChangeResponse, err error) {
	var o []string
	if o, err = wi.getSerialPortOutput(); err != nil {
		return
	}

	for i := len(o) - 1; i >= 0; i-- {
		d := []byte(o[i])

		var e *Envelope
		if e, err = UnmarshalEnvelope(d); err != nil {
			continue
		}

		if e.ID == id {
			err = unmarshalPayload(d, &response)

			return
		}
	}
	err = errNotFound

	return
}

// TODO:
//   * fix after refactoring - hard-coded values and errors
func (wi *windowsInstance) checkHeartbeat() (err error) {
	var o []string
	if o, err = wi.getSerialPortOutput(); err != nil {
		return
	}

	for i := len(o) - 1; i >= 0; i-- {
		d := []byte(o[i])

		var e *Envelope
		if e, err = UnmarshalEnvelope(d); err != nil {
			continue
		}

		if e.Type == "heartbeat" {
			hb := time.Unix(e.Timestamp, 0)
			now := time.Now()

			if hb.After(now) {
				err = errors.New("timestamp from future")
			}

			if hb.Before(now.Add(-60 * time.Second)) {
				err = errors.New("stale heartbeat")
			}

			return
		}
	}
	err = errNotFound

	return
}

func (wi *windowsInstance) checkLogMessages() error {
	o, err := wi.getSerialPortOutput()
	if err != nil {
		return err
	}

	for i := len(o) - 1; i >= 0; i-- {
		d := []byte(o[i])

		var e *Envelope
		if e, err = UnmarshalEnvelope(d); err != nil {
			continue
		}

		if e.Type == "log" {
			return nil
		}
	}

	return errNotFound
}

type logMessage struct {
	Msg string
}

func (wi *windowsInstance) checkTermMessages() error {
	o, err := wi.getSerialPortOutput()
	if err != nil {
		return err
	}

	for i := len(o) - 1; i >= 0; i-- {
		d := []byte(o[i])

		var e *Envelope
		if e, err = UnmarshalEnvelope(d); err != nil {
			continue
		}

		if e.Type != "log" {
			continue
		}

		var m logMessage
		if err = unmarshalPayload(d, &m); err != nil {
			continue
		}

		if m.Msg == "received SIGTERM or SIGINT" {
			return nil
		}
	}

	return errNotFound
}
