package grpcserver

import (
	"context"
	"sync"
	"time"

	"github.com/golang/protobuf/ptypes/timestamp"

	api "a.yandex-team.ru/cloud/mdb/deploy/agent/internal/api/v1"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/commander"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
)

type runningCommand struct {
	command commander.Command
	result  chan interface{}
}

type SaltService struct {
	running map[string]runningCommand
	cmds    chan commander.Command
	lock    sync.RWMutex
	l       log.Logger
}

func New(l log.Logger) *SaltService {
	return &SaltService{
		running: make(map[string]runningCommand),
		cmds:    make(chan commander.Command),
		l:       l,
	}
}

var _ commander.ProgressTrackedCommandSourcer = &SaltService{}
var _ api.CommandServiceServer = &SaltService{}

func (ss *SaltService) Commands(context.Context) <-chan commander.Command {
	return ss.cmds
}

func (ss *SaltService) Done(_ context.Context, result commander.Result) {
	ss.lock.Lock()
	defer ss.lock.Unlock()

	if running, ok := ss.running[result.CommandID]; ok {
		running.result <- result
		close(running.result)
		delete(ss.running, result.CommandID)
	}
}

func (ss *SaltService) Track(ctx context.Context, progress commander.Progress) {
	ss.lock.Lock()
	defer ss.lock.Unlock()

	if running, ok := ss.running[progress.CommandID]; ok {
		running.result <- progress
	}
}

func commandFromRequest(request *api.CommandRequest) (commander.Command, error) {
	if request == nil {
		return commander.Command{}, semerr.InvalidInput("empty request")
	}
	cmd := commander.Command{
		ID:      request.Id,
		Name:    request.Cmd,
		Args:    request.Args,
		Source:  commander.DataSource{Version: request.Version},
		Options: commander.Options{Colored: true, Progress: request.Progress},
	}
	if err := cmd.Valid(); err != nil {
		return commander.Command{}, err
	}
	return cmd, nil
}

func timeToGRPC(ts time.Time) *timestamp.Timestamp {
	if ts.IsZero() {
		return nil
	}
	return &timestamp.Timestamp{Seconds: ts.Unix(), Nanos: int32(ts.Nanosecond())}
}

func resultToResponse(r commander.Result) *api.CommandResponse {
	var respErr string
	if r.Error != nil {
		respErr = r.Error.Error()
	}
	return &api.CommandResponse{
		Id:         r.CommandID,
		ExitCode:   int32(r.ExitCode),
		Stdout:     r.Stdout,
		Stderr:     r.Stderr,
		Error:      respErr,
		StartedAt:  timeToGRPC(r.StartedAt),
		FinishedAt: timeToGRPC(r.FinishedAt),
	}
}

func progressToResponse(r commander.Progress) *api.CommandResponse {
	return &api.CommandResponse{
		Id:       r.CommandID,
		Progress: r.Message,
	}
}

func (ss *SaltService) newCommand(cmd commander.Command) (<-chan interface{}, error) {
	ss.lock.Lock()
	defer ss.lock.Unlock()

	if oldCmd, ok := ss.running[cmd.ID]; ok {
		return nil, semerr.FailedPreconditionf("Command with ID %q already exists: %v", cmd.ID, oldCmd)
	}
	result := make(chan interface{}, 100)
	ss.running[cmd.ID] = runningCommand{
		command: cmd,
		result:  result,
	}
	ss.cmds <- cmd
	return result, nil
}

func (ss *SaltService) Run(request *api.CommandRequest, stream api.CommandService_RunServer) error {
	cmd, err := commandFromRequest(request)
	if err != nil {
		return err
	}

	result, err := ss.newCommand(cmd)
	if err != nil {
		return err
	}

	for r := range result {
		var message *api.CommandResponse
		switch r := r.(type) {
		case commander.Result:
			message = resultToResponse(r)
		case commander.Progress:
			message = progressToResponse(r)
		default:
			ss.l.Errorf("SaltService got unexpected result type %+v", r)
		}
		if err := stream.Send(message); err != nil {
			return err
		}
	}
	return nil
}
