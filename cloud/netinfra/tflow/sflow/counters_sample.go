package sflow

import (
	"bytes"
	"encoding/binary"
	"fmt"

	"a.yandex-team.ru/cloud/netinfra/tflow/util"
	"a.yandex-team.ru/library/go/core/log"
)

/////////////////////////////////////////////////////
// CounterSample
//  0                      15                      31
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |          int sample sequence number           |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |id type |       src id index value             |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |               int number of records           |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  /                counter records                /
//  /                                               /
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

// CounterSample store Counter FlowSample
type CounterSample struct {
	EnterpriseID   SFlowEnterpriseID
	Format         SampleType
	SampleLength   uint32
	SequenceNumber uint32
	SourceIDClass  SFlowSourceFormat
	SourceIDIndex  SFlowSourceValue
	RecordCount    uint32
	Records        []Record
}

type SFlowSourceFormat uint32

type SFlowSourceValue uint32

const (
	SFlowTypeSingleInterface      SFlowSourceFormat = 0
	SFlowTypePacketDiscarded      SFlowSourceFormat = 1
	SFlowTypeMultipleDestinations SFlowSourceFormat = 2
)

type SFlowCounterRecordType uint32

const (
	SFlowTypeGenericInterfaceCounters   SFlowCounterRecordType = 1
	SFlowTypeEthernetInterfaceCounters  SFlowCounterRecordType = 2
	SFlowTypeTokenRingInterfaceCounters SFlowCounterRecordType = 3
	SFlowType100BaseVGInterfaceCounters SFlowCounterRecordType = 4
	SFlowTypeVLANCounters               SFlowCounterRecordType = 5
	SFlowTypeProcessorCounters          SFlowCounterRecordType = 1001
)

type SFlowCounterDataFormat uint32

type SFlowDataSourceExpanded struct {
	SourceIDClass SFlowSourceFormat
	SourceIDIndex SFlowSourceValue
}

// SampleType returns the type of sFlow sample.
func (s *CounterSample) SampleType() int {
	return int(s.Format)
}

// GetRecords returns sample records
func (s *CounterSample) GetRecords() []Record {
	return s.Records
}

func decodeCounterSample(r *bytes.Reader, format SampleType, expanded bool, logger log.Logger) (Sample, error) {
	var (
		s   = &CounterSample{}
		err error
	)

	if err = util.ReadBigEndian(r, &s.SequenceNumber); err != nil {
		return nil, fmt.Errorf("decodeCounterSample() can't read SequenceNumber, %s", err)
	}

	if expanded {
		if err := util.ReadASet(r, &s.SourceIDClass, &s.SourceIDIndex); err != nil {
			return nil, err
		}
	} else {
		var sdc uint32
		if err := util.ReadBigEndian(r, &sdc); err != nil {
			return nil, err
		}
		s.SourceIDClass, s.SourceIDIndex = SFlowSourceFormat(sdc>>30), SFlowSourceValue(uint32(0x3FFFFFFF)&uint32(sdc))
	}
	if err := util.ReadBigEndian(r, &s.RecordCount); err != nil {
		return nil, err
	}

	logger.Debugf(
		"s.EnterpriseID %d s.Format %d s.SampleLength %d s.SequenceNumber %d s.SourceIDClass %d s.SourceIDIndex %d s.RecordCount %d\n",
		s.EnterpriseID, s.Format, s.SampleLength, s.SequenceNumber, s.SourceIDClass, s.SourceIDIndex, s.RecordCount,
	)

	for i := uint32(0); i < s.RecordCount; i++ {

		var (
			cdf    SFlowCounterDataFormat
			length = uint32(0)
		)
		if err := util.ReadBigEndian(r, &cdf); err != nil {
			return nil, err
		}
		_, counterRecordType := (cdf >> 12), SFlowCounterRecordType(uint32(0xFFF)&uint32(cdf))

		err = binary.Read(r, binary.BigEndian, &length)
		if err != nil {
			return nil, err
		}

		switch counterRecordType {
		case SFlowTypeGenericInterfaceCounters:
			if err := skipRecord(r, length); err != nil {
				return nil, err
			}
			// logger.Debugf("===SFlowTypeGenericInterfaceCounters===\n")
			// if record, err := decodeGenericInterfaceCounters(r); err == nil {
			// 	s.Records = append(s.Records, record)
			// } else {
			// 	return s, err
			// }
		case SFlowTypeEthernetInterfaceCounters:
			if err := skipRecord(r, length); err != nil {
				return nil, err
			}
			// logger.Debugf("===SFlowTypeEthernetInterfaceCounters===\n")
			//return s, errors.New("===SFlowTypeEthernetInterfaceCounters===")
			// if record, err := decodeEthernetCounters(r); err == nil {
			// 	s.Records = append(s.Records, record)
			// } else {
			// 	return s, err
			// }
		case SFlowTypeVLANCounters:
			if err := skipRecord(r, length); err != nil {
				return nil, err
			}
			// logger.Debugf("skipping TypeVLANCounters\n")
			//return s, errors.New("skipping TypeVLANCounters")
		case SFlowTypeProcessorCounters:
			if err := skipRecord(r, length); err != nil {
				return nil, err
			}
			// logger.Debugf("===SFlowTypeProcessorCounters===\n")
			//return s, errors.New("===SFlowTypeProcessorCounters===")
			// if record, err := decodeProcessorCounters(r); err == nil {
			// 	s.Records = append(s.Records, record)
			// } else {
			// 	return s, err
			// }
		default:
			if err := skipRecord(r, length); err != nil {
				return nil, err
			}
			logger.Debugf("decodeCounterSample() skipping record. type: %v\n", counterRecordType)
			logger.Debugf("decodeCounterSample() skipping record. %d/%d\n", r.Len(), r.Size())
			continue
		}
	}
	return s, nil
}

