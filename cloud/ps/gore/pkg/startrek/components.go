package startrek

import (
	"net/http"
	"net/url"
	"path"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/rehttp"
)

type Suggest struct {
	ID   int    `json:"id,omitempty"`
	Name string `json:"name,omitempty"`
}

func (c *Client) GetComponents(k, q string) ([]Suggest, error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return nil, err
	}

	p := url.Values{}
	p.Add("input", k)
	p.Add("queue", q)
	u.RawQuery = p.Encode()
	u.Path = path.Join(u.Path, c.Components)
	rh := rehttp.Client{
		ServiceName:  ServiceName,
		Token:        c.Token,
		ExpectedCode: []int{http.StatusOK},
	}

	cps := []Suggest{}
	err = rh.GetJSON(u, nil, &cps)
	return cps, err
}
