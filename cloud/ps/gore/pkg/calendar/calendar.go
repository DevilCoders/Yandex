package calendar

import (
	"net/http"
	"net/url"
	"path"
	"strconv"
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/rehttp"
)

const ServiceName string = "calendar"

type Event struct {
	EventID     int64  `json:"id"`
	Start       string `json:"startTs"`
	End         string `json:"endTs"`
	Name        string `json:"name"`
	Description string `json:"description"`
}

type Client struct {
	BaseURL        string
	CreateEventURL string
	DeleteEventURL string
	GetEventsURL   string
	GetEventURL    string
	ShareEventURL  string
}

func NewClient(url, ceurl, deurl, geurl, geeurl, seurl string) (c *Client) {
	return &Client{
		BaseURL:        url,
		CreateEventURL: ceurl,
		DeleteEventURL: deurl,
		GetEventsURL:   geurl,
		GetEventURL:    geeurl,
		ShareEventURL:  seurl,
	}
}

func (c *Client) CreateEvent(uid, layerID, name, description string, dateStart, dateEnd int64, tz string) error {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return err
	}

	loc, err := time.LoadLocation(tz)
	if err != nil {
		return err
	}

	r := map[string]interface{}{
		"type":     "user",
		"startTs":  time.Unix(dateStart, 0).In(loc).Format("2006-01-02T15:04:05"),
		"endTs":    time.Unix(dateEnd, 0).In(loc).Format("2006-01-02T15:04:05"),
		"name":     name,
		"layerId":  layerID,
		"isAllDay": true,
	}

	p := url.Values{}
	p.Add("uid", uid)
	p.Add("tz", tz)
	u.RawQuery = p.Encode()
	u.Path = path.Join(u.Path, c.CreateEventURL)

	rh := rehttp.Client{
		ServiceName: ServiceName,
		ExpectedCode: []int{
			http.StatusOK,
			http.StatusCreated,
			http.StatusAccepted,
		},
	}

	_, err = rh.GetText(u, r)
	return err
}

func (c *Client) GetEventsInRange(uid, layerID, tz string, dateStart, dateEnd int64) ([]Event, error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return nil, err
	}

	loc, err := time.LoadLocation(tz)
	if err != nil {
		return nil, err
	}

	p := url.Values{}
	p.Add("uid", uid)
	if loc != nil {
		p.Add("tz", loc.String())
	}
	p.Add("from", time.Unix(dateStart, 0).In(loc).Format("2006-01-02T15:04:05"))
	p.Add("to", time.Unix(dateEnd, 0).In(loc).Format("2006-01-02T15:04:05"))
	p.Add("layerId", layerID)

	u.RawQuery = p.Encode()
	u.Path = path.Join(u.Path, c.GetEventsURL)

	rh := rehttp.Client{
		ServiceName:  ServiceName,
		ExpectedCode: []int{http.StatusOK},
	}

	cr := struct {
		LastUpdate int64   `json:"lastUpdateTs"`
		Events     []Event `json:"events"`
	}{}

	err = rh.GetJSON(u, nil, &cr)
	if err != nil {
		return nil, err
	}

	return cr.Events, err
}

func (c *Client) GetEvent(id int64, uid, layerID, tz string) (e Event, err error) {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return e, err
	}

	p := url.Values{}
	p.Add("uid", uid)
	p.Add("tz", tz)
	p.Add("eventId", strconv.FormatInt(id, 10))

	u.RawQuery = p.Encode()
	u.Path = path.Join(u.Path, c.GetEventURL)
	rh := rehttp.Client{
		ServiceName: ServiceName,
		ExpectedCode: []int{
			http.StatusOK,
		},
	}

	err = rh.GetJSON(u, nil, &e)

	return e, err
}

func (c *Client) DeleteEvent(id, uid, layerID, tz string) error {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return err
	}

	p := url.Values{}
	p.Add("uid", uid)
	p.Add("tz", tz)
	p.Add("eventId", id)

	u.RawQuery = p.Encode()
	u.Path = path.Join(u.Path, c.DeleteEventURL)
	rh := rehttp.Client{
		ServiceName: ServiceName,
		ExpectedCode: []int{
			http.StatusOK,
			http.StatusAccepted,
		},
	}

	_, err = rh.GetText(u, nil)
	return err
}

func (c *Client) AttachEvent(uid, layerID, availability, id string) error {
	u, err := url.ParseRequestURI(c.BaseURL)
	if err != nil {
		return err
	}

	r := map[string]interface{}{
		"availability": availability,
		"layerId":      layerID,
	}

	p := url.Values{}
	p.Add("uid", uid)
	p.Add("id", id)

	u.RawQuery = p.Encode()
	u.Path = path.Join(u.Path, c.GetEventsURL)

	rh := rehttp.Client{
		ServiceName: ServiceName,
		ExpectedCode: []int{
			http.StatusOK,
			http.StatusAccepted,
		},
	}

	_, err = rh.GetText(u, r)
	return err
}
