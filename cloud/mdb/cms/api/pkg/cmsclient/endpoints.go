package cmsclient

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/client/tasks"
	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/library/go/ptr"
)

func (c *client) DeleteTask(ctx context.Context, taskID string) error {
	r := tasks.NewDeleteTaskParamsWithContext(ctx).
		WithTaskID(taskID).
		WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx))).
		WithXYaServiceTicket(&c.ticket)

	_, err := c.swag.Tasks.DeleteTask(r)
	if err != nil {
		return apiError(err)
	}
	return nil
}

func (c *client) GetTasks(ctx context.Context) (*models.TasksResultsArray, error) {
	r := tasks.NewListUnhandledManagementRequestsParamsWithContext(ctx).
		WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx))).
		WithXYaServiceTicket(&c.ticket)

	resp, err := c.swag.Tasks.ListUnhandledManagementRequests(r)
	if err != nil {
		return nil, apiError(err)
	}
	return resp.Payload, nil
}
