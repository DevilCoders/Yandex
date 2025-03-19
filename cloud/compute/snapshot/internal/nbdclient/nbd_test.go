package nbdclient

import (
	"io"
	"sync"
	"testing"

	"github.com/stretchr/testify/assert"
)

type readWritePipe struct {
	*io.PipeReader
	*io.PipeWriter
}

func (p readWritePipe) Close() error {
	firstError := p.PipeReader.Close()
	secondError := p.PipeWriter.Close()
	if firstError != nil {
		return firstError
	}
	return secondError
}

func (p readWritePipe) CloseWithError(err error) error {
	firstError := p.PipeReader.CloseWithError(err)
	secondError := p.PipeWriter.CloseWithError(err)
	if firstError != nil {
		return firstError
	}
	return secondError

}

func newReadWritePipes() (a, b readWritePipe) {
	r1, w1 := io.Pipe()
	r2, w2 := io.Pipe()
	a = readWritePipe{
		PipeReader: r1,
		PipeWriter: w2,
	}
	b = readWritePipe{
		PipeReader: r2,
		PipeWriter: w1,
	}
	return a, b
}

func TestNbdOldNegotiationInit(t *testing.T) {
	at := assert.New(t)
	const diskSize uint64 = 100
	clientConn, serverConn := newReadWritePipes()

	defer func() {
		_ = clientConn.Close()
		_ = serverConn.Close()
	}()

	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()

		// server side connection
		_, err := serverConn.Write([]byte("NBDMAGIC"))
		at.NoError(err)
		err = write(serverConn, oldStyleNegotiationMagic)
		at.NoError(err)
		err = write(serverConn, diskSize)
		at.NoError(err)
		err = write(serverConn, uint32(0)) // flags
		at.NoError(err)
		err = write(serverConn, make([]byte, reservedZeroBytesSize)) // reserved
		at.NoError(err)
	}()

	c, err := NewClient(clientConn)
	at.NoError(err)
	at.NotNil(c)
	at.False(c.newStyleNegotiation)
	at.EqualValues(diskSize, c.size)
}

func TestNewStyleNonfixedNegotiationInit(t *testing.T) {
	at := assert.New(t)
	const diskSize uint64 = 100
	clientConn, serverConn := newReadWritePipes()

	defer func() {
		_ = clientConn.Close()
		_ = serverConn.Close()
	}()

	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		defer wg.Done()

		// server side connection
		_, err := serverConn.Write([]byte("NBDMAGIC"))
		at.NoError(err)
		err = write(serverConn, iHaveOpt)
		at.NoError(err)
		err = write(serverConn, uint16(0)) // flags
		at.NoError(err)
		var clientFlags uint32
		err = read(serverConn, &clientFlags)
		at.NoError(err)
		at.EqualValues(0, clientFlags)

		// read NBD_OPT_EXPORT_NAME
		var magic uint64
		err = read(serverConn, &magic)
		at.NoError(err)
		at.EqualValues(iHaveOpt, magic)

		var optionID uint32
		err = read(serverConn, &optionID)
		at.NoError(err)
		at.EqualValues(nbdOptExportName, optionID)

		var dataLen uint32
		err = read(serverConn, &dataLen)
		at.NoError(err)
		at.EqualValues(uint32(0), dataLen)

		// answer NBD_OPT_EXPORT_NAME
		err = write(serverConn, diskSize)
		at.NoError(err)
		err = write(serverConn, uint16(0)) // transmission flags
		at.NoError(err)
		err = write(serverConn, make([]byte, reservedZeroBytesSize))
		at.NoError(err)
	}()

	c, err := NewClient(clientConn)
	at.NoError(err)
	at.NotNil(c)
	at.True(c.newStyleNegotiation)
	at.EqualValues(diskSize, c.size)
}

func TestNbdSize(t *testing.T) {
	c := Client{size: 123}
	if c.Size() != 123 {
		t.Error()
	}
}

