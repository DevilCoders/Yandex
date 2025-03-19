package ipfix

import (
	"bytes"
	"fmt"
	"testing"
)

func failHelper(means string, got string, t *testing.T) {
	if got != means {
		t.Errorf("wanted %s but got %s", means, got)
	}
}

func TestDecodeMilliTimestamp(t *testing.T) {
	ts := []byte{0, 0, 0, 0, 0, 0, 0, 0}
	means := "1970.01.01 00:00:00.000 UTC"
	str, _ := decodeMilliTimestamp(bytes.NewReader(ts))
	failHelper(means, str, t)
}

func TestDecodeNatEvent(t *testing.T) {
	event := []byte{3}
	means := "NAT addresses exhausted"
	str, _ := decodeNatEvent(bytes.NewReader(event))
	failHelper(means, str, t)
}

func TestDecodeNatEventUnknown(t *testing.T) {
	event := []byte{42}
	means := "event 42"
	str, _ := decodeNatEvent(bytes.NewReader(event))
	failHelper(means, str, t)
}

func TestDecodeIPv4Addr(t *testing.T) {
	addr := []byte{0x0a, 0, 0, 0x01}
	means := "10.0.0.1"
	str, _ := decodeIPv4(bytes.NewReader(addr))
	failHelper(means, str, t)
}

func TestDecodeIPv6Addr(t *testing.T) {
	addr := []byte{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	means := "::"
	str, _ := decodeIPv6(bytes.NewReader(addr))
	failHelper(means, str, t)
}

func TestDecodeProto(t *testing.T) {
	proto := []byte{6}
	means := "tcp"
	str, _ := decodeProto(bytes.NewReader(proto))
	failHelper(means, str, t)
}

func TestDecodeProtoUnknown(t *testing.T) {
	proto := []byte{42}
	means := "proto 42"
	str, _ := decodeProto(bytes.NewReader(proto))
	failHelper(means, str, t)
}

func TestDecodeShort(t *testing.T) {
	num := []byte{0, 42}
	means := "42"
	str, _ := decodeUint16(bytes.NewReader(num))
	failHelper(means, str, t)
}

func TestDecodeLong(t *testing.T) {
	num := []byte{0, 0, 0x0f, 0xed}
	means := "4077"
	str, _ := decodeUint32(bytes.NewReader(num))
	failHelper(means, str, t)
}

func TestDecodeUnknown(t *testing.T) {
	val := []byte{3, 14, 15, 92, 6}
	means := "[3 14 15 92 6]"
	str, _ := decodeUnknown(bytes.NewReader(val), 5)
	failHelper(means, str, t)
}

func TestIsElementKnown(t *testing.T) {
	protoID := uint16(4)
	if !isElementKnown(protoID) {
		t.Errorf("failed to find protocolIdentifier element")
	}
}

func TestIsElementUnknown(t *testing.T) {
	if isElementKnown(4077) {
		t.Errorf("appears to know element #4077")
	}
}

func TestRegisterUnknown(t *testing.T) {
	id := uint16(4077)
	if isElementKnown(id) {
		t.Errorf("element #4077 allready known")
	}
	e := newUnknownElement(id, 8)
	registerElement(e)
	if !isElementKnown(id) {
		t.Errorf("failed to register element #4077")
	}
	removeElement(id)
	if isElementKnown(id) {
		t.Errorf("failed to remove element #4077")
	}
}

func TestDecodeElement(t *testing.T) {
	protoID := uint16(4)
	proto := []byte{17}
	means := "protocolIdentifier:udp"
	elt, _ := decodeElement(protoID, bytes.NewReader(proto))
	failHelper(means, fmt.Sprintf("%s:%s", elt.Name, elt.Value), t)
}

func TestDecodeElementFail(t *testing.T) {
	protoID := uint16(4)
	_, err := decodeElement(protoID, bytes.NewReader([]byte{}))
	if err == nil {
		t.Errorf("no error on empty input")
	}
}

func TestDecodeUnknownElement(t *testing.T) {
	id := uint16(4077)
	len := uint16(5)
	val := []byte{3, 14, 15, 92, 6}
	means := "id 4077:[3 14 15 92 6]"
	e := newUnknownElement(id, len)
	registerElement(e)
	defer removeElement(id)
	elt, _ := decodeElement(id, bytes.NewReader(val))
	failHelper(means, fmt.Sprintf("%s:%s", elt.Name, elt.Value), t)
}
