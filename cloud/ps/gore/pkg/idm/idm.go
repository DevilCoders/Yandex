package idm

import (
	"fmt"
	"net/http"
	"net/url"
	"path"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/rehttp"
)

const ServiceName string = "IDM"

// A lot of effort was buried here. Please, pay respects.
// F
type IDMRequest struct {
	Simulate  bool   `json:"simulate,omitempty"`
	Silent    bool   `json:"silent,omitempty"`
	System    string `json:"system,omitempty"`
	User      string `json:"user,omitempty"`
	Type      string `json:"type,omitempty"`
	Path      string `json:"path,omitempty"`
	DepriveAt string `json:"deprive_at,omitempty"`
	//Limit     int    `json:"limit,omitempty"`
}

type Client struct {
	BaseURL  string
	Get      string
	Post     string
	SlugPath string
	System   string
	Token    string
}

func NewClient(url, geturl, posturl, path, system string) *Client {
	return &Client{
		BaseURL:  url,
		Get:      geturl,
		Post:     posturl,
		SlugPath: path,
		System:   system,
	}
}

func (c *Client) SetOAuthToken(t string) *Client {
	c.Token = t
	return c
}

func (c *Client) GetTotalActiveRoles(id int64, user string) (int64, error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return 0, err
	}

	u.RawQuery = fmt.Sprintf("system=%s&user=%s&type=active&path=%s%d/", c.System, user, c.SlugPath, id)
	u.Path = path.Join(u.Path, c.Get) + "/"

	rh := rehttp.Client{
		ServiceName:  ServiceName,
		Token:        c.Token,
		ExpectedCode: []int{http.StatusOK},
	}

	ir := struct {
		Meta struct {
			Total int64 `json:"total_count"`
		} `json:"meta"`
	}{}

	err = rh.GetJSON(u, nil, &ir)
	if err != nil {
		return 0, err
	}

	return ir.Meta.Total, err
}

func (c *Client) SetRole(user, date string, id int64) error {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return err
	}

	u.Path = path.Join(u.Path, c.Post) + "/"

	r := IDMRequest{
		Simulate:  false,
		Silent:    true,
		System:    c.System,
		User:      user,
		Type:      "active",
		Path:      fmt.Sprintf("%s%d/", c.SlugPath, id),
		DepriveAt: date,
	}

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
