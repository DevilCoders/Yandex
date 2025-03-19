package domain

import (
	"fmt"
	"time"
)

type LinkItem struct {
	URL, Label string
}

type Profile struct {
	Service, ProfileType string
	ResourceID           string `yson:"resource_id"`
	Labels               map[string]string
	TS                   time.Time
	TSNano               int64
	Links                []LinkItem
	Data                 string `json:"-"`
}

func (p *Profile) FillLinks(base string) {
	itemURL := fmt.Sprintf("%v/service/%v/type/%v/resource/%v/profile/%v", base, p.Service, p.ProfileType, p.ResourceID, p.TSNano)
	itemPprofResBase := fmt.Sprintf("%v/service/%v/type/%v/resource/%v/pprof_result/%v/", base, p.Service, p.ProfileType, p.ResourceID, p.TSNano)
	p.Links = []LinkItem{
		{URL: itemURL, Label: "raw"},
		{URL: itemPprofResBase + "png", Label: "png"},
		{URL: itemPprofResBase + "tree", Label: "tree"},
		{URL: itemPprofResBase + "text", Label: "text"},
	}
	if p.ProfileType == "cpu" {
		p.Links = append(p.Links, LinkItem{URL: fmt.Sprintf("/build/speedscope/#profileURL=%v", itemURL), Label: "flamegraph"})
	}
}

type ServicesList struct {
	Services []ServiceItem
}

type ServiceItem struct {
	Name      string
	Resources []string
}

type ProfileResourceGroup struct {
	Service, ProfileType string
	ResourceID           string `yson:"resource_id"`
	Count                int
	MinTS                int64
	MaxTS                int64
	Links                []string
}

func (p *ProfileResourceGroup) FillLinks(baseURL string) {
	minURL := fmt.Sprintf("%v/service/%v/type/%v/resource/%v/profile/%v",
		baseURL,
		p.Service,
		p.ProfileType,
		p.ResourceID,
		p.MinTS,
	)
	maxURL := fmt.Sprintf("%v/service/%v/type/%v/resource/%v/profile/%v",
		baseURL,
		p.Service,
		p.ProfileType,
		p.ResourceID,
		p.MaxTS,
	)
	p.Links = []string{minURL, maxURL}
}
