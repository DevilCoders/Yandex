package main

import (
	"github.com/golang/protobuf/ptypes/timestamp"
	"time"
)

func Time2Proto(t time.Time) *timestamp.Timestamp {
	micro := t.UnixMicro()
	return &timestamp.Timestamp{Seconds: micro / 1000000, Nanos: int32(micro % 1000000 * 1000)} // округляем до 1000, потому как YDB хранит с такой точностью
}
