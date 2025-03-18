package main

import (
	"fmt"
	"log"
	"runtime"
	"time"

	grpc_status "google.golang.org/genproto/googleapis/rpc/status"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"

	pb "a.yandex-team.ru/antirobot/captcha_cloud_api/proto"

	uaclient "a.yandex-team.ru/library/go/yandex/uagent/log/zap/client"
	"a.yandex-team.ru/logbroker/unified_agent/client/go/ua"
)

func ErrorToGrpcStatus(err error) *grpc_status.Status {
	if err != nil {
		if statusErr, ok := status.FromError(err); ok {
			return statusErr.Proto()
		} else {
			return status.New(codes.Internal, err.Error()).Proto()
		}
	}
	return nil
}

func ErrorToProto(err error) *pb.TError {
	if err == nil {
		return nil
	}
	return &pb.TError{Error: ErrorToGrpcStatus(err)}
}

func FileLine() string {
	_, fileName, fileLine, ok := runtime.Caller(1)
	var s string
	if ok {
		s = fmt.Sprintf("%s:%d", fileName, fileLine)
	} else {
		s = ""
	}
	return s
}

type Logger struct {
	UnifiedAgentClient *ua.UAClient
	FileLogger         *log.Logger
}

func (logger *Logger) logToFile(jsonString string) {
	if logger.FileLogger == nil {
		return
	}

	logger.FileLogger.Println(jsonString)
}

func (logger *Logger) logToUnifiedAgent(payload []byte) {
	if logger.UnifiedAgentClient == nil {
		return
	}

	meta := map[string]string{}
	err := logger.UnifiedAgentClient.Send(
		[]uaclient.Message{
			{
				Payload: payload,
				Meta:    meta,
				Time:    nil,
			},
		},
	)
	if err != nil {
		log.Println("Failed to send log message to unified agent:", err)
	}
}

func (logger *Logger) Log(msg *pb.TLogRecord) {
	timestamp := uint64(time.Now().UnixNano() / 1000000)
	row := &pb.TLogRow{
		Timestamp: timestamp,
		Record:    msg,
	}

	rowBytes, err := proto.Marshal(row)
	if err != nil {
		log.Println("Failed to serialize log message:", err)
		return
	}

	jsonString := protojson.MarshalOptions{
		Multiline:       false,
		AllowPartial:    true,
		UseProtoNames:   true,
		UseEnumNumbers:  false,
		EmitUnpopulated: false,
	}.Format(msg)

	logger.logToFile(jsonString)
	//TODO: add "go"
	logger.logToUnifiedAgent(rowBytes)
}

func (logger *Logger) LogBytes(msg []byte) {
	logger.logToFile(string(msg))
	//TODO: add "go"
	logger.logToUnifiedAgent(msg)
}

func (logger *Logger) Message(msg string) {
	logger.Log(&pb.TLogRecord{GeneralMessage: &pb.TGeneralMessage{Message: msg, Level: 0}})
}

func (logger *Logger) Error(fileLine string, err error) {
	if err != nil {
		logger.Log(&pb.TLogRecord{Error: &pb.TError{Error: ErrorToGrpcStatus(err), FileLine: fileLine}})
	}
}

func (logger *Logger) LogAuditEvent(auditEvent proto.Message) {
	jsonBytes, marshalErr := protojson.MarshalOptions{
		Multiline:       false,
		AllowPartial:    true,
		UseProtoNames:   true,
		UseEnumNumbers:  false,
		EmitUnpopulated: false,
	}.Marshal(auditEvent)

	if marshalErr != nil {
		log.Println("Failed to marshal search event JSON:", marshalErr)
	} else {
		logger.LogBytes(jsonBytes)
	}
}
