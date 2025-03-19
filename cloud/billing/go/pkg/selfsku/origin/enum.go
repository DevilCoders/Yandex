package origin

import (
	"fmt"
	"strconv"
)

//go:generate stringer -type=UsageType -linecomment -output enums_string.go

// UsageType of sku record
type UsageType uint8

const (
	DeltaUsage      UsageType = iota // delta
	CumulativeUsage                  // cumulative
)

func (v *UsageType) UnmarshalText(text []byte) error {
	switch string(text) {
	case DeltaUsage.String():
		*v = DeltaUsage
	case CumulativeUsage.String():
		*v = CumulativeUsage
	default:
		return fmt.Errorf("unknown usage type %s", strconv.Quote(string(text)))
	}
	return nil
}

func (v UsageType) MarshalText() ([]byte, error) {
	return []byte(v.String()), nil
}
