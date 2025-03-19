package tasksclient

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient/models"
)

//go:generate ../../../../scripts/mockgen.sh Client

type Client interface {
	CreateMoveInstanceTask(ctx context.Context, fqdn string, from string) (taskID string, err error)
	TaskStatus(ctx context.Context, taskID string) (models.TaskStatus, error)
}
