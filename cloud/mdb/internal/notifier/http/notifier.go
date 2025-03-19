package http

import (
	"bytes"
	"context"
	"encoding/json"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/notifier"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	Endpoint  string                   `json:"endpoint" yaml:"endpoint"`
	Transport httputil.TransportConfig `json:"transport" yaml:"transport"`
}

// https://github.yandex-team.ru/data-ui/notify/blob/master/docs/api.md
type sendBody struct {
	Type       string                 `json:"type"`
	Data       map[string]interface{} `json:"data,omitempty"`
	UserID     string                 `json:"userId,omitempty"`
	CloudID    string                 `json:"cloudId,omitempty"`
	Transports []string               `json:"transports,omitempty"`
}

func DefaultConfig() Config {
	return Config{
		Transport: httputil.DefaultTransportConfig(),
	}
}

func NewAPI(cfg Config, l log.Logger) (*api, error) {
	client, err := httputil.NewClient(httputil.ClientConfig{Name: "MDB Maintenance Notifier", Transport: cfg.Transport}, l)
	if err != nil {
		return nil, err
	}
	return &api{
		endpoint: cfg.Endpoint,
		client:   client,
	}, nil
}

type api struct {
	endpoint string
	client   *httputil.Client
}

var _ notifier.API = &api{}

func getTemplateByID(templateID notifier.TemplateID) (string, error) {
	switch templateID {
	case notifier.MaintenanceScheduleTemplate:
		return "mdb.maintenance.schedule", nil
	case notifier.MaintenancesScheduleTemplate:
		return "mdb.maintenances.schedule", nil
	}
	return "", xerrors.Errorf("unknown template id %v", templateID)
}

func getTransportByID(transportID notifier.TransportID) (string, error) {
	switch transportID {
	case notifier.TransportMail:
		return "mail", nil
	case notifier.TransportWeb:
		return "web", nil
	case notifier.TransportSms:
		return "sms", nil
	}
	return "", xerrors.Errorf("unknown transport id %v", transportID)
}

func getTransportsByID(transportIDs []notifier.TransportID) ([]string, error) {
	var transports = make([]string, 0, len(transportIDs))
	for _, tid := range transportIDs {
		transport, err := getTransportByID(tid)
		if err != nil {
			return nil, err
		}
		transports = append(transports, transport)
	}
	return transports, nil
}

func (a *api) notify(ctx context.Context, body sendBody) error {
	if a.endpoint == "" {
		// Do not failed without endpoint inf config
		return nil
	}
	url, err := httputil.JoinURL(a.endpoint, "v1/send")
	if err != nil {
		return err
	}

	payload, err := json.Marshal(body)
	if err != nil {
		return xerrors.Errorf("send body marshaling failed: %v", err)
	}

	req, err := http.NewRequestWithContext(ctx, http.MethodPost, url, bytes.NewBuffer(payload))
	if err != nil {
		return err
	}
	req.Header.Set("Accept", "application/json")
	req.Header.Set("Content-Type", "application/json")
	requestID := requestid.New()
	req.Header.Set("X-Request-Id", requestID)

	resp, err := a.client.Do(req, "Notification Send")
	if err != nil {
		return err
	}
	defer func() { _ = resp.Body.Close() }()

	if resp.StatusCode >= 400 {
		return xerrors.Errorf("request with id %v failed with error code %d", requestID, resp.StatusCode)
	}
	return nil
}

func (a *api) NotifyCloud(ctx context.Context, cloudID string, templateID notifier.TemplateID, templateData map[string]interface{}, transportIDs []notifier.TransportID) error {
	template, err := getTemplateByID(templateID)
	if err != nil {
		return err
	}

	transports, err := getTransportsByID(transportIDs)
	if err != nil {
		return err
	}

	body := sendBody{
		Type:       template,
		Data:       templateData,
		Transports: transports,
		CloudID:    cloudID,
	}
	return a.notify(ctx, body)
}

func (a *api) NotifyUser(ctx context.Context, userID string, templateID notifier.TemplateID, templateData map[string]interface{}, transportIDs []notifier.TransportID) error {
	template, err := getTemplateByID(templateID)
	if err != nil {
		return err
	}

	transports, err := getTransportsByID(transportIDs)
	if err != nil {
		return err
	}

	body := sendBody{
		Type:       template,
		Data:       templateData,
		Transports: transports,
		UserID:     userID,
	}
	return a.notify(ctx, body)
}

func (a *api) IsReady(ctx context.Context) error {
	url, err := httputil.JoinURL(a.endpoint, "ping")
	if err != nil {
		return err
	}
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, url, http.NoBody)
	if err != nil {
		return err
	}

	resp, err := a.client.Do(req, "Notification Ping")
	if err != nil {
		return err
	}
	defer func() { _ = resp.Body.Close() }()

	return err
}
