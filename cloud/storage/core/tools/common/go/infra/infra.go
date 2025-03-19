package infra

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"time"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
)

const (
	InfraURL = "https://infra-api.yandex-team.ru"
)

type EventID = int

type InfraClientIface interface {
	CreateEvent(
		ctx context.Context,
		serviceID int,
		environmentID int,
		title string,
		startTime time.Time,
		duration time.Duration,
	) (EventID, error)

	ExtendEvent(ctx context.Context, eventID EventID, finishTime time.Time) error
}

////////////////////////////////////////////////////////////////////////////////

type infraClient struct {
	logutil.WithLog
	http *http.Client

	ticket     string
	oauthToken string
}

type infraRequest struct {
	ServiceID              int    `json:"serviceId"`
	EnvironmentID          int    `json:"environmentId"`
	Title                  string `json:"title"`
	Description            string `json:"description"`
	Type                   string `json:"type"`
	Severity               string `json:"severity"`
	SendEmailNotifications bool   `json:"sendEmailNotifications"`
	Tickets                string `json:"tickets"`
	StartTime              int64  `json:"startTime"`
	FinishTime             int64  `json:"finishTime"`
	Dc                     bool   `json:"dc"`
}

func (i *infraClient) CreateEvent(
	ctx context.Context,
	serviceID int,
	environmentID int,
	title string,
	startTime time.Time,
	duration time.Duration,
) (EventID, error) {

	finishTime := startTime.Add(duration)

	payload, _ := json.Marshal(&infraRequest{
		ServiceID:              serviceID,
		EnvironmentID:          environmentID,
		Title:                  title,
		Description:            "",
		Type:                   "maintenance",
		Severity:               "minor",
		SendEmailNotifications: true,
		Tickets:                i.ticket,
		StartTime:              startTime.Unix(),
		FinishTime:             finishTime.Unix(),
		Dc:                     true,
	})

	req, err := http.NewRequest(
		"POST",
		InfraURL+"/v1/events",
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return 0, fmt.Errorf("CreateEvent. Can't create request: %w", err)
	}

	req.Header.Add("Authorization", "OAuth "+i.oauthToken)
	req.Header.Add("Content-Type", "application/json;charset=UTF-8")
	req.Header.Add("Accept", "application/json")

	resp, err := i.http.Do(req)
	if err != nil {
		return 0, fmt.Errorf("CreateEvent. Request error: %w", err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return 0, fmt.Errorf("CreateEvent. Read body error: %w", err)
	}

	if resp.StatusCode != http.StatusOK {
		return 0, fmt.Errorf("CreateEvent. Bad response: %v", resp)
	}

	i.LogDbg(ctx, "[INFRA] CreateEvent response: %v", string(body))

	r := &struct {
		ID EventID `json:"id"`
	}{}

	if err := json.Unmarshal(body, r); err != nil {
		return 0, fmt.Errorf("CreateEvent. Unmarshal error: %w", err)
	}

	return r.ID, nil
}

func (i *infraClient) ExtendEvent(
	ctx context.Context,
	eventID EventID,
	finishTime time.Time,
) error {

	payload := []byte(fmt.Sprintf(`{ "finishTime": %v }`, finishTime.Unix()))

	req, err := http.NewRequest(
		"PUT",
		fmt.Sprintf("%v/v1/events/%v", InfraURL, eventID),
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return fmt.Errorf("ExtendEvent. Can't create request: %w", err)
	}

	req.Header.Add("Authorization", "OAuth "+i.oauthToken)
	req.Header.Add("Content-Type", "application/json;charset=UTF-8")
	req.Header.Add("Accept", "application/json")

	resp, err := i.http.Do(req)
	if err != nil {
		return fmt.Errorf("ExtendEvent. Request error: %w", err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return fmt.Errorf("ExtendEvent. Read body error: %w", err)
	}

	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("ExtendEvent. Bad response: %v", resp)
	}

	i.LogDbg(ctx, "[INFRA] ExtendEvent response: %v", string(body))

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func NewClient(
	log nbs.Log,
	ticket string,
	oauthToken string,
) InfraClientIface {
	return &infraClient{
		WithLog:    logutil.WithLog{Log: log},
		http:       &http.Client{},
		ticket:     ticket,
		oauthToken: oauthToken,
	}
}
