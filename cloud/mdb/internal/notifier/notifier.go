package notifier

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
)

//go:generate ../../scripts/mockgen.sh API

type TransportID int

const (
	TransportMail TransportID = iota + 1
	TransportWeb
	TransportSms
)

type TemplateID int

const (
	MaintenanceScheduleTemplate TemplateID = iota + 1
	MaintenancesScheduleTemplate
)

type API interface {
	ready.Checker
	NotifyCloud(ctx context.Context, cloudID string, templateID TemplateID, templateData map[string]interface{}, transportIDs []TransportID) error
	NotifyUser(ctx context.Context, userID string, templateID TemplateID, templateData map[string]interface{}, transportIDs []TransportID) error
}