func TestNbdClose(t *testing.T) {
	at := assert.New(t)
	clientConn, serverConn := newReadWritePipes()
	c := Client{
		conn: clientConn,
	}

	go func() {
		err := c.Close()
		at.NoError(err)
	}()

	var magic uint32
	err := read(serverConn, &magic)
	at.NoError(err)
	at.EqualValues(nbdReuestMagic, magic)

	var commandFlag uint16
	err = read(serverConn, &commandFlag)
	at.NoError(err)
	at.EqualValues(uint16(0), commandFlag)

	var commandType uint16
	err = read(serverConn, &commandType)
	at.NoError(err)
	at.EqualValues(nbdCmdDisk, commandType)

	var handle uint64
	err = read(serverConn, &handle)
	at.NoError(err)
	at.EqualValues(uint64(0), handle)

	var offset uint64
	err = read(serverConn, &offset)
	at.NoError(err)
	at.EqualValues(uint64(0), offset)

	var length uint32
	err = read(serverConn, &length)
	at.NoError(err)
	at.EqualValues(uint32(0), length)
}

func TestWriteTransmission(t *testing.T) {
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
	at := assert.New(t)
	clientConn, serverConn := newReadWritePipes()

	// zero data
	go func() {
		err := writeTransmission(clientConn, transmissionRequest{
			transmissionRequestHeader: transmissionRequestHeader{
				// NbdRequestMagic: 0, need set auto
				CommandFlags: 1,
				Type:         2,
				Handle:       3,
				Offset:       4,
			},
		})
		at.NoError(err)
	}()

	var magic uint32
	err := read(serverConn, &magic)
	at.NoError(err)
	at.EqualValues(nbdReuestMagic, magic)

	var commandFlag uint16
	err = read(serverConn, &commandFlag)
	at.NoError(err)
	at.EqualValues(uint16(1), commandFlag)

	var commandType uint16
	err = read(serverConn, &commandType)
	at.NoError(err)
	at.EqualValues(uint16(2), commandType)

	var handle uint64
	err = read(serverConn, &handle)
	at.NoError(err)
	at.EqualValues(uint64(3), handle)

	var offset uint64
	err = read(serverConn, &offset)
	at.NoError(err)
	at.EqualValues(uint64(4), offset)

	var length uint32
	err = read(serverConn, &length)
	at.NoError(err)
	at.EqualValues(uint32(0), length)

	// data
	dataOrig := []byte{1, 2, 3, 4}
	go func() {
		err := writeTransmission(clientConn, transmissionRequest{
			transmissionRequestHeader: transmissionRequestHeader{
				// NbdRequestMagic: 0, need set auto
				CommandFlags: 2,
				Type:         3,
				Handle:       4,
				Offset:       5,
				Length:       uint32(len(dataOrig)),
			},
			Data: dataOrig,
		})
		at.NoError(err)
	}()

	err = read(serverConn, &magic)
	at.NoError(err)
	at.EqualValues(nbdReuestMagic, magic)

	err = read(serverConn, &commandFlag)
	at.NoError(err)
	at.EqualValues(uint16(2), commandFlag)

	err = read(serverConn, &commandType)
	at.NoError(err)
	at.EqualValues(uint16(3), commandType)

	err = read(serverConn, &handle)
	at.NoError(err)
	at.EqualValues(uint64(4), handle)

	err = read(serverConn, &offset)
	at.NoError(err)
	at.EqualValues(uint64(5), offset)

	err = read(serverConn, &length)
	at.NoError(err)
	at.EqualValues(uint32(len(dataOrig)), length)

	data := make([]byte, length)
	err = read(serverConn, &data)
	at.NoError(err)
	at.EqualValues(dataOrig, data)
}

func TestReadTransmissionSimpleHeader(t *testing.T) {
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
	at := assert.New(t)
	clientConn, serverConn := newReadWritePipes()

	// ok
	headerOrig := transmissionSimpleReplyHeader{
		SimpleReplyMagic: nbdSimpleReplyMagic,
		Errors:           1,
		Handle:           2,
	}
	go func() {
		writeSimpleHeader(serverConn, headerOrig)
	}()
	header, err := readTransmissionSimpleHeader(clientConn)
	at.NoError(err)
	at.EqualValues(headerOrig, header)

	clientConn, serverConn = newReadWritePipes()

	// bad
	go func() {
		err := write(serverConn, nbdSimpleReplyMagic+1)
		at.NoError(err)
	}()
	_, err = readTransmissionSimpleHeader(clientConn)
	at.Error(err)
}

