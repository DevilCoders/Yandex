package solomon

import (
	"context"

	"a.yandex-team.ru/admins/combaine/senders"
	"a.yandex-team.ru/admins/combaine/senders/pb"
	"a.yandex-team.ru/library/go/core/log"
	"github.com/vmihailenco/msgpack"
)

var (
	defaultFields = []string{
		"75_prc", "90_prc", "93_prc",
		"94_prc", "95_prc", "96_prc",
		"97_prc", "98_prc", "99_prc",
	}
)

type grpcService struct {
	*Config
}

// DoSend repack request and send sensort to solomon api
func (s *grpcService) DoSend(ctx context.Context, req *pb.SenderRequest) (*pb.SenderResponse, error) {
	log := senders.Logger.With(log.String("session", req.Id))

	var tc TaskConfig
	err := msgpack.Unmarshal(req.Config, &tc)
	if err != nil {
		return nil, err
	}
	tc.L = log
	tc.Timestamp = req.PrevTime
	tc.updateByServerConfig(s.Config)
	err = tc.Validate()
	if err != nil {
		log.Errorf("TaskConfig validation failed: %s", err)
		return nil, err
	}

	payload, err := senders.RepackSenderRequest(req, log)
	if err != nil {
		log.Errorf("Failed to repack sender request: %v", err)
		return nil, err
	}
	err = tc.Send(payload)
	if err != nil {
		log.Errorf("Sending error %s", err)
		return nil, err
	}
	return &pb.SenderResponse{Response: "Ok"}, nil
}

func NewGRPCService(configPath string) pb.SenderServer {
	conf, err := LoadSenderConfig(configPath)
	if err != nil {
		senders.Logger.Fatalf("failed to load config: %v", err)
	}
	return &grpcService{Config: conf}
}
