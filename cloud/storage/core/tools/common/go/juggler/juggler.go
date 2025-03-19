package juggler

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"time"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
)

const (
	JugglerURL = "https://juggler-api.search.yandex.net"
)

type MuteID = string

type JugglerClientIface interface {
	GetDownHosts(ctx context.Context) ([]string, error)

	HasDowntime(ctx context.Context, host string) (bool, error)

	SetMutes(
		ctx context.Context,
		service string,
		host string,
		startTime time.Time,
		duration time.Duration,
		namespace string,
		project string,
	) (MuteID, error)
}

////////////////////////////////////////////////////////////////////////////////

type jugglerClient struct {
	log  nbs.Log
	http *http.Client

	ticket     string
	oauthToken string
}

type filters = map[string]string

type downtimesRequest struct {
	Filters []filters `json:"filters"`
	Page    int       `json:"page"`
}

type downtimesFilter struct {
	Host    string `json:"host"`
	Service string `json:"service"`
}

type downtimesItem struct {
	Filters []downtimesFilter `json:"filters"`
}

type downtimesResponse struct {
	Items []downtimesItem `json:"items"`
}

type mutesRequest struct {
	Filters     []filters `json:"filters"`
	StartTime   int64     `json:"start_time"`
	EndTime     int64     `json:"end_time"`
	Description string    `json:"description"`
	Project     string    `json:"project"`
}

func (j *jugglerClient) getDownHosts(ctx context.Context, page int, downtimeFilters filters) (
	[]string,
	error,
) {
	payload, _ := json.Marshal(&downtimesRequest{
		Filters: []filters{downtimeFilters},
		Page:    page,
	})

	req, err := http.NewRequest(
		"POST",
		JugglerURL+"/v2/downtimes/get_downtimes",
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return nil, fmt.Errorf("getDownHosts. Can't create request: %w", err)
	}

	req.Header.Add("Authorization", "OAuth "+j.oauthToken)
	req.Header.Add("Content-Type", "application/json;charset=UTF-8")
	req.Header.Add("Accept", "application/json")

	resp, err := j.http.Do(req)
	if err != nil {
		return nil, fmt.Errorf("getDownHosts. Request error: %w", err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, fmt.Errorf("getDownHosts. Read body error: %w", err)
	}

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("getDownHosts. Bad response: %v", resp)
	}

	r := &downtimesResponse{}

	if err := json.Unmarshal(body, r); err != nil {
		return nil, fmt.Errorf("getDownHosts. Unmarshal error: %w", err)
	}

	var result []string

	for _, item := range r.Items {
		for _, filt := range item.Filters {
			if filt.Host != "" && filt.Service == "" {
				result = append(result, filt.Host)
			}
		}
	}

	return result, nil
}

func (j *jugglerClient) GetDownHosts(ctx context.Context) (
	[]string,
	error,
) {
	var result []string

	for page := 1; ; page++ {
		hosts, err := j.getDownHosts(ctx, page, filters{"namespace": "ycloud"})
		if err != nil {
			return result, err
		}

		if len(hosts) == 0 {
			break
		}

		result = append(result, hosts...)
	}

	return result, nil
}

func (j *jugglerClient) HasDowntime(ctx context.Context, host string) (
	bool,
	error,
) {
	hosts, err := j.getDownHosts(ctx, 1, filters{
		"namespace": "ycloud",
		"host":      host,
	})
	if err != nil {
		return false, err
	}

	return len(hosts) != 0, nil
}

func (j *jugglerClient) SetMutes(
	ctx context.Context,
	service string,
	host string,
	startTime time.Time,
	duration time.Duration,
	namespace string,
	project string,
) (MuteID, error) {
	endTime := startTime.Add(duration)

	payload, _ := json.Marshal(&mutesRequest{
		Filters: []filters{
			filters{
				"namespace": namespace,
				"service":   service,
				"host":      host,
			},
		},
		StartTime:   startTime.Unix(),
		EndTime:     endTime.Unix(),
		Description: j.ticket,
		Project:     project,
	})

	req, err := http.NewRequest(
		"POST",
		JugglerURL+"/v2/mutes/set_mutes",
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return "", fmt.Errorf("SetMutes. Can't create request: %w", err)
	}

	req.Header.Add("Authorization", "OAuth "+j.oauthToken)
	req.Header.Add("Content-Type", "application/json;charset=UTF-8")
	req.Header.Add("Accept", "application/json")

	resp, err := j.http.Do(req)
	if err != nil {
		return "", fmt.Errorf("SetMutes. Request error: %w", err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return "", fmt.Errorf("SetMutes. Read body error: %w", err)
	}

	if resp.StatusCode != http.StatusOK {
		return "", fmt.Errorf("SetMutes. Bad response: %v", resp)
	}

	r := &struct {
		ID MuteID `json:"mute_id"`
	}{}

	if err := json.Unmarshal(body, r); err != nil {
		return "", fmt.Errorf("SetMutes. Unmarshal error: %w", err)
	}

	return r.ID, nil
}

////////////////////////////////////////////////////////////////////////////////

func NewClient(
	log nbs.Log,
	ticket string,
	oauthToken string,
) JugglerClientIface {
	return &jugglerClient{
		log:        log,
		http:       &http.Client{},
		ticket:     ticket,
		oauthToken: oauthToken,
	}
}
