package juggler

import (
	"fmt"
	"net/http"
	"net/url"
	"path"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/rehttp"
)

const ServiceName string = "Juggler"

type Rule struct {
	CheckKwargs    struct{} `json:"check_kwargs"`
	Description    string   `json:"description"`
	MatchRawEvents bool     `json:"match_raw_events"`
	Namespace      string   `json:"namespace"`
	RuleID         string   `json:"rule_id"`
	Selector       string   `json:"selector"`
	TemplateKwargs struct {
		Login                  []string    `json:"login,omitempty"`
		Logins                 []string    `json:"logins,omitempty"`
		Method                 []string    `json:"method,omitempty"`
		Assignee               string      `json:"assignee,omitempty"`
		Followers              []string    `json:"followers,omitempty"`
		Queue                  string      `json:"queue,omitempty"`
		Components             []string    `json:"components,omitempty"`
		TimeStart              *string     `json:"time_start,omitempty"`
		TimeEnd                *string     `json:"time_end,omitempty"`
		Delay                  int         `json:"delay,omitempty"`
		Repeat                 int         `json:"repeat,omitempty"`
		CallTries              int         `json:"call_tries,omitempty"`
		OnSuccessNextCallDelay int         `json:"on_success_next_call_delay,omitempty"`
		IgnoreGaps             bool        `json:"ignore_gaps,omitempty"`
		RestartAfter           int         `json:"restart_after,omitempty"`
		DayStart               int         `json:"day_start,omitempty"`
		DayEnd                 int         `json:"day_end,omitempty"`
		Status                 interface{} `json:"status,omitempty"`
	} `json:"template_kwargs"`
	TemplateName string `json:"template_name"`
}

type jugglerAns struct {
	Rules []Rule `json:"rules"`
	Total int    `json:"total"`
}

type Client struct {
	BaseURL    string
	ChecksBase string
	NotifySet  string
	NotifyGet  string
	EventsURL  string
	Token      string
	Login      string
}

func NewClient(url, burl, surl, gurl, eurl string) *Client {
	return &Client{
		BaseURL:    url,
		ChecksBase: burl,
		NotifySet:  surl,
		NotifyGet:  gurl,
		EventsURL:  eurl,
	}
}

func (c *Client) SetOAuthToken(t string) *Client {
	c.Token = t
	return c
}

func (c *Client) SetLogin(l string) *Client {
	c.Login = l
	return c
}

func (c *Client) SetRuleOwners(r *Rule, d, ns string, ls []string) (err error) {
	if len(ls) == 0 {
		return nil
	}

	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return err
	}

	p := url.Values{}
	p.Add("do", "1")
	u.RawQuery = p.Encode()
	u.Path = path.Join(u.Path, c.NotifySet)
	switch r.TemplateName {
	default:
	case "phone_escalation":
		r.TemplateKwargs.Logins = ls
	case "startrek":
		if len(ls) > 1 {
			r.TemplateKwargs.Followers = ls[1:]
		}
		r.TemplateKwargs.Assignee = ls[0]
	case "on_status_change":
		r.TemplateKwargs.Login = ls
	}

	r.Namespace = ns
	r.Description = d

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

type ruleid struct {
	RuleID string `json:"rule_id"`
}

type filter struct {
	Filters []ruleid `json:"filters"`
}

func (c *Client) GetRuleByID(id string) (r *Rule, err error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return
	}

	p := url.Values{}
	p.Add("do", "1")
	u.RawQuery = p.Encode()
	u.Path = path.Join(u.Path, c.NotifyGet)
	f := filter{
		Filters: []ruleid{{id}},
	}

	rh := rehttp.Client{
		ServiceName:  ServiceName,
		Token:        c.Token,
		ExpectedCode: []int{http.StatusOK},
	}

	var j jugglerAns
	err = rh.RequestJSON("POST", u, f, &j)
	if err != nil {
		return
	}

	if j.Total < 1 {
		return nil, fmt.Errorf("no rules found")
	}

	return &j.Rules[0], nil
}
