package sflow

import (
	"bytes"
	"encoding/binary"
	"encoding/hex"
	"fmt"
	"io"
	"net"
	"strings"
	"sync/atomic"

	"a.yandex-team.ru/cloud/netinfra/tflow/metrics"
	"a.yandex-team.ru/cloud/netinfra/tflow/util"
	"a.yandex-team.ru/library/go/core/log"
)

// Record ...
type Record interface{}

// ExtendedSwitchRecord ...
type ExtendedSwitchRecord struct {
	SourceVlan          uint32
	SourcePriority      uint32
	DestinationVlan     uint32
	DestinationPriority uint32
}

// RawPacketRecord ...
type RawPacketRecord struct {
	Protocol            uint32
	FrameLength         uint32
	Stripped            uint32
	HeaderSize          uint32
	Headers             []Record
	EncapsulatedHeaders []Record // Will hold header encapsulated in mpls
}

// RawPacket ...
type RawPacket struct {
	DstMAC    net.HardwareAddr
	SrcMAC    net.HardwareAddr
	EtherType uint32
	Vlan      uint32

	IPVersion     uint32
	TOS           uint32
	FlowLabel     uint32
	PayloadLength uint32
	NextHeader    uint32
	TCPFlags      uint32
	TTL           uint32
	SrcIP         net.IP
	DstIP         net.IP
	SrcPort       uint32
	DstPort       uint32
}

func decodeExtendedSwitchRecord(data *bytes.Reader) (ExtendedSwitchRecord, error) {
	record := ExtendedSwitchRecord{}
	err := binary.Read(data, binary.BigEndian, &record)
	return record, err
}

func decodeRawPacketRecord(data *bytes.Reader, logger log.Logger) (RawPacketRecord, error) {
	var (
		err error
	)

	atomic.AddUint64(&metrics.Metrics.RawHeaders, 1)

	record := RawPacketRecord{}

	if err = util.ReadBigEndian(data, &record.Protocol); err != nil {
		return record, fmt.Errorf("decodeRawPacketRecord() can't read Protocol: %s", err)
	}

	if err = util.ReadBigEndian(data, &record.FrameLength); err != nil {
		return record, fmt.Errorf("decodeRawPacketRecord() can't read FrameLength: %s", err)
	}

	if err = util.ReadBigEndian(data, &record.Stripped); err != nil {
		return record, fmt.Errorf("decodeRawPacketRecord() can't read Stripped: %s", err)
	}

	if err = util.ReadBigEndian(data, &record.HeaderSize); err != nil {
		return record, fmt.Errorf("decodeRawPacketRecord() can't read HeaderSize: %s", err)
	}

	//skipRawPacketRecordPayload(data, record.HeaderSize, logger)

	packet := &RawPacket{}
	buf := make([]byte, getRawHeadersLength(record.HeaderSize))
	if _, err := data.Read(buf); err != nil {
		return record, err
	}

	packet.DstMAC = net.HardwareAddr(buf[0:6])
	packet.SrcMAC = net.HardwareAddr(buf[6:12])

	packet.EtherType = uint32(binary.BigEndian.Uint16(buf[12:14]))

	buf = buf[14:]
	if packet.EtherType == 0x8100 {
		packet.Vlan = uint32(binary.BigEndian.Uint16(buf[:2]) & 0x0FFF)
		packet.EtherType = uint32(binary.BigEndian.Uint16(buf[2:4]))
		buf = buf[4:]
	}

	switch packet.EtherType {
	case 0x800: // IPv4
		buf, err = packet.DecodeIPv4(buf, logger)
		if err != nil {
			return record, err
		}
		packet.DecodeL4(buf, logger)

		// MPLS over GRE ipv4 parsing
		if packet.NextHeader == 47 {
			etype := uint32(binary.BigEndian.Uint16(buf[2:4]))
			if etype == 0x8847 {
				buf, err = DecodeMPLS(buf, logger)
				if err != nil {
					return record, err
				}
				encapsulated := &RawPacket{
					DstMAC:    packet.DstMAC,
					SrcMAC:    packet.SrcMAC,
					EtherType: packet.EtherType,
					Vlan:      packet.Vlan,
				}
				if _, err := encapsulated.decodeIPUnderMPLSEncapsulation(buf, logger); err != nil {
					return record, err
				}
				record.EncapsulatedHeaders = append(record.EncapsulatedHeaders, *encapsulated)
			}
		}

	case 0x86dd: // IPv6
		buf, err = packet.DecodeIPv6(buf, logger)
		if err != nil {
			return record, err
		}
		packet.DecodeL4(buf, logger)

	case 0x8847: // MPLS unicast
		buf, err = DecodeMPLS(buf, logger)
		if err != nil {
			return record, err
		}
		encapsulated := &RawPacket{
			DstMAC:    packet.DstMAC,
			SrcMAC:    packet.SrcMAC,
			EtherType: packet.EtherType,
			Vlan:      packet.Vlan,
		}
		if _, err := encapsulated.decodeIPUnderMPLSEncapsulation(buf, logger); err != nil {
			return record, err
		}
		record.EncapsulatedHeaders = append(record.EncapsulatedHeaders, *encapsulated)

	case 0x806: // ARP

	default:
		return record, fmt.Errorf("decodeRawPacketRecord() unknown packet.EtherType: %#0x",
			packet.EtherType)

	}

	record.Headers = append(record.Headers, *packet)
	return record, nil
}

