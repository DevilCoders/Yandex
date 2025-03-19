package ipfix

import (
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"time"
)

type elt struct {
	ID     uint16
	Len    uint16
	Name   string
	Decode func(r io.Reader) (string, error)
}

type element struct {
	Name  string
	Value string
}

func decodeMilliTimestamp(r io.Reader) (string, error) {
	var val int64
	if err := binary.Read(r, binary.BigEndian, &val); err != nil {
		return "", err
	}
	sec := val / 1000
	nanosec := (val % 1000) * 1000000
	return time.Unix(sec, nanosec).UTC().Format("2006.01.02 15:04:05.000 UTC"), nil
}

func decodeNatEvent(r io.Reader) (string, error) {
	var val uint8
	if err := binary.Read(r, binary.BigEndian, &val); err != nil {
		return "", err
	}
	if rv, ok := natEvents[val]; ok {
		return rv, nil
	} else {
		return fmt.Sprintf("event %d", val), nil
	}
}

func decodeAddr(r io.Reader, len int) (string, error) {
	val := make([]byte, len)
	if _, err := r.Read(val); err != nil {
		return "", err
	}

	return net.IP(val).String(), nil
}

func decodeIPv4(r io.Reader) (string, error) {
	return decodeAddr(r, 4)
}

func decodeIPv6(r io.Reader) (string, error) {
	return decodeAddr(r, 16)
}

func decodeProto(r io.Reader) (string, error) {
	var val uint8
	if err := binary.Read(r, binary.BigEndian, &val); err != nil {
		return "", err
	}
	if rv, ok := protos[val]; ok {
		return rv, nil
	} else {
		return fmt.Sprintf("proto %d", val), nil
	}
}

func decodeUint16(r io.Reader) (string, error) {
	var val uint16
	if err := binary.Read(r, binary.BigEndian, &val); err != nil {
		return "", err
	}
	return fmt.Sprintf("%d", val), nil
}

func decodeUint32(r io.Reader) (string, error) {
	var val uint32
	if err := binary.Read(r, binary.BigEndian, &val); err != nil {
		return "", err
	}
	return fmt.Sprintf("%d", val), nil
}

func decodeUnknown(r io.Reader, len uint16) (string, error) {
	val := make([]byte, len)
	if _, err := r.Read(val); err != nil {
		return "", err
	} else {
		return fmt.Sprintf("%v", val), nil
	}
}

//see https://www.iana.org/assignments/ipfix/ipfix.xhtml#ipfix-information-element-data-types
var elements = map[uint16]elt{
	4:   {4, 1, "protocolIdentifier", decodeProto},
	7:   {7, 2, "sourceTransportPort", decodeUint16},
	8:   {8, 4, "sourceIPv4Address", decodeIPv4},
	11:  {11, 2, "destinationTransportPort", decodeUint16},
	27:  {27, 16, "sourceIPv6Address", decodeIPv6},
	28:  {28, 16, "destinationIPv6Address", decodeIPv6},
	225: {225, 4, "postNATSourceIPv4Address", decodeIPv4},
	226: {226, 4, "postNATDestinationIPv4Address", decodeIPv4},
	227: {227, 2, "postNATSourceTransportPort", decodeUint16},
	228: {228, 2, "postNATDestinationTransportPort", decodeUint16},
	230: {230, 1, "natEvent", decodeNatEvent},
	234: {234, 4, "ingressVRFID", decodeUint32},
	283: {283, 4, "natPoolId", decodeUint32},
	323: {232, 8, "observationTimeMilliseconds", decodeMilliTimestamp},
	466: {466, 4, "natQuotaExceededEvent", decodeUint32},
	471: {471, 4, "maxSessioinEntries", decodeUint32},
	472: {472, 4, "maxBIBEntries", decodeUint32},
	473: {473, 4, "maxEntriesPerUser", decodeUint32},
	475: {475, 4, "maxFragmentsPendingReassembly", decodeUint32},
}

//see https://www.iana.org/assignments/ipfix/ipfix.xhtml#ipfix-nat-event-type
var natEvents = map[uint8]string{
	0:  "Reserved",
	1:  "NAT translation create",
	2:  "NAT translation delete",
	3:  "NAT addresses exhausted",
	4:  "NAT44 session create",
	5:  "NAT44 session delete",
	6:  "NAT64 session create",
	7:  "NAT64 session delete",
	8:  "NAT44 BIB create",
	9:  "NAT44 BIB delete",
	10: "NAT64 BIB create",
	11: "NAT64 BIB delete",
	12: "NAT ports exhausted",
	13: "Quota exceeded",
	14: "Address binding create",
	15: "Address binding delete",
	16: "Port block allocation",
	17: "Port block de-allocation",
	18: "Threshold Reached",
}

//see https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
var protos = map[uint8]string{
	1:  "icmp",
	6:  "tcp",
	17: "udp",
}

func newUnknownElement(id uint16, len uint16) elt {
	return elt{
		id,
		len,
		fmt.Sprintf("id %d", id),
		func(r io.Reader) (string, error) {
			return decodeUnknown(r, len)
		},
	}
}

func registerElement(e elt) {
	elements[e.ID] = e
}

func removeElement(id uint16) {
	delete(elements, id)
}

func isElementKnown(id uint16) bool {
	_, ok := elements[id]
	return ok
}

func decodeElement(id uint16, r io.Reader) (element, error) {
	elt := elements[id]
	value, err := elt.Decode(r)
	if err != nil {
		return element{}, err
	}
	return element{elt.Name, value}, nil
}
