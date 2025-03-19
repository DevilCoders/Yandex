package ipfix

import (
	"bytes"
	"testing"
)

func TestParsePacketHeader(t *testing.T) {
	data := []byte{
		0x00, 0x0a, 0x00, 0x62, 0x5d, 0x36, 0xd8, 0xcf,
		0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x28,
	}
	means := packet{10, 98, 1563875535, 3, 40}
	p, err := parsePacketHeader(bytes.NewReader(data))
	if err != nil {
		t.Errorf("failed to parse packet header")
		return
	}
	if p.Version != means.Version {
		t.Errorf("got version %d expected %d", p.Version, means.Version)
	}
	if p.Length != means.Length {
		t.Errorf("got length %d expected %d", p.Length, means.Length)
	}
	if p.TS != means.TS {
		t.Errorf("got timestamp %d expected %d", p.TS, means.TS)
	}
	if p.Seq != means.Seq {
		t.Errorf("got sequance id %d expected %d", p.Seq, means.Seq)
	}
	if p.Domain != means.Domain {
		t.Errorf("got observation domain id %d expeced %d", p.Domain, means.Domain)
	}
}

func TestWrongVersion(t *testing.T) {
	data := []byte{
		0x00, 0x0b, 0x00, 0x62, 0x5d, 0x36, 0xd8, 0xcf,
		0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x28,
	}
	_, err := parsePacketHeader(bytes.NewReader(data))
	if err == nil {
		t.Errorf("no error on wrong protocol version")
	}
}