////////////////////////////////////////////////////
//  Interface Counters Record
////////////////////////////////////////////////////
//  0                      15                      31
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |      20 bit Interprise (0)     |12 bit format |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                  counter length               |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    IfIndex                    |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    IfType                     |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                   IfSpeed                     |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                   IfDirection                 |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    IfStatus                   |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                   IFInOctets                  |
//  |                                               |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                   IfInUcastPkts               |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                  IfInMulticastPkts            |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                  IfInBroadcastPkts            |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    IfInDiscards               |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    InInErrors                 |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                  IfInUnknownProtos            |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                   IfOutOctets                 |
//  |                                               |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                   IfOutUcastPkts              |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                  IfOutMulticastPkts           |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                  IfOutBroadcastPkts           |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                   IfOutDiscards               |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                    IfOUtErrors                |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//  |                 IfPromiscouousMode            |
//  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

type SFlowGenericInterfaceCounters struct {
	IfIndex            uint32
	IfType             uint32
	IfSpeed            uint64
	IfDirection        uint32
	IfStatus           uint32
	IfInOctets         uint64
	IfInUcastPkts      uint32
	IfInMulticastPkts  uint32
	IfInBroadcastPkts  uint32
	IfInDiscards       uint32
	IfInErrors         uint32
	IfInUnknownProtos  uint32
	IfOutOctets        uint64
	IfOutUcastPkts     uint32
	IfOutMulticastPkts uint32
	IfOutBroadcastPkts uint32
	IfOutDiscards      uint32
	IfOutErrors        uint32
	IfPromiscuousMode  uint32
}

func decodeGenericInterfaceCounters(data *bytes.Reader) (SFlowGenericInterfaceCounters, error) {
	gic := SFlowGenericInterfaceCounters{}
	var length uint32
	if err := util.ReadBigEndian(data, &length); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfIndex); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfType); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfSpeed); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfDirection); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfStatus); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfInOctets); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfInUcastPkts); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfInMulticastPkts); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfInBroadcastPkts); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfInDiscards); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfInErrors); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfInUnknownProtos); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfOutOctets); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfOutUcastPkts); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfOutMulticastPkts); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfOutBroadcastPkts); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfOutDiscards); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfOutErrors); err != nil {
		return gic, err
	}
	if err := util.ReadBigEndian(data, &gic.IfPromiscuousMode); err != nil {
		return gic, err
	}
	return gic, nil
}