//
// IPv4
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |Version|  IHL  |Type of Service|          Total Length         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |         Identification        |Flags|      Fragment Offset    |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |  Time to Live |    Protocol   |         Header Checksum       |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                       Source Address                          |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                    Destination Address                        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                    Options                    |    Padding    |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// DecodeIPv4 parsing IPv4 header
func (p *RawPacket) DecodeIPv4(buf []byte, logger log.Logger) ([]byte, error) {
	ihl := uint8(buf[0]) & 0x0F
	if (len(buf) < 20) || (ihl < 5) {
		return buf, fmt.Errorf("DecodeIPv4() Invalid IPv4 header. Length: %d IHL: %d", len(buf), ihl)
	}
	p.IPVersion = uint32(buf[0]) >> 4
	if p.IPVersion != 4 {
		return buf, fmt.Errorf("DecodeIPv4() unexpected ip version for IPv4: %d", p.IPVersion)
	}

	p.TOS = uint32(buf[1])

	// TODO: Len. Need to decide which size should be here
	p.PayloadLength = uint32(binary.BigEndian.Uint16(buf[2:4]))

	p.FlowLabel = uint32(binary.BigEndian.Uint16(buf[4:6])) // Id field in v4 packet
	// flagAndFrag := binary.BigEndian.Uint16(buf[6:8])
	// packet.Flags = uint32(flagAndFrag >> 13)
	// packet.FragOffset = flagAndFrag & 0x1FFF
	p.TTL = uint32(buf[8])
	p.NextHeader = uint32(buf[9])
	// packet.Checksum = binary.BigEndian.Uint16(buf[10:12])
	p.SrcIP = buf[12:16]
	p.DstIP = buf[16:20]

	buf = buf[(ihl * 4):]

	return buf, nil
}

//
// IPv6
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |Version| Traffic Class |             Flow Label                |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |        Payload Length         |  Next Header  |   Hop Limit   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                                                               |
// |                            Source                             |
// |                           Address                             |
// |                                                               |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                                                               |
// |                         Destination                           |
// |                           Address                             |
// |                                                               |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// DecodeIPv6 parsing IPv6 header
func (p *RawPacket) DecodeIPv6(buf []byte, logger log.Logger) ([]byte, error) {
	p.IPVersion = uint32(buf[0]) >> 4
	if p.IPVersion != 6 {
		return buf, fmt.Errorf("DecodeIPv6() unexpected ip version for IPv6: %d",
			p.IPVersion)
	}
	p.TOS = uint32((binary.BigEndian.Uint16(buf[0:2]) >> 4) & 0x00FF)
	p.FlowLabel = binary.BigEndian.Uint32(buf[0:4]) & 0x000FFFFF
	p.PayloadLength = uint32(binary.BigEndian.Uint16(buf[4:6]))
	p.NextHeader = uint32(buf[6])
	p.TTL = uint32(buf[7])
	p.SrcIP = buf[8:24]
	p.DstIP = buf[24:40]

	buf = buf[40:]

	// https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
	// Skip useless headers to reach tcp/udp, etc
	// 0  - IPv6 Hop-by-Hop Option
	// 43 - Routing Header for IPv6
	// 44 - Fragment Header for IPv6
	// 51 - Authentication Header
	// 60 - Destination Options for IPv6
	// Next headers should be processed
	// 6 - TCP
	// 17 - UDP
	// 41 - IPv6 encapsulation
	// 58 - IPv6-ICMP
loop:
	for {
		switch p.NextHeader {
		case 0, 43, 44, 51, 60:
			p.NextHeader = uint32(buf[0])

			// headerLen count len in 8 bytes chunks (64bit)
			// First chunk doesn't count
			headerLen := 8 * (int64(buf[1]) + 1)

			// fmt.Printf("\nDecodeIPv6() headerLen >= buffer. NextHeader: %d len: %d buf size: %d\n buf:\n %s\n\n",
			// 	p.NextHeader,
			// 	headerLen,
			// 	len(buf),
			// 	hex.Dump(buf))

			buf = buf[headerLen:]

			// fmt.Printf("\nDecodeIPv6() skipping extension header: %d len: %d buf:\n%s",
			// 	p.NextHeader,
			// 	headerLen,
			// 	hex.Dump(buf))
		case 4, 6, 17, 41, 58:
			break loop
		default:
			logger.Warnf("\nDecodeIPv6() unknown header type: %d\n", p.NextHeader)
			break loop
		}
	}
	return buf, nil
}

