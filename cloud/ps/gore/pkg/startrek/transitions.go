package startrek

import (
	"net/http"
	"net/url"
	"path"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/rehttp"
)

type Transition struct {
	ID string `json:"id"`
}

func (c *Client) UpdateTransitions(k, t string) error {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return err
	}

	u.Path = path.Join(u.Path, c.Issues, k, c.Transitions, t, "_execute")

	rh := rehttp.Client{
		ServiceName: ServiceName,
		Token:       c.Token,
		ExpectedCode: []int{
			http.StatusOK,
			http.StatusAccepted,
			http.StatusCreated,
		},
	}

	_, err = rh.RequestText("POST", u, nil)
	return err
}

func (c *Client) CheckTransitions(k, t string) (bool, error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return false, err
	}

	u.Path = path.Join(u.Path, c.Issues, k, c.Transitions)
	rh := rehttp.Client{
		ServiceName:  ServiceName,
		Token:        c.Token,
		ExpectedCode: []int{http.StatusOK},
	}

	trs := []Transition{}

	err = rh.GetJSON(u, nil, &trs)
	if err != nil {
		return false, err
	}

	for _, tr := range trs {
		if tr.ID == t {
			return true, nil
		}
	}

	return false, nil
}
