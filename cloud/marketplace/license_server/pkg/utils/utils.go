package utils

import (
	"time"

	"google.golang.org/protobuf/types/known/timestamppb"
)

func GetTimeNow() time.Time {
	ts := time.Now().UTC().Unix()
	return time.Unix(ts, 0).UTC()
}

func GetTimestamppbFromTime(t time.Time) *timestamppb.Timestamp {
	ts := timestamppb.New(t)
	ts.Nanos = 0
	return ts
}
