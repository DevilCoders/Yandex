package startrek

import (
	"net/http"
	"net/url"
	"path"
	"strconv"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/rehttp"
)

type QueueDafaults struct {
	AppendFields []string `json:"appendFields,omitempty"`
	Version      *int     `json:"version,omitempty"`
	Values       struct {
		Assignee  Person   `json:"assignee,omitempty"`
		Duty      []Person `json:"duty,omitempty"`
		Followers []Person `json:"followers,omitempty"`
	} `json:"values,omitempty"`
}

func (c *Client) UpdateQueueValues(q QueueDafaults, k string, id int) (err error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return
	}

	p := url.Values{}
	p.Add("version", strconv.FormatInt(int64(*q.Version), 10))
	u.RawQuery = p.Encode()
	q.Version = nil

	u.Path = path.Join(u.Path, c.Queues, k, "defaultValues", strconv.FormatInt(int64(id), 10))
	rh := rehttp.Client{
		ServiceName:  ServiceName,
		Token:        c.Token,
		ExpectedCode: []int{http.StatusOK},
	}

	_, err = rh.RequestText("PUT", u, q)

	return err
}

func (c *Client) GetQueueValues(k string, id int) (q QueueDafaults, err error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return
	}

	u.Path = path.Join(u.Path, c.Queues, k, "defaultValues", strconv.FormatInt(int64(id), 10))

	rh := rehttp.Client{
		ServiceName:  ServiceName,
		Token:        c.Token,
		ExpectedCode: []int{http.StatusOK},
	}

	err = rh.GetJSON(u, nil, &q)

	return q, err
}