// DecodeL4 parsing transport layer
func (p *RawPacket) DecodeL4(buf []byte, logger log.Logger) {
	if len(buf) < 4 {
		logger.Errorf("DecodeL4() buffer too short: %d\n%s\n%s\n",
			len(buf),
			p.String(), hex.Dump(buf))
		return
	}
	switch p.NextHeader {
	case 6, 17:
		p.SrcPort = uint32(binary.BigEndian.Uint16(buf[0:2]))
		p.DstPort = uint32(binary.BigEndian.Uint16(buf[2:4]))
		if p.NextHeader == 6 {
			p.TCPFlags = uint32(binary.BigEndian.Uint16(buf[12:14]) & 0x1ff)
		}
	default:
		return

	}
}

// DecodeMPLS going through labels to reach ip information in
// ecapsulated packet
func DecodeMPLS(buf []byte, logger log.Logger) ([]byte, error) {
	i := 0
	for len(buf) >= 4 {
		logger.Debugf("i: %d S: %d buf: %s\n", i, buf[2]&1, hex.Dump(buf[0:4]))
		if uint8(buf[2]&0x1) == 1 {
			buf = buf[4:]
			return buf, nil
		}
		buf = buf[4:]
		i++
	}

	return buf, fmt.Errorf("DecodeMPLS() can't find Bottom-of-Stack")
}

//
func (p *RawPacket) decodeIPUnderMPLSEncapsulation(buf []byte, logger log.Logger) ([]byte, error) {
	var (
		err error
	)
	p.IPVersion = uint32(buf[0]) >> 4

	switch p.IPVersion {
	case 4:
		buf, err = p.DecodeIPv4(buf, logger)
		if err != nil {
			return buf, err
		}
		p.DecodeL4(buf, logger)
		return buf, nil
	case 6:
		buf, err = p.DecodeIPv6(buf, logger)
		if err != nil {
			return buf, err
		}
		p.DecodeL4(buf, logger)
		return buf, nil
	}
	return buf, fmt.Errorf("decodeIPUnderEncapsulation() unknown protocol: %#0x", p.IPVersion)
}

func (p *RawPacket) String() string {
	var (
		msg strings.Builder
	)

	if _, err := fmt.Fprintf(&msg, "=== dstMac %s || srcMac %s || etype %#0x ",
		p.DstMAC,
		p.SrcMAC,
		p.EtherType); err != nil {
		fmt.Printf("RawPacket.String() 1: %v", err)
	}

	if _, err := fmt.Fprintf(&msg, "|| Vlan %d || etype %#0x \n", p.Vlan, p.EtherType); err != nil {
		fmt.Printf("RawPacket.String() 2: %v", err)
	}

	if _, err := fmt.Fprintf(&msg, "    ipVer %d || trafClass %d || flowLabel %#0x || len %d || NextHeader %d || TTL %d\n"+
		"    srcIP %s || dstIP %s \n"+
		"    srcPort %d || dstPort %d || TCPFlags %d ",
		p.IPVersion,
		p.TOS,
		p.FlowLabel,
		p.PayloadLength,
		p.NextHeader,
		p.TTL,
		p.SrcIP,
		p.DstIP,
		p.SrcPort,
		p.DstPort,
		p.TCPFlags); err != nil {
		fmt.Printf("RawPacket.String() 3: %v", err)
	}

	msg.WriteString("===")

	return msg.String()
}

// TSKV print RawPacket data as tskv
func (p *RawPacket) TSKV() string {
	return fmt.Sprintf("src_mac=%s\tdst_mac=%s\tether_type=%#04x\tvlan=%d\tip_version=%d\ttos=%d\t"+
		"flow_label=%d\tnext_header=%d\ttcp_flags=%d\tttl=%d\tsrc_ip=%s\tdst_ip=%s\tsrc_port=%d\tdst_port=%d",
		p.SrcMAC,
		p.DstMAC,
		p.EtherType,
		p.Vlan,
		p.IPVersion,
		p.TOS,
		p.FlowLabel,
		p.NextHeader,
		p.TCPFlags,
		p.TTL,
		p.SrcIP,
		p.DstIP,
		p.SrcPort,
		p.DstPort,
	)
}

func skipRawPacketRecordPayload(data *bytes.Reader, headerLength uint32, logger log.Logger) {
	if padding := (headerLength % 4); padding != 0 {
		headerLength = headerLength + (4 - padding)
	}

	if _, err := data.Seek(int64(headerLength), io.SeekCurrent); err != nil {
		logger.Errorf("skipRawPacketRecordPayload(): %v", err)
	}
}

func getRawHeadersLength(headerLength uint32) int64 {
	if padding := (headerLength % 4); padding != 0 {
		headerLength = headerLength + (4 - padding)
	}

	return int64(headerLength)
}
