package sflow

import (
	"bytes"
	"fmt"
	"io"

	"a.yandex-team.ru/cloud/netinfra/tflow/consumer"
	"a.yandex-team.ru/cloud/netinfra/tflow/util"
	"a.yandex-team.ru/library/go/core/log"
)

/////////////////////////////////////////////////////
// Type 1 SFlowFlowSample
//  0                      15                      31
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |      20 bit Interprise (0)     |12 bit format |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                  sample length                |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |          int sample sequence number           |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |id type |       src id index value             |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |               int sampling rate               |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                int sample pool                |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    int drops                  |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                 int input ifIndex             |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                int output ifIndex             |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |               int number of records           |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  /                   flow records                /
//  /                                               /
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

/////////////////////////////////////////////////////
// Type 3 - SFlowFlowSample (expanded)
//  0                      15                      31
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |      20 bit Interprise (0)     |12 bit format |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                  sample length                |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |          int sample sequence number           |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |               int src id type                 |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |             int src id index value            |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |               int sampling rate               |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                int sample pool                |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    int drops                  |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |           int input interface format          |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |           int input interface value           |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |           int output interface format         |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |           int output interface value          |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |               int number of records           |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  /                   flow records                /
//  /                                               /
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

// FlowSample represents a sampled packet and contains
// one or more records describing the packet
type FlowSample struct {
	EnterpriseID          SFlowEnterpriseID
	Format                SampleType
	SequenceNumber        uint32
	SourceIDClass         uint32
	SourceIDIndex         uint32
	SamplingRate          uint32
	SamplePool            uint32
	Dropped               uint32
	InputInterfaceFormat  uint32
	InputInterface        uint32
	OutputInterfaceFormat uint32
	OutputInterface       uint32
	RecordCount           uint32
	Records               []Record
	samples               []consumer.MessageElement
	encaps                []consumer.MessageElement
	str                   string
}

type SFlowEnterpriseID uint32

const (
	SFlowStandard SFlowEnterpriseID = 0
)

// SampleType specifies the type of sample.
type SampleType uint32

const (
	TypeFlowSample            SampleType = 1
	TypeCounterSample         SampleType = 2
	TypeExpandedFlowSample    SampleType = 3
	TypeExpandedCounterSample SampleType = 4
)

// SampleType returns the type of sFlow sample.
func (s *FlowSample) SampleType() int {
	return int(s.Format)
}

// GetRecords returns sample records
func (s *FlowSample) GetRecords() []Record {
	return s.Records
}

// SFlowDataFormat encodes the EnterpriseID in the most
// significant 12 bits, and the SampleType in the least significant
// 20 bits.
//type SFlowDataFormat uint32
//
//func (sdf SampleType) decode() (SFlowEnterpriseID, SampleType) {
//	return SFlowEnterpriseID(sdf >> 12), SampleType(sdf & 0xfff)
//}

