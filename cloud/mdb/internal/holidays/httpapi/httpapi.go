package httpapi

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// API description https://wiki.yandex-team.ru/calendar/api/new-web/
// Most of the API methods require TVM auth, but hopefully /get-holidays we can call without it.
// https://clubs.at.yandex-team.ru/calendar/2838/2851#reply-calendar-2851

const dateFormat = "2006-01-02"

type Config struct {
	Client  httputil.ClientConfig `json:"client"`
	Country string                `yaml:"country" json:"country"`
	URL     string                `yaml:"url" json:"url"`
	Timeout encodingutil.Duration `yaml:"timeout" json:"timeout"`
}

func DefaultConfig() Config {
	return Config{
		URL: "https://api.calendar.yandex-team.ru/intapi",
		// get-holidays accept geobase codes as countries,
		// so we may use '9999` (yandex-team) as holidays region,
		// but I don't find any differences in its holidays comparing to 'rus'.
		// https://paste.yandex-team.ru/5683908
		Country: "rus",
		Timeout: encodingutil.FromDuration(time.Second * 10),
		Client: httputil.ClientConfig{
			Name:      "MDB Calendar Client",
			Transport: httputil.DefaultTransportConfig(),
		},
	}
}

type Calendar struct {
	httpClient *httputil.Client
	l          log.Logger
	cfg        Config
}

var _ holidays.Calendar = &Calendar{}

func New(cfg Config, l log.Logger) (*Calendar, error) {
	c, err := httputil.NewClient(cfg.Client, l)
	if err != nil {
		return nil, xerrors.Errorf("init http client: %w", err)
	}
	return &Calendar{httpClient: c, l: l, cfg: cfg}, nil
}

func (c *Calendar) Range(ctx context.Context, from, to time.Time) ([]holidays.Day, error) {
	url := fmt.Sprintf("%s/get-holidays?for=%s&from=%s&to=%s&outMode=all", c.cfg.URL, c.cfg.Country, from.Format(dateFormat), to.Format(dateFormat))
	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return nil, xerrors.Errorf("new request: %w", err)
	}
	rid := requestid.FromContextOrNew(ctx)
	req.Header.Add("X-Request-ID", rid)

	ctx, cancel := context.WithTimeout(ctx, c.cfg.Timeout.Duration)
	defer cancel()
	req = req.WithContext(ctx)

	resp, err := c.httpClient.Do(req, "Calendar Range")
	if err != nil {
		return nil, xerrors.Errorf("do request (request-id: %s) %w", rid, err)
	}
	defer func() { _ = resp.Body.Close() }()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return nil, xerrors.Errorf("read response body (request-id: %s): %w", rid, err)
	}
	if resp.StatusCode != http.StatusOK {
		return nil, xerrors.Errorf("request (request-id: %s) failed with code %d: %s", rid, resp.StatusCode, body)
	}
	ret, err := parseHolidaysResponse(body)
	if err != nil {
		return nil, xerrors.Errorf("parse (request-id: %s): %w", rid, err)
	}
	return ret, nil
}

type getHolidaysResponse struct {
	Holidays *[]struct {
		Date string `json:"date"`
		Type string `json:"type"`
	} `json:"holidays"`
}

func parseHolidaysResponse(body []byte) ([]holidays.Day, error) {
	var responseData getHolidaysResponse
	if err := json.Unmarshal(body, &responseData); err != nil {
		return nil, xerrors.Errorf("decode response (%s): %w", string(body), err)
	}
	if responseData.Holidays == nil {
		return nil, xerrors.Errorf("got response without $.holidays: %s", string(body))
	}
	ret := make([]holidays.Day, 0, len(*responseData.Holidays))
	for _, hd := range *responseData.Holidays {
		date, err := time.Parse(dateFormat, hd.Date)
		if err != nil {
			return nil, xerrors.Errorf("malformed day in holiday %+v: %w", hd.Date, err)
		}
		var dateType holidays.DayType
		switch hd.Type {
		case "holiday":
			dateType = holidays.Holiday
		case "weekday":
			dateType = holidays.Weekday
		case "weekend":
			dateType = holidays.Weekend
		default:
			return nil, xerrors.Errorf("unknown day type: %s. In %+v", hd.Type, hd)
		}

		ret = append(ret, holidays.Day{
			Date: date,
			Type: dateType,
		})
	}

	return ret, nil
}
