package main

import (
	"log"
	"time"

	"google.golang.org/protobuf/proto"

	"a.yandex-team.ru/antirobot/device_validator/proto"
)

func (env *Env) Log(msg *device_validator_pb.TLogRecord) {
	if env.UnifiedAgentClient != nil {
		go env.LogSync(msg)
	}
}

func (env *Env) LogSync(msg *device_validator_pb.TLogRecord) {
	if env.UnifiedAgentClient == nil {
		return
	}

	timestamp := uint64(time.Now().UnixNano() / 1000000)
	row := &device_validator_pb.TLogRow{
		Timestamp: &timestamp,
		Record:    msg,
	}

	rowBytes, err := proto.Marshal(row)
	if err != nil {
		env.Stats.LoggingErrors.Inc()
		log.Println("Failed to serialize log message:", err)
		return
	}

	meta := map[string]string{}

	err = env.UnifiedAgentClient.Send(rowBytes, meta)
	if err != nil {
		env.Stats.LoggingErrors.Inc()
		log.Println("Failed to send log message to unified agent:", err)
	}
}
