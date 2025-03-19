package client

import (
	"context"
	"fmt"
	"io"
	"time"

	api "a.yandex-team.ru/cloud/mdb/deploy/agent/internal/api/v1"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/defaults"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/library/go/core/log"
)

type Config struct {
	Addr      string                `json:"addr" yaml:"addr"`
	Transport grpcutil.ClientConfig `json:"transport" yaml:"transport"`
}

func DefaultConfig() Config {
	cfg := Config{
		Addr:      defaults.SocketPath,
		Transport: grpcutil.DefaultClientConfig(),
	}
	cfg.Transport.Security.Insecure = true
	return cfg
}

type CommandService struct {
	service api.CommandServiceClient
}

func New(ctx context.Context, cfg Config, l log.Logger) (*CommandService, error) {
	conn, err := grpcutil.NewConn(ctx, cfg.Addr, "deploy-agent-cli", cfg.Transport, l)
	if err != nil {
		return nil, fmt.Errorf("initialize grpc connection %w", err)
	}
	return &CommandService{service: api.NewCommandServiceClient(conn)}, nil
}

type CommandRequest struct {
	ID       string
	Cmd      string
	Args     []string
	Version  string
	Progress bool
}

type CommandResponse struct {
	PID        int
	CommandID  string
	ExitCode   int
	Error      string
	Stdout     string
	Stderr     string
	Progress   string
	StartedAt  time.Time
	FinishedAt time.Time
}

func (cs *CommandService) Run(ctx context.Context, commandRequest CommandRequest) (<-chan CommandResponse, error) {
	req := &api.CommandRequest{
		Id:       commandRequest.ID,
		Cmd:      commandRequest.Cmd,
		Args:     commandRequest.Args,
		Version:  commandRequest.Version,
		Progress: commandRequest.Progress,
	}
	stream, err := cs.service.Run(ctx, req)
	if err != nil {
		return nil, err
	}
	responses := make(chan CommandResponse)
	go func() {
		defer close(responses)
		for {
			message, err := stream.Recv()
			if err != nil {
				if err == io.EOF {
					return
				}
				responses <- CommandResponse{Error: fmt.Sprintf("error while reciving response: %s", err)}
				return
			}
			var startedAt, finishedAt time.Time
			if message.StartedAt != nil {
				startedAt = message.StartedAt.AsTime()
			}
			if message.FinishedAt != nil {
				finishedAt = message.FinishedAt.AsTime()
			}
			responses <- CommandResponse{
				PID:        int(message.Pid),
				CommandID:  message.Id,
				ExitCode:   int(message.ExitCode),
				Error:      message.Error,
				Stdout:     message.Stdout,
				Stderr:     message.Stderr,
				Progress:   message.Progress,
				StartedAt:  startedAt,
				FinishedAt: finishedAt,
			}
		}
	}()
	return responses, nil
}
