package http

import (
	"fmt"
	"net/http"
	"net/url"
	"path"
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/auth"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/rehttp"
	"a.yandex-team.ru/cloud/ps/gore/pkg/abc"
)

const ServiceName string = "abc"

type client struct {
	BaseURL string
	Get     string
	Token   string
}

func NewClient(url, geturl string) (abc.Client, error) {
	return &client{
		BaseURL: url,
		Get:     geturl,
	}, nil
}

func (c *client) Authenticate(a auth.AuthType, t string) abc.Client {
	if a == auth.OAuth {
		c.Token = t
		return c
	}

	return c
}

func (c *client) GetShifts(from, to time.Time, sid, schID uint64) (shs []abc.Shift, err error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return
	}

	p := url.Values{}
	p.Add("fields", "schedule,person,replaces,is_approved,start_datetime,end_datetime")
	p.Add("date_from", from.Format("2006-01-02"))
	p.Add("date_to", to.Format("2006-01-02"))
	p.Add("service", fmt.Sprintf("%d", sid))
	p.Add("schedule", fmt.Sprintf("%d", schID))
	p.Add("with_watcher", "1")
	u.RawQuery = p.Encode()
	u.Path = path.Join(u.Path, c.Get) + "/"
	rh := rehttp.Client{
		ServiceName:  ServiceName,
		Token:        c.Token,
		ExpectedCode: []int{http.StatusOK},
	}

	var r abc.Response
	err = rh.GetJSON(u, nil, &r)
	if err != nil {
		return
	}

	if len(r.Result) < 1 {
		return nil, fmt.Errorf("no shifts found")
	}

	return r.Result, nil
}
