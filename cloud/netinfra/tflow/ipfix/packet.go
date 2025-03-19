package ipfix

import (
	"encoding/binary"
	"errors"
	"io"
)

//see https://tools.ietf.org/html/rfc7011#section-3.1
type packet struct {
	Version uint16
	Length  uint16
	TS      uint32
	Seq     uint32
	Domain  uint32
}

var ipfixVersion uint16 = 10
var ipfixHeaderLen uint16 = 16

func parsePacketHeader(r io.Reader) (packet, error) {
	var p packet
	if err := binary.Read(r, binary.BigEndian, &p.Version); err != nil {
		return p, err
	}
	if err := binary.Read(r, binary.BigEndian, &p.Length); err != nil {
		return p, err
	}
	if err := binary.Read(r, binary.BigEndian, &p.TS); err != nil {
		return p, err
	}
	if err := binary.Read(r, binary.BigEndian, &p.Seq); err != nil {
		return p, err
	}
	if err := binary.Read(r, binary.BigEndian, &p.Domain); err != nil {
		return p, err
	}
	if p.Version != ipfixVersion {
		return p, errors.New("wrong version")
	}
	return p, nil
}
