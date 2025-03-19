package golem

import (
	"net/http"
	"net/url"
	"path"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/rehttp"
)

var ServiceName string = "Golem"

type Golem struct {
	Object string
	Resps  []struct {
		Name string
		Type string
	}
}

type Client struct {
	Auth    interface{}
	BaseURL string
	Get     string
	Post    string
	Token   string
}

func NewClient(url, geturl, posturl, token string) (c Client) {
	c.BaseURL = url
	c.Get = geturl
	c.Post = posturl
	c.Token = token
	return
}

func (c Client) SetResps(host string, resps []string) (err error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return
	}

	r := map[string]interface{}{
		"object": host,
		"resps":  resps,
	}

	u.Path = path.Join(u.Path, c.Post)
	rh := rehttp.Client{
		ServiceName: ServiceName,
		Token:       c.Token,
		ExpectedCode: []int{
			http.StatusOK,
			http.StatusAccepted,
			http.StatusCreated,
		},
	}

	_, err = rh.RequestText("POST", u, r)
	return err
}

func (c Client) GetResps(host string) (g Golem, err error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return
	}

	p := url.Values{}
	p.Add("object", host)
	u.RawQuery = p.Encode()
	u.Path = path.Join(u.Path, c.Get)

	rh := rehttp.Client{
		ServiceName:  ServiceName,
		Token:        c.Token,
		ExpectedCode: []int{http.StatusOK},
	}

	err = rh.GetJSON(u, nil, &g)
	if err != nil {
		return
	}
	return g, nil
}