func decodeFlowSample(r *bytes.Reader, format SampleType, expanded bool, encapEnabled bool, d *Datagram, out chan consumer.Message, logger log.Logger) (*FlowSample, error) {
	var (
		err error
		s   = &FlowSample{}
	)

	//s.EnterpriseID, s.Format = format.decode()
	s.EnterpriseID, s.Format = SFlowEnterpriseID(format>>12), SampleType(format&0xfff)

	if err = util.ReadBigEndian(r, &s.SequenceNumber); err != nil {
		return nil, fmt.Errorf("can't read SequenceNumber: %v", err)
	}

	if expanded {
		if err = util.ReadBigEndian(r, &s.SourceIDClass); err != nil {
			return nil, fmt.Errorf("can't read SourceIDClass: %v", err)
		}
		if err = util.ReadBigEndian(r, &s.SourceIDIndex); err != nil {
			return nil, fmt.Errorf("can't read SourceIDIndex: %v", err)
		}
	} else {
		var temp uint32
		if err = util.ReadBigEndian(r, &temp); err != nil {
			return nil, fmt.Errorf("can't read id type / src id index value : %v", err)
		}
		s.SourceIDClass, s.SourceIDIndex = temp>>30, uint32(0x3FFFFFFF)&uint32(temp)
	}

	if err = util.ReadBigEndian(r, &s.SamplingRate); err != nil {
		return nil, fmt.Errorf("Can't read SamplingRate: %v", err)
	}

	if err = util.ReadBigEndian(r, &s.SamplePool); err != nil {
		return nil, fmt.Errorf("Can't read SamplePool: %v", err)
	}

	if err = util.ReadBigEndian(r, &s.Dropped); err != nil {
		return nil, fmt.Errorf("Can't read Dropped: %v", err)
	}

	if expanded {
		if err = util.ReadBigEndian(r, &s.InputInterfaceFormat); err != nil {
			return nil, fmt.Errorf("Can't read InputInterfaceFormat: %v", err)
		}

		if err = util.ReadBigEndian(r, &s.InputInterface); err != nil {
			return nil, fmt.Errorf("Can't read InputInterface: %v", err)
		}

		if err = util.ReadBigEndian(r, &s.OutputInterfaceFormat); err != nil {
			return nil, fmt.Errorf("Can't read OutputInterfaceFormat: %v", err)
		}

		if err = util.ReadBigEndian(r, &s.OutputInterface); err != nil {
			return nil, fmt.Errorf("Can't read OutputInterface: %v", err)
		}

	} else {

		if err = util.ReadBigEndian(r, &s.InputInterface); err != nil {
			return nil, fmt.Errorf("Can't read InputInterface: %v", err)
		}

		if err = util.ReadBigEndian(r, &s.OutputInterface); err != nil {
			return nil, fmt.Errorf("Can't read OutputInterface: %v", err)
		}
	}

	if err = util.ReadBigEndian(r, &s.RecordCount); err != nil {
		return nil, fmt.Errorf("Can't read RecordCount: %v", err)
	}
	s.Records = make([]Record, 0, s.RecordCount)
	s.samples = make([]consumer.MessageElement, 0, s.RecordCount)
	if encapEnabled {
		s.encaps = make([]consumer.MessageElement, 0, s.RecordCount)
	}

	sumLen := uint32(0)
	logger.Debugf("START iteration via records. seq: %v\n", s.SequenceNumber)

	for i := 0; i < int(s.RecordCount); i++ {
		format, length := uint32(0), uint32(0)

		if err = util.ReadBigEndian(r, &format); err != nil {
			return nil, fmt.Errorf("decodeFlowSample() can't read record format : %v, %d/%d, size: %d, len: %d <<<<<<<<<<<<<<", err, i, s.RecordCount, r.Size(), r.Len())
		}

		_, flowRecordType := format>>12, FlowRecordType(format&0xfff)

		if err = util.ReadBigEndian(r, &length); err != nil {
			return nil, fmt.Errorf("decodeFlowSample() can't read flow record length: %v, %d/%d, size: %d, len: %d", err, i, s.RecordCount, r.Size(), r.Len())
		}

		sumLen = sumLen + length

		record := Record(nil)
		switch flowRecordType {
		case TypeRawPacketFlow:
			logger.Debugf("RawHeader: %+v\n", record)
			rec, err := decodeRawPacketRecord(r, logger)
			if err != nil {
				logger.Errorf("decodeFlowSample()/TypeRawPacketFlow/decodeRawPacketRecord() : %v\n", err)
				return s, err
			}
			//skipRecord(r, length)
			if encapEnabled && len(rec.EncapsulatedHeaders) > 0 {
				s.encaps = append(s.encaps, Encaped{header: d, sample: s, record: &rec})
			}
			s.samples = append(s.samples, Sampled{header: d, sample: s, record: &rec})
			record = rec
		case TypeExtendedSwitchFlow:
			if err := skipRecord(r, length); err != nil {
				return nil, err
			}
			// record, err = decodeExtendedSwitchRecord(r)
			// if err != nil {
			// 	return nil, err
			// }
		case TypeEthernetFrameFlow:
			fallthrough
		case TypeIpv4Flow:
			fallthrough
		case TypeIpv6Flow:
			fallthrough
		case TypeExtendedRouterFlow:
			if err := skipRecord(r, length); err != nil {
				return nil, err
			}
		default:
			if err := skipRecord(r, length); err != nil {
				return nil, err
			}
			logger.Debugf("decodeFlowSample() skipping record. type: %v\n", flowRecordType)
			logger.Debugf("decodeFlowSample() skipping record. %d/%d, iteration: %d/%d, length: %d\n", r.Len(), r.Size(), i, s.RecordCount, length)
			continue
		}

		if record != nil {
			s.Records = append(s.Records, record)
		}
	}
	logger.Debugf("SUM %v\n", sumLen)
	logger.Debugf("END iteration via records. seq: %v\n", s.SequenceNumber)
	//TODO: frame_size moved to record string. i hope i'm able to do so
	s.str = fmt.Sprintf(
		"sampling_rate=%d\tsample_seq_number=%d\tsample_pool=%d\tdropped=%d\t"+
			"input_interface=%d\tinput_interface_format=%d\toutput_interface=%d\toutput_interface_format=%d\t",
		s.SamplingRate,
		s.SequenceNumber,
		s.SamplePool,
		s.Dropped,
		s.InputInterface,
		s.InputInterfaceFormat,
		s.OutputInterface,
		s.OutputInterfaceFormat,
	)
	return s, nil
}

func skipRecord(data *bytes.Reader, length uint32) error {
	_, err := data.Seek(int64(length), io.SeekCurrent)
	return err
}

type FlowRecordType uint32

const (
	TypeRawPacketFlow      FlowRecordType = 1
	TypeEthernetFrameFlow  FlowRecordType = 2
	TypeIpv4Flow           FlowRecordType = 3
	TypeIpv6Flow           FlowRecordType = 4
	TypeExtendedSwitchFlow FlowRecordType = 1001
	TypeExtendedRouterFlow FlowRecordType = 1002
	//TypeExtendedGatewayFlow            FlowRecordType = 1003
	//TypeExtendedUserFlow               FlowRecordType = 1004
	//TypeExtendedUrlFlow                FlowRecordType = 1005
	//TypeExtendedMlpsFlow               FlowRecordType = 1006
	//TypeExtendedNatFlow                FlowRecordType = 1007
	//TypeExtendedMlpsTunnelFlow         FlowRecordType = 1008
	//TypeExtendedMlpsVcFlow             FlowRecordType = 1009
	//TypeExtendedMlpsFecFlow            FlowRecordType = 1010
	//TypeExtendedMlpsLvpFecFlow         FlowRecordType = 1011
	//TypeExtendedVlanFlow               FlowRecordType = 1012
	//TypeExtendedIpv4TunnelEgressFlow   FlowRecordType = 1023
	//TypeExtendedIpv4TunnelIngressFlow  FlowRecordType = 1024
	//TypeExtendedIpv6TunnelEgressFlow   FlowRecordType = 1025
	//TypeExtendedIpv6TunnelIngressFlow  FlowRecordType = 1026
	//TypeExtendedDecapsulateEgressFlow  FlowRecordType = 1027
	//TypeExtendedDecapsulateIngressFlow FlowRecordType = 1028
	//TypeExtendedVniEgressFlow          FlowRecordType = 1029
	//TypeExtendedVniIngressFlow         FlowRecordType = 1030
)
