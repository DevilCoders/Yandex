package nbdclient

import (
	"bytes"
	"io"
)

const (
	// NBDMAGIC
	nbdMagic uint64 = 0x4e42444d41474943

	// IHAVEOPT
	iHaveOpt uint64 = 0x49484156454F5054

	// cliserv_magic
	oldStyleNegotiationMagic uint64 = 0x00420281861253

	reservedZeroBytesSize = 124
)

type clientToServerHandshakeOption uint32

// NBD_OPT_EXPORT_NAME
const nbdOptExportName clientToServerHandshakeOption = 1

type setOptionHeader struct {
	MagicIHAVEOPT uint64
	Option        clientToServerHandshakeOption
	DataLen       uint32
}

type exportReply struct {
	Size              uint64
	TransmissionFlags uint16
	Reserved          [reservedZeroBytesSize]byte
}

func writeOption(writer io.Writer, option clientToServerHandshakeOption, data []byte) error {
	buf := &bytes.Buffer{}
	err := write(buf, setOptionHeader{MagicIHAVEOPT: iHaveOpt, Option: option, DataLen: uint32(len(data))})
	if err != nil {
		return err
	}
	err = write(buf, data)
	if err != nil {
		return err
	}
	return write(writer, buf.Bytes())
}

func readExport(r io.Reader) (exportReply, error) {
	var result exportReply
	err := read(r, &result)
	return result, err
}
