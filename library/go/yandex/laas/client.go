package laas

import (
	"context"
	"net"
	"strconv"
	"time"
)

// Client is an abstract interface for LaaS client
type Client interface {
	DetectRegion(context.Context, Params) (*RegionResponse, error)
}

type Params struct {
	UserIP    net.IP
	YandexUID uint64
	YandexGID int
	YP        string
	URLPrefix string
}

type RegionResponse struct {
	LocationUnixtime           Timestamp        `json:"location_unixtime,omitempty"`
	SuspectedLocationUnixtime  Timestamp        `json:"suspected_location_unixtime,omitempty"`
	ProbableRegions            []ProbableRegion `json:"probable_regions,omitempty"`
	RegionID                   int              `json:"region_id,omitempty"`
	Precision                  int              `json:"precision,omitempty"`
	Latitude                   float64          `json:"latitude,omitempty"`
	Longitude                  float64          `json:"longitude,omitempty"`
	SuspectedRegionID          int              `json:"suspected_region_id,omitempty"`
	CityID                     int              `json:"city_id,omitempty"`
	RegionByIP                 int              `json:"region_by_ip,omitempty"`
	SuspectedRegionCity        int              `json:"suspected_region_city,omitempty"`
	LocationAccuracy           int              `json:"location_accuracy,omitempty"`
	SuspectedLatitude          float64          `json:"suspected_latitude,omitempty"`
	SuspectedLongitude         float64          `json:"suspected_longitude,omitempty"`
	SuspectedLocationAccuracy  int              `json:"suspected_location_accuracy,omitempty"`
	SuspectedPrecision         int              `json:"suspected_precision,omitempty"`
	CountryIDByIP              int              `json:"country_id_by_ip,omitempty"`
	ProbableRegionsReliability float32          `json:"probable_regions_reliability,omitempty"`
	ShouldUpdateCookie         bool             `json:"should_update_cookie,omitempty"`
	IsUserChoice               bool             `json:"is_user_choice,omitempty"`
	IsAnonymousVpn             bool             `json:"is_anonymous_vpn,omitempty"`
	IsPublicProxy              bool             `json:"is_public_proxy,omitempty"`
	IsSerpTrustedNet           bool             `json:"is_serp_trusted_net,omitempty"`
	IsTor                      bool             `json:"is_tor,omitempty"`
	IsHosting                  bool             `json:"is_hosting,omitempty"`
	IsGdpr                     bool             `json:"is_gdpr,omitempty"`
	IsMobile                   bool             `json:"is_mobile,omitempty"`
	IsYandexNet                bool             `json:"is_yandex_net,omitempty"`
	IsYandexStaff              bool             `json:"is_yandex_staff,omitempty"`
}

type ProbableRegion struct {
	RegionID int     `json:"region_id,omitempty"`
	Weight   float64 `json:"weight,omitempty"`
}

type Timestamp struct {
	time.Time
}

func (t *Timestamp) UnmarshalJSON(b []byte) error {
	ts, err := strconv.ParseInt(string(b), 10, 0)
	if err != nil {
		return err
	}
	t.Time = time.Unix(ts, 0)
	return nil
}
