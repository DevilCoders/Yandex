package grpc

import (
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/services/env"
)

var EmptyPB = &empty.Empty{}

type baseService struct {
	*env.Env
}

// TODO: common services stuff should be placed here.
