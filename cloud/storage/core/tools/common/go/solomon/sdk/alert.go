package sdk

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
)

////////////////////////////////////////////////////////////////////////////////

type AlertID = string

type AlertChannelConfig struct {
	NotifyAboutStatuses []string `json:"notifyAboutStatuses"`
	RepeatDelaySecs     uint     `json:"repeatDelaySecs"`
}

type AlertChannel struct {
	ID     string             `json:"id"`
	Config AlertChannelConfig `json:"config"`
}

type AlertExpression struct {
	Program         string `json:"program"`
	CheckExpression string `json:"checkExpression"`
}

type AlertType struct {
	Expression AlertExpression `json:"expression"`
}

type AlertAnnotations struct {
	Host               string `json:"host"`
	Tags               string `json:"tags"`
	Debug              string `json:"debug"`
	Service            string `json:"service"`
	Content            string `json:"content"`
	GraphLink          string `yaml:"graphLink"`
	JugglerDescription string `json:"juggler_description"`
}

type Alert struct {
	ID          string `json:"id"`
	ProjectID   string `json:"projectId"`
	Name        string `json:"name"`
	Description string `json:"description"`
	State       string `json:"state"`
	Version     uint   `json:"version"`

	GroupByLabels       []string         `json:"groupByLabels"`
	Channels            []AlertChannel   `json:"channels"`
	Type                AlertType        `json:"type"`
	Annotations         AlertAnnotations `json:"annotations"`
	PeriodMillis        uint             `json:"periodMillis"`
	WindowSecs          uint             `json:"windowSecs"`
	DelaySeconds        uint             `json:"delaySeconds"`
	DelaySecs           uint             `json:"delaySecs"`
	ResolvedEmptyPolicy string           `json:"resolvedEmptyPolicy"`
}

////////////////////////////////////////////////////////////////////////////////

type listAlertsItem struct {
	ID        string `json:"id"`
	ProjectID string `json:"projectId"`
	Name      string `json:"name"`
}

type listAlertsResponse struct {
	Items         []listAlertsItem `json:"items"`
	NextPageToken string           `json:"nextPageToken"`
}

func (s *solomonClient) listAlerts(
	ctx context.Context,
	pageToken string,
) ([]AlertID, string, error) {

	url := s.url + "/" + s.projectID + "/alerts?pageSize=1000"
	if pageToken != "" {
		url += "&pageToken=" + pageToken
	}

	body, err := s.executeRequest(ctx, "listAlerts", "GET", url, nil)
	if err != nil {
		return nil, "", err
	}

	r := &listAlertsResponse{}
	if err := json.Unmarshal(body, r); err != nil {
		return nil, "", fmt.Errorf("listAlerts. Unmarshal error: %w", err)
	}

	var result []string

	for _, item := range r.Items {
		result = append(result, item.ID)
	}

	return result, r.NextPageToken, nil
}

func (s *solomonClient) ListAlerts(
	ctx context.Context,
) ([]AlertID, error) {

	var result []string

	pageToken := ""
	for {
		alerts, nextPageToken, err := s.listAlerts(ctx, pageToken)
		if err != nil {
			return result, err
		}

		result = append(result, alerts...)
		pageToken = nextPageToken

		if pageToken == "" {
			break
		}
	}

	return result, nil
}

////////////////////////////////////////////////////////////////////////////////

func (s *solomonClient) GetAlert(
	ctx context.Context,
	alertID AlertID,
) (Alert, error) {

	url := s.url + "/" + s.projectID + "/alerts/" + alertID

	body, err := s.executeRequest(ctx, "GetAlert", "GET", url, nil)
	if err != nil {
		return Alert{}, err
	}

	r := &Alert{}
	if err := json.Unmarshal(body, r); err != nil {
		return Alert{}, fmt.Errorf("GetAlert. Unmarshal error: %w", err)
	}

	return *r, nil
}

////////////////////////////////////////////////////////////////////////////////

func (s *solomonClient) AddAlert(
	ctx context.Context,
	alert Alert,
) (Alert, error) {

	url := s.url + "/" + s.projectID + "/alerts"

	payload, err := json.Marshal(alert)
	if err != nil {
		return Alert{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"AddAlert",
		"POST",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Alert{}, err
	}

	r := &Alert{}
	if err := json.Unmarshal(body, r); err != nil {
		return Alert{}, err
	}

	return *r, nil
}

////////////////////////////////////////////////////////////////////////////////

func (s *solomonClient) UpdateAlert(
	ctx context.Context,
	alert Alert,
) (Alert, error) {

	url := s.url + "/" + s.projectID + "/alerts/" + alert.ID

	payload, err := json.Marshal(alert)
	if err != nil {
		return Alert{}, err
	}

	body, err := s.executeRequest(
		ctx,
		"UpdateAlert",
		"PUT",
		url,
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return Alert{}, err
	}

	r := &Alert{}
	if err := json.Unmarshal(body, r); err != nil {
		return Alert{}, err
	}

	return *r, nil
}

////////////////////////////////////////////////////////////////////////////////

func (s *solomonClient) DeleteAlert(
	ctx context.Context,
	alertID AlertID,
) error {

	url := s.url + "/" + s.projectID + "/alerts/" + alertID

	_, err := s.executeRequest(ctx, "DeleteAlert", "DELETE", url, nil)
	if err != nil {
		return err
	}

	return nil
}
