package nbdclient

import (
	"io"

	"golang.org/x/xerrors"
)

type nbdCmdType uint16

// NBD_CMD_READ
const nbdCmdRead nbdCmdType = 0

// NBD_CMD_WRITE
const nbdCmdWrite nbdCmdType = 1

// NBD_CMD_DISC
const nbdCmdDisk nbdCmdType = 2

// NBD_REQUEST_MAGIC
const nbdReuestMagic uint32 = 0x25609513

// NBD_SIMPLE_REPLY_MAGIC
const nbdSimpleReplyMagic uint32 = 0x67446698

type transmissionRequestHeader struct {
	NbdRequestMagic uint32
	CommandFlags    uint16
	Type            nbdCmdType
	Handle          uint64
	Offset          uint64
	Length          uint32
}
type transmissionRequest struct {
	transmissionRequestHeader
	Data []byte
}

type transmissionSimpleReplyHeader struct {
	SimpleReplyMagic uint32
	Errors           uint32
	Handle           uint64
}

func readTransmissionSimpleHeader(r io.Reader) (transmissionSimpleReplyHeader, error) {
	/*
		#### Simple reply message
		...
		S: 32 bits, 0x67446698, magic (`NBD_SIMPLE_REPLY_MAGIC`; used to be
		   `NBD_REPLY_MAGIC`)
		S: 32 bits, error (MAY be zero)
		S: 64 bits, handle
		S: (*length* bytes of data if the request is of type `NBD_CMD_READ` and
		    *error* is zero)
	*/

	var result transmissionSimpleReplyHeader
	err := read(r, &result.SimpleReplyMagic)
	if err != nil {
		return result, xerrors.Errorf("read simple transmission header magic: %w", err)
	}

	if result.SimpleReplyMagic != nbdSimpleReplyMagic {
		return result, xerrors.New("bad simple reply magic")
	}

	err = read(r, &result.Errors)
	if err != nil {
		return result, xerrors.Errorf("read simple transmission header errors: %w", err)
	}

	err = read(r, &result.Handle)
	if err != nil {
		return result, xerrors.Errorf("read simple transmission header handle: %w", err)
	}

	return result, err
}

func writeTransmission(w io.Writer, request transmissionRequest) error {
	/*
	   #### Request message

	   The request message, sent by the client, looks as follows:

	   C: 32 bits, 0x25609513, magic (`NBD_REQUEST_MAGIC`)
	   C: 16 bits, command flags
	   C: 16 bits, type
	   C: 64 bits, handle
	   C: 64 bits, offset (unsigned)
	   C: 32 bits, length (unsigned)
	   C: (*length* bytes of data if the request is of type `NBD_CMD_WRITE`)

	*/
	request.NbdRequestMagic = nbdReuestMagic
	err := write(w, &request.transmissionRequestHeader)
	if err != nil {
		return err
	}
	if len(request.Data) > 0 {
		return write(w, &request.Data)
	}
	return nil
}
