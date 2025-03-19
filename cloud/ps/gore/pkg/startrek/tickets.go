package startrek

import (
	"net/http"
	"net/url"
	"path"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/rehttp"
)

type ChecklistItem struct {
	Text    string `json:"text,omitempty"`
	Checked bool   `json:"checked,omitempty"`
}

type Component struct {
	ID   string `json:"id,omitempty"`
	Name string `json:"display,omitempty"`
}

type Person struct {
	Display string `json:"display,omitempty"`
	Login   string `json:"id,omitempty"`
}

type Link struct {
	Issue        string `json:"issue,omitempty"`
	Relationship string `json:"relationship,omitempty"`
}

type Ticket struct {
	Assignee       Person          `json:"assignee,omitempty"`
	ChecklistItems []ChecklistItem `json:"checklistItems,omitempty"`
	Components     []Component     `json:"components,omitempty"`
	CreatedAt      string          `json:"createdAt,omitempty"`
	Description    string          `json:"description,omitempty"`
	DescRender     string          `json:"descriptionRenderType,omitempty"`
	Followers      []Person        `json:"followers,omitempty"`
	Key            string          `json:"key,omitempty"`
	Links          []Link          `json:"links,omitempty"`
	Tags           []string        `json:"tags,omitempty"`
	Queue          *struct {
		Key string `json:"key,omitempty"`
	} `json:"queue,omitempty"`
	Summary string `json:"summary,omitempty"`
}

func (c *Client) CreateTicketForService(t Ticket) (tr Ticket, err error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return
	}

	u.Path = path.Join(u.Path, c.Issues)

	rh := rehttp.Client{
		ServiceName: ServiceName,
		Token:       c.Token,
		ExpectedCode: []int{
			http.StatusOK,
			http.StatusAccepted,
			http.StatusCreated,
		},
	}

	err = rh.RequestJSON("POST", u, t, &tr)
	return tr, err
}

func (c *Client) GetTicket(k string) (t Ticket, err error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return
	}

	u.Path = path.Join(u.Path, c.Issues, k)
	rh := rehttp.Client{
		ServiceName: ServiceName,
		Token:       c.Token,
		ExpectedCode: []int{
			http.StatusOK,
			http.StatusAccepted,
			http.StatusCreated,
		},
	}

	err = rh.GetJSON(u, nil, &t)
	return t, err
}

func (c *Client) UpdateTicket(k string, t Ticket) (tr Ticket, err error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return
	}

	u.Path = path.Join(u.Path, c.Issues, k)
	rh := rehttp.Client{
		ServiceName: ServiceName,
		Token:       c.Token,
		ExpectedCode: []int{
			http.StatusOK,
			http.StatusAccepted,
			http.StatusCreated,
		},
	}

	err = rh.RequestJSON("PATCH", u, t, &tr)
	return tr, err
}
