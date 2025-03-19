package tests

import (
	"fmt"
	"time"

	"google.golang.org/grpc"

	cmsv1 "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/expectations"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcmocker/parsers"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type CMSTaskDescription struct {
	InstanceID     string        `yaml:"instance_id"`
	ExecutedSteps  []string      `yaml:"executed_steps"`
	CreatedFromNow time.Duration `yaml:"created_from_now"`
}

func NewTestCMSGrpcServer(expectations expectations.Matcher, port int) *grpcmocker.Server {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		panic(err)
	}

	p := parsers.NewDefaultParser()
	rcParser := parsers.NewResponderCodeParser()
	if err = p.RegisterResponderParser(rcParser); err != nil {
		panic(err)
	}

	server, err := grpcmocker.Run(
		fmt.Sprintf(":%d", port),
		func(s *grpc.Server) error {
			cmsv1.RegisterInstanceOperationServiceServer(s, &cmsv1.UnimplementedInstanceOperationServiceServer{})
			return nil
		},
		grpcmocker.WithExpectations(expectations),
		grpcmocker.WithLogger(l),
	)
	if err != nil {
		panic(err)
	}
	return server
}
