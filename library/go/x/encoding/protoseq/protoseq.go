// Package protoseq implements a coding scheme used in LogFeller in order
// collect separate Protobuf messages into one frame.
//
// See https://wiki.yandex-team.ru/logfeller/splitter/protoseq/ for details.
package protoseq

import (
	"bytes"
	"encoding/binary"
	"errors"
	"io"

	"github.com/golang/protobuf/proto"
)

var (
	ErrInInitialState = errors.New(".More() should be called first")
	ErrWrongSyncWord  = errors.New("wrong sync word")
)

// SyncWord is a word used to trailing delimiter of an item in ProtoSeq frame.
var SyncWord = [32]byte{
	31, 247, 247, 126, 190, 166, 94, 158, 55, 166, 246, 46, 254, 174, 71, 167,
	183, 110, 191, 175, 22, 158, 159, 55, 246, 87, 247, 102, 167, 6, 175, 247,
}

// decoderState encodes a internal state of Protoseq decoder. Possible states
// are init, more, decode, and terminal. They have the following transition table.
//
//  init -> more | terminal
//  more -> more | decode | terminal
//  decode -> more | decode | terminal
//  terminal -> terminal
//
// Decoder object is instantiated in init state. Terminal state is achievable
// from any state. If a decoder is in more or init state then invocation of
// More() or Decode() correspondently does not change a decoder state.
// Transitions more -> decode and decode -> more order invocations of More()
// and Decode() in a proper sequence.
type decoderState byte

const (
	initState decoderState = iota
	moreState
	decodeState
	terminalState
)

// Decoder decodes a Protoseq frame into a batch of type-identical Protobuf
// messages.
type Decoder struct {
	r       io.Reader
	err     error
	buf     [32]byte     // Payload length/protoseq magic.
	payload bytes.Buffer // Protobuf payload itself.
	s       decoderState
}

func NewDecoder(r io.Reader) *Decoder {
	return &Decoder{r: r, s: initState}
}

func (d *Decoder) Decode(v proto.Message) error {
	switch d.s {
	case initState:
		d.transitToTerminalState(ErrInInitialState)
		return d.err
	case terminalState:
		return d.err // In terminal state already.
	case moreState:
		d.s = decodeState
		fallthrough
	case decodeState:
		return proto.Unmarshal(d.payload.Bytes(), v)
	default:
		panic("unreachable state")
	}
}

func (d *Decoder) Err() error {
	return d.err
}

func (d *Decoder) More() bool {
	switch d.s {
	case moreState:
		return true // Already in moreState.
	case terminalState:
		return false
	case initState:
		fallthrough
	case decodeState:
		if _, err := io.ReadFull(d.r, d.buf[:4]); err != nil {
			if err == io.EOF {
				// That's fine, just end of sequence. No errors needed.
				err = nil
			}

			d.transitToTerminalState(err)
			return false
		}

		size := binary.LittleEndian.Uint32(d.buf[:4])
		d.payload.Reset()
		if _, err := io.CopyN(&d.payload, d.r, int64(size)); err != nil {
			d.transitToTerminalState(err)
			return false
		}

		if _, err := io.ReadFull(d.r, d.buf[:32]); err != nil {
			d.transitToTerminalState(err)
			return false
		}

		if SyncWord != d.buf {
			d.transitToTerminalState(ErrWrongSyncWord)
			return false
		}

		d.s = moreState
		return true
	default:
		panic("unreachable state")
	}
}

func (d *Decoder) transitToTerminalState(err error) {
	d.s = terminalState
	d.err = err
}

// Encoder encodes a batch of Protobuf messages to a Protoseq frame. Protoseq
// is used in logfeller in order to process chunks of identical Protobuf
// messages.
type Encoder struct {
	w io.Writer
}

func NewEncoder(w io.Writer) *Encoder {
	return &Encoder{w: w}
}

func (e *Encoder) Encode(v proto.Message) error {
	var bytes, err = proto.Marshal(v)
	if err != nil {
		return err
	}

	var prefix [4]byte
	binary.LittleEndian.PutUint32(prefix[:], uint32(len(bytes)))

	if _, err := e.w.Write(prefix[:]); err != nil {
		return err
	}

	if _, err := e.w.Write(bytes); err != nil {
		return err
	}

	if _, err := e.w.Write(SyncWord[:]); err != nil {
		return err
	}

	return nil
}
