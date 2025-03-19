package sflow

import (
	"bytes"
	"encoding/binary"
	"encoding/hex"
	"errors"
	"fmt"
	"net"
	"strings"
	"sync/atomic"
	"time"

	"a.yandex-team.ru/cloud/netinfra/tflow/consumer"
	"a.yandex-team.ru/cloud/netinfra/tflow/metrics"
	"a.yandex-team.ru/cloud/netinfra/tflow/util"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	ErrUnsupportedSFlowVersion = errors.New("the sflow version doesn't support")
	ErrUnknownSampleType       = errors.New("the sflow version doesn't support")
)

/////////////////////////////////////////////////////
// sFlow datagram Header
//
//  0                      15                      31
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |           int sFlow version (2|4|5)           |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |   int IP version of the Agent (1=v4|2=v6)     |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  /    Agent IP address (v4=4byte|v6=16byte)      /
//  /                                               /
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |               int sub agent id                |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |         int datagram sequence number          |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |            int switch uptime in ms            |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |          int n samples in datagram            |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  /                  n samples                    /
//  /                                               /
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

// Datagram structure
type Datagram struct {
	Version    uint32 // sFlow version
	IPVersion  uint32 // (1=v4|2=v6) address of agent
	IPAddress  net.IP // Agent IP address
	AgentSubID uint32 // Identifies a source of sFlow data
	SequenceNo uint32 // Sequence of sFlow Datagrams
	SysUpTime  uint32 // Uptime in milliseconds
	SamplesNo  uint32 // Number of samples
	Timestamp  int64

	Samples   []Sample
	strSample string
	strEncap  string
}

type Sampled struct {
	header *Datagram
	sample *FlowSample
	record *RawPacketRecord
}

type Encaped struct {
	header *Datagram
	sample *FlowSample
	record *RawPacketRecord
}

func (s Sampled) String() string {
	var sb strings.Builder
	sb.WriteString(s.header.strSample)
	sb.WriteString(s.sample.str)
	sb.WriteString(fmt.Sprintf("frame_size=%d\t", s.record.FrameLength))
	//TODO: idunno. @atsygane uses first header only
	raw := (s.record.Headers[0]).(RawPacket)
	sb.WriteString(raw.TSKV())
	sb.WriteString("\n")
	return sb.String()
}

func (e Encaped) String() string {
	var sb strings.Builder
	sb.WriteString(e.header.strEncap)
	//TODO: idunno. @atsygane uses no sample prefix on encap tskv
	sb.WriteString(fmt.Sprintf("frame_size=%d\t", e.record.FrameLength))
	//TODO: idunno. @atsygane uses first header only
	raw := (e.record.EncapsulatedHeaders[0]).(RawPacket)
	sb.WriteString(raw.TSKV())
	sb.WriteString("\n")
	return sb.String()
}

func (dgram *Datagram) decodeDatagramHeader(payload *bytes.Reader) error {
	var (
		err   error
		ipLen = net.IPv4len
	)

	if err = util.ReadBigEndian(payload, &dgram.Version); err != nil {
		return err
	}

	if dgram.Version != 5 {
		return ErrUnsupportedSFlowVersion
	}

	if err = util.ReadBigEndian(payload, &dgram.IPVersion); err != nil {
		return err
	}

	if dgram.IPVersion == 2 {
		// IPv6 agent address
		ipLen = net.IPv6len
	}

	ip := make([]byte, ipLen)
	if _, err = payload.Read(ip); err != nil {
		return err
	}
	dgram.IPAddress = ip

	if err = util.ReadASet(payload, &dgram.AgentSubID, &dgram.SequenceNo, &dgram.SysUpTime, &dgram.SamplesNo); err != nil {
		return err
	}

	//TODO: idunno. @atsygane uses different prefixes in different tskv files
	dgram.strSample = fmt.Sprintf(
		"tskv\tagent=%s\tagent_sub_id=%d\tsequence_number=%d\tuptime=%d\tunixtime=%d\t",
		dgram.IPAddress,
		dgram.AgentSubID,
		dgram.SequenceNo,
		dgram.SysUpTime,
		dgram.Timestamp,
	)

	dgram.strEncap = fmt.Sprintf(
		"tskv\tagent=%s\tunixtime=%d\t",
		dgram.IPAddress,
		dgram.Timestamp,
	)

	return nil
}

type Sample interface {
	SampleType() int
	GetRecords() []Record
}

func decodeSample(r *bytes.Reader, d *Datagram, encapEnabled bool, out chan consumer.Message, logger log.Logger) (Sample, error) {
	format, length, err := SampleType(0), uint32(0), error(nil)

	err = binary.Read(r, binary.BigEndian, &format)
	if err != nil {
		return nil, err
	}

	err = binary.Read(r, binary.BigEndian, &length)
	if err != nil {
		return nil, err
	}

	switch format {
	case TypeFlowSample:
		retval, err := decodeFlowSample(r, format, false, encapEnabled, d, out, logger)
		if err != nil {
			out <- consumer.Message{Type: consumer.SFlowSample, Elements: retval.samples}
			if encapEnabled && len(retval.encaps) > 0 {
				out <- consumer.Message{Type: consumer.SFlowEncap, Elements: retval.encaps}
			}
		}
		return retval, err

	case TypeCounterSample:
		return decodeCounterSample(r, format, false, logger)

	case TypeExpandedFlowSample:
		retval, err := decodeFlowSample(r, format, true, encapEnabled, d, out, logger)
		if err == nil {
			out <- consumer.Message{Type: consumer.SFlowSample, Elements: retval.samples}
			if encapEnabled && len(retval.encaps) > 0 {
				out <- consumer.Message{Type: consumer.SFlowEncap, Elements: retval.encaps}
			}
		}
		return retval, err

	case TypeExpandedCounterSample:
		return decodeCounterSample(r, format, true, logger)

	default:
		fmt.Printf("\nUnknown Sample format: %d\n", format)
		_, err = r.Seek(int64(length), 1)
		if err != nil {
			return nil, err
		}

		return nil, ErrUnknownSampleType
	}
}

// Decode sFlow packet
func Decode(addr net.IP, payload []byte, encapEnabled bool, out chan consumer.Message, logger log.Logger) (*Datagram, error) {
	var (
		err   error
		dgram = &Datagram{}
	)

	atomic.AddUint64(&metrics.Metrics.SFlowPackets, 1)
	atomic.AddUint64(&metrics.Metrics.SFlowBytes, uint64(len(payload)))
	dgram.Timestamp = time.Now().Unix()

	r := bytes.NewReader(payload)

	if err = dgram.decodeDatagramHeader(r); err != nil {
		return nil, fmt.Errorf("can't decode header: %v\nposition: %#0x\n%+v\n%s\n\n\n", err, r.Size()-int64(r.Len()), dgram, hex.Dump(payload))
	}

	for i := dgram.SamplesNo; i > 0; i-- {
		sample, err := decodeSample(r, dgram, encapEnabled, out, logger)
		if err != nil {
			return nil, err
		}

		dgram.Samples = append(dgram.Samples, sample)
	}

	return dgram, nil
}
