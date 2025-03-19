package nbdclient

// partial implementation of network nbd protocol - for read disk from local qemu-nbd process
import (
	"encoding/binary"
	"io"
	"math"
	"sync"

	"golang.org/x/xerrors"
)

// Safe to use from many goroutines
// but work sequentional now
type Client struct {
	size                uint64
	newStyleNegotiation bool

	mu     sync.Mutex // fields under mutex protected by mutex
	conn   io.ReadWriter
	handle uint64
}

func NewClient(conn io.ReadWriter) (*Client, error) {
	c := Client{conn: conn}
	err := c.handshake()
	if err != nil {
		return nil, err
	}
	err = c.switchToTransmitPhase()
	if err != nil {
		return nil, err
	}
	return &c, nil
}

// Close client
func (c *Client) Close() error {
	c.mu.Lock()
	defer c.mu.Unlock()

	return writeTransmission(c.conn, transmissionRequest{
		transmissionRequestHeader: transmissionRequestHeader{
			Type: nbdCmdDisk,
		},
		Data: nil,
	})
}

func (c *Client) HandshakeNegotiation() string {
	if c.newStyleNegotiation {
		return "newstyle-nonfixed"
	}
	return "oldstyle"
}

// ReadAt Read all or nothing instead of last block - if end of block outside from image size
// ReadAt read data until end of block and return io.EOF
// It untouch rest bytes in data
// It block until read data or get error from client connection
func (c *Client) ReadAt(data []byte, offset int64) (int, error) {
	if len(data) > math.MaxUint32 {
		return 0, xerrors.New("Nbd protocol support only 32-bit unsigned length")
	}

	eofFlag := uint64(offset)+uint64(len(data)) > c.size
	if eofFlag {
		restDataLen := c.size - uint64(offset)
		data = data[:int(restDataLen)]
	}

	if len(data) > 0 {
		// TODO: rewrite for work in parallel and detect right reader from Handle
		c.mu.Lock()
		defer c.mu.Unlock()

		c.handle++
		handle := c.handle

		err := writeTransmission(c.conn, transmissionRequest{
			transmissionRequestHeader: transmissionRequestHeader{
				Type:   nbdCmdRead,
				Handle: handle,
				Offset: uint64(offset),
				Length: uint32(len(data)),
			},
			Data: nil,
		})
		if err != nil {
			return 0, xerrors.Errorf("error while write data request to nbd: %w", err)
		}

		header, err := readTransmissionSimpleHeader(c.conn)
		if err != nil {
			return 0, xerrors.Errorf("error while read data header from nbd: %w", err)
		}
		if header.Errors != 0 {
			return 0, xerrors.Errorf("error from nbd server: %v", header.Errors)
		}
		err = read(c.conn, &data)
		if err != nil {
			return 0, xerrors.Errorf("error while read data from nbd: %w", err)
		}
	}

	if eofFlag {
		return len(data), io.EOF
	} else {
		return len(data), nil
	}

}

func (c *Client) Size() int64 {
	return int64(c.size)
}

func (c *Client) handshake() error {
	var err error
	w := func(data interface{}) {
		if err == nil {
			err = write(c.conn, data)
		}
	}
	r := func(data interface{}) {
		if err == nil {
			err = read(c.conn, data)
		}
	}

	var d uint64
	r(&d)
	if d != nbdMagic {
		return xerrors.New("Bad NBDMAGIC")
	}
	r(&d)
	switch d {
	case iHaveOpt:
		c.newStyleNegotiation = true
		// new non fixed negotiation
		var serverHandshakeFlags uint16
		r(&serverHandshakeFlags)
		// ignore server flags
		w(uint32(0)) // client flags
		return err
	case oldStyleNegotiationMagic:
		r(&c.size)
		var serverFlags uint32
		r(&serverFlags)
		reserved := make([]byte, 124)
		r(&reserved)
		return err
	default:
		return xerrors.Errorf("Bad magic after NBDMAGIC: %016x", d)
	}

}

func (c *Client) switchToTransmitPhase() error {
	if !c.newStyleNegotiation {
		// not need for oldstyle
		return nil
	}
	err := writeOption(c.conn, nbdOptExportName, nil)
	if err != nil {
		return err
	}
	res, err := readExport(c.conn)
	if err != nil {
		return err
	}
	c.size = res.Size
	return nil
}

func read(r io.Reader, data interface{}) error {
	return binary.Read(r, binary.BigEndian, data)
}

func write(w io.Writer, data interface{}) error {
	return binary.Write(w, binary.BigEndian, data)
}
