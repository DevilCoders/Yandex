package notifieremulator

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/notifier"
	"a.yandex-team.ru/library/go/core/log"
)

type Notification struct {
	CloudID      string
	UserID       string
	TemplateID   notifier.TemplateID
	TemplateData map[string]interface{}
	TransportIDs []notifier.TransportID
}

func NewAPI(logger log.Logger) *API {
	return &API{logger: logger}
}

type API struct {
	notifications []Notification
	logger        log.Logger
}

var _ notifier.API = &API{}

func (a *API) IsReady(ctx context.Context) error {
	return nil
}

func (a *API) NotifyCloud(ctx context.Context, cloudID string, templateID notifier.TemplateID, templateData map[string]interface{}, transportIDs []notifier.TransportID) error {
	n := Notification{
		CloudID:      cloudID,
		TemplateID:   templateID,
		TemplateData: templateData,
		TransportIDs: transportIDs,
	}
	a.logger.Debugf("New notification %v", n)
	a.notifications = append(a.notifications, n)
	return nil
}

func (a *API) NotifyUser(ctx context.Context, userID string, templateID notifier.TemplateID, templateData map[string]interface{}, transportIDs []notifier.TransportID) error {
	n := Notification{
		UserID:       userID,
		TemplateID:   templateID,
		TemplateData: templateData,
		TransportIDs: transportIDs,
	}
	a.logger.Debugf("New notification %v", n)
	a.notifications = append(a.notifications, n)
	return nil
}

func (a *API) LastNotification() (Notification, bool) {
	if len(a.notifications) == 0 {
		return Notification{}, false
	}
	return a.notifications[len(a.notifications)-1], true
}

func (a *API) Notifications() []Notification {
	return a.notifications
}

func (a *API) Clear() {
	a.notifications = []Notification{}
}
