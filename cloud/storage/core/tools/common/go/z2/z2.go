package z2

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
)

const (
	Z2Url = "https://z2-cloud.yandex-team.ru"
)

type Z2UpdateStatus struct {
	FailedWorkers []string `json:"failedWorkers"`
	Status        string   `json:"updateStatus"`
	Result        string   `json:"result"`
}

type Z2Item struct {
	Name    string `json:"name"`
	Version string `json:"version"`
}

type Z2ClientIface interface {
	EditItems(ctx context.Context, configID string, items []Z2Item) error
	Update(ctx context.Context, configID string, forceYes bool) error
	UpdateStatus(ctx context.Context, configID string) (*Z2UpdateStatus, error)
	ListWorkers(ctx context.Context, configID string) ([]string, error)
}

////////////////////////////////////////////////////////////////////////////////

type z2Client struct {
	logutil.WithLog
	http   *http.Client
	url    string
	apiKey string
}

type z2Response struct {
	Success      bool   `json:"success"`
	ErrorMessage string `json:"errorMsg"`
}

func (z2 *z2Client) EditItems(
	ctx context.Context,
	configID string,
	items []Z2Item,
) error {
	data := url.Values{}
	data.Set("configId", configID)
	data.Set("apiKey", z2.apiKey)

	itemsJSON, _ := json.Marshal(items)
	data.Set("items", string(itemsJSON))

	req, err := http.NewRequest(
		"POST",
		z2.url+"editItems",
		bytes.NewBufferString(data.Encode()),
	)

	if err != nil {
		return fmt.Errorf("EditItems. Can't create request: %w", err)
	}

	req.Header.Add("Content-Type", "application/x-www-form-urlencoded")
	req.Header.Add("Accept", "application/json")

	resp, err := z2.http.Do(req)
	if err != nil {
		return fmt.Errorf("EditItems. Request error: %w", err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return fmt.Errorf("EditItems. Read body error: %w", err)
	}

	z2.LogDbg(ctx, "[Z2] EditItems response: %v", resp)

	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("EditItems. Bad response: %v", resp)
	}

	m := &z2Response{}

	if err := json.Unmarshal(body, m); err != nil {
		return fmt.Errorf("EditItems. Unmarshal error: %w", err)
	}

	if !m.Success {
		return fmt.Errorf("EditItems. Error: %v", m.ErrorMessage)
	}

	return nil
}

func (z2 *z2Client) Update(ctx context.Context, configID string, forceYes bool) error {
	data := url.Values{}
	data.Set("configId", configID)
	data.Set("apiKey", z2.apiKey)
	data.Set("forceYes", func() string {
		if forceYes {
			return "1"
		}
		return "0"
	}())

	req, err := http.NewRequest(
		"POST",
		z2.url+"update",
		bytes.NewBufferString(data.Encode()),
	)

	if err != nil {
		return fmt.Errorf("Update. Can't create request: %w", err)
	}

	req.Header.Add("Content-Type", "application/x-www-form-urlencoded")
	req.Header.Add("Accept", "application/json")

	z2.LogDbg(ctx, "[Z2] Update request: %v %v", req.URL, data)

	resp, err := z2.http.Do(req)
	if err != nil {
		return fmt.Errorf("Update. Request error: %w", err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return fmt.Errorf("Update. Read body error: %w", err)
	}

	z2.LogDbg(ctx, "[Z2] Update response: %v", resp)

	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("Update. Bad response: %v", resp)
	}

	m := &z2Response{}

	if err := json.Unmarshal(body, m); err != nil {
		return fmt.Errorf("Update. Unmarshal error: %w", err)
	}

	if !m.Success {
		return fmt.Errorf("Update. Error: %v", m.ErrorMessage)
	}

	return nil
}

func (z2 *z2Client) invokeGetRequest(
	action string,
	configID string,
) (*http.Response, error) {
	req, err := http.NewRequest("GET", z2.url+action, nil)

	if err != nil {
		return nil, fmt.Errorf("can't create request: %w", err)
	}

	req.Header.Add("Accept", "application/json")

	q := req.URL.Query()
	q.Add("configId", configID)
	q.Add("apiKey", z2.apiKey)
	req.URL.RawQuery = q.Encode()

	resp, err := z2.http.Do(req)
	if err != nil {
		return nil, fmt.Errorf("request error: %w", err)
	}

	return resp, nil
}

func (z2 *z2Client) UpdateStatus(
	ctx context.Context,
	configID string,
) (*Z2UpdateStatus, error) {
	resp, err := z2.invokeGetRequest("updateStatus", configID)
	if err != nil {
		return nil, fmt.Errorf("UpdateStatus. %w", err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, fmt.Errorf("UpdateStatus. Read body error: %w", err)
	}

	z2.LogDbg(ctx, "[Z2] UpdateStatus response: %v", resp)

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("UpdateStatus. Bad response: %v", resp)
	}

	m := &struct {
		z2Response
		Response Z2UpdateStatus `json:"response"`
	}{}

	if err := json.Unmarshal(body, m); err != nil {
		return nil, fmt.Errorf("UpdateStatus. Unmarshal error: %w", err)
	}

	if !m.Success {
		return nil, fmt.Errorf("UpdateStatus. Error: %v", m.ErrorMessage)
	}

	return &m.Response, nil
}

func (z2 *z2Client) ListWorkers(
	ctx context.Context,
	configID string,
) ([]string, error) {
	resp, err := z2.invokeGetRequest("workers", configID)
	if err != nil {
		return nil, fmt.Errorf("ListWorkers '%v'. %w", configID, err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, fmt.Errorf("ListWorkers '%v'. Read body error: %w", configID, err)
	}

	z2.LogDbg(ctx, "[Z2] ListWorkers '%v' response: %v", configID, resp)

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("ListWorkers '%v'. Bad response: %v", configID, resp)
	}

	type z2Workers struct {
		Workers []string `json:"workers"`
	}

	m := &struct {
		z2Response
		Response z2Workers `json:"response"`
	}{}

	if err := json.Unmarshal(body, m); err != nil {
		return nil, fmt.Errorf("ListWorkers '%v'. Unmarshal error: %w", configID, err)
	}

	if !m.Success {
		return nil, fmt.Errorf("ListWorkers '%v'. Error: %v", configID, m.ErrorMessage)
	}

	return m.Response.Workers, nil
}

////////////////////////////////////////////////////////////////////////////////

func NewClient(
	log nbs.Log,
	apiKey string,
) Z2ClientIface {
	return &z2Client{
		WithLog: logutil.WithLog{Log: log},
		http:    &http.Client{},
		url:     Z2Url + "/api/v1/",
		apiKey:  apiKey,
	}
}
