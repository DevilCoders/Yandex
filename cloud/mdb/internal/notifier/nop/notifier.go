package nop

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/notifier"
)

type api struct{}

var _ notifier.API = &api{}

func NewAPI() *api {
	return &api{}
}

func (a *api) NotifyCloud(ctx context.Context, cloudID string, templateID notifier.TemplateID, templateData map[string]interface{}, transportIDs []notifier.TransportID) error {
	return nil
}

func (a *api) NotifyUser(ctx context.Context, userID string, templateID notifier.TemplateID, templateData map[string]interface{}, transportIDs []notifier.TransportID) error {
	return nil
}

func (a *api) IsReady(ctx context.Context) error {
	return nil
}