func TestNbdReadAt(t *testing.T) {
	const diskSize uint64 = 10
	at := assert.New(t)
	clientConn, serverConn := newReadWritePipes()
	client := Client{
		conn: clientConn,
		size: diskSize,
	}

	var wg sync.WaitGroup

	// ok
	okData := []byte{1, 2, 3, 4, 5}
	const okDataLen = 5
	const okDataOffset = 1
	wg.Add(1)
	go func() {
		defer wg.Done()

		req := readTransmissionRequest(serverConn)
		at.EqualValues(transmissionRequest{transmissionRequestHeader: transmissionRequestHeader{
			NbdRequestMagic: nbdReuestMagic,
			CommandFlags:    0,
			Type:            nbdCmdRead,
			Handle:          1,
			Offset:          okDataOffset,
			Length:          okDataLen,
		}}, req)
		writeSimpleHeader(serverConn, transmissionSimpleReplyHeader{
			SimpleReplyMagic: nbdSimpleReplyMagic,
			Errors:           0,
			Handle:           0,
		})
		err := write(serverConn, okData)
		at.NoError(err)
	}()

	data := make([]byte, okDataLen)
	readBytes, err := client.ReadAt(data, okDataOffset)
	at.NoError(err)
	at.EqualValues(readBytes, okDataLen)
	at.EqualValues(okData, data)
	wg.Wait()

	// out of disk size
	client = Client{
		conn: clientConn,
		size: diskSize,
	}

	const outSideOffset = 4
	const outSideDataLen = 7
	const outSideOkDataLen = 6
	outSideReadOrig := []byte{0, 1, 2, 3, 4, 5}

	wg.Add(1)
	go func() {
		defer wg.Done()

		req := readTransmissionRequest(serverConn)
		at.EqualValues(transmissionRequest{transmissionRequestHeader: transmissionRequestHeader{
			NbdRequestMagic: nbdReuestMagic,
			CommandFlags:    0,
			Type:            nbdCmdRead,
			Handle:          1,
			Offset:          outSideOffset,
			Length:          outSideOkDataLen,
		}}, req)
		writeSimpleHeader(serverConn, transmissionSimpleReplyHeader{
			SimpleReplyMagic: nbdSimpleReplyMagic,
			Errors:           0,
			Handle:           0,
		})
		err := write(serverConn, outSideReadOrig)
		at.NoError(err)
	}()

	outSideData := make([]byte, outSideDataLen)
	readBytes, err = client.ReadAt(outSideData, outSideOffset)
	at.EqualError(err, io.EOF.Error())
	at.EqualValues(readBytes, outSideOkDataLen)
	at.EqualValues(outSideReadOrig, outSideData[:outSideOkDataLen])
	wg.Wait()

}

func writeSimpleHeader(w io.Writer, header transmissionSimpleReplyHeader) {
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
	_ = write(w, uint32(header.SimpleReplyMagic))
	_ = write(w, uint32(header.Errors))
	_ = write(w, uint64(header.Handle))
}

func readTransmissionRequest(r io.Reader) transmissionRequest {
	/*
		The request message, sent by the client, looks as follows:

		C: 32 bits, 0x25609513, magic (`NBD_REQUEST_MAGIC`)
		C: 16 bits, command flags
		C: 16 bits, type
		C: 64 bits, handle
		C: 64 bits, offset (unsigned)
		C: 32 bits, length (unsigned)
		C: (*length* bytes of data if the request is of type `NBD_CMD_WRITE`)
	*/
	var result transmissionRequest
	_ = read(r, &result.NbdRequestMagic)
	_ = read(r, &result.CommandFlags)
	_ = read(r, &result.Type)
	_ = read(r, &result.Handle)
	_ = read(r, &result.Offset)
	_ = read(r, &result.Length)

	if result.Type == nbdCmdWrite {
		data := make([]byte, result.Length)
		_ = read(r, &data)
		result.Data = data
	}
	return result
}
