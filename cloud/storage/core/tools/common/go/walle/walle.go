package walle

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"strconv"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
)

const (
	WalleURL = "https://api.wall-e.yandex-team.ru/v1"
)

type WalleClientIface interface {
	SplitByLocation(ctx context.Context, hosts []string) (map[string][]string, error)
}

////////////////////////////////////////////////////////////////////////////////

type walleClient struct {
	logutil.WithLog

	http *http.Client
}

type splitRequest struct {
	Names []string `json:"names"`
}

type location struct {
	Queue string `json:"queue"`
	Rack  string `json:"rack"`
}

type resultItem struct {
	Name     string   `json:"name"`
	Location location `json:"location"`
}

type splitResponse struct {
	NextCursor int          `json:"next_cursor"`
	Result     []resultItem `json:"result"`
}

func (w *walleClient) splitByLocation(
	ctx context.Context,
	hosts []string,
	cursor int,
) (map[string][]string, int, error) {

	payload, _ := json.Marshal(&splitRequest{
		Names: hosts,
	})

	req, err := http.NewRequest(
		"POST",
		WalleURL+"/get-hosts",
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return nil, 0, fmt.Errorf("SplitByLocation. Can't create request: %w", err)
	}

	req.Header.Add("Content-Type", "application/json;charset=UTF-8")
	req.Header.Add("Accept", "application/json")

	q := req.URL.Query()
	q.Add("fields", "name,location.queue,location.rack")
	q.Add("cursor", strconv.Itoa(cursor))
	req.URL.RawQuery = q.Encode()

	resp, err := w.http.Do(req)
	if err != nil {
		return nil, 0, fmt.Errorf("SplitByLocation. Request error: %w", err)
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, 0, fmt.Errorf("SplitByLocation. Read body error: %w", err)
	}

	if resp.StatusCode != http.StatusOK {
		return nil, 0, fmt.Errorf("SplitByLocation. Bad response: %v", resp)
	}

	r := &splitResponse{}

	if err := json.Unmarshal(body, r); err != nil {
		return nil, 0, fmt.Errorf("SplitByLocation. Unmarshal error: %w", err)
	}

	result := make(map[string][]string)

	for _, host := range r.Result {
		key := fmt.Sprintf("%v:%v", host.Location.Queue, host.Location.Rack)
		result[key] = append(result[key], host.Name)
	}

	return result, r.NextCursor, nil
}

func (w *walleClient) SplitByLocation(
	ctx context.Context,
	hosts []string,
) (map[string][]string, error) {
	result := make(map[string][]string)

	var (
		cursor int
		err    error
		r      map[string][]string
	)

	for {
		r, cursor, err = w.splitByLocation(ctx, hosts, cursor)
		if err != nil {
			return result, err
		}

		for k, v := range r {
			result[k] = append(result[k], v...)
		}

		if cursor == 0 {
			break
		}
	}

	return result, nil
}

////////////////////////////////////////////////////////////////////////////////

func NewClient(
	log nbs.Log,
) WalleClientIface {
	return &walleClient{
		WithLog: logutil.WithLog{Log: log},
		http:    &http.Client{},
	}
}
