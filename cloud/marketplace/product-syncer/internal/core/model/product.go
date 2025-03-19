package model

import (
	"fmt"

	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
)

type ProductYaml struct {
	ProductID   string `yaml:"product_id"`
	ProductType string `yaml:"type"`
	ProductName string `yaml:"name"`
	PublisherID string `yaml:"publisher_id"`
}

type VersionYaml struct {
	CategoryNames   []string         `yaml:"category_names"`
	ID              string           `yaml:"id"`
	LicenseRules    []interface{}    `yaml:"license_rules"`
	MarketingInfo   MarketingInfo    `yaml:"marketing_info"`
	Metadata        Metadata         `yaml:"metadata"`
	Payload         Payload          `yaml:"payload"`
	Pricing         Pricing          `yaml:"pricing"`
	ProductID       string           `yaml:"product_id"`
	PublisherID     string           `yaml:"publisher_id"`
	RelatedProducts []interface{}    `yaml:"related_products"`
	Restrictions    Restrictions     `yaml:"restrictions"`
	Source          Source           `yaml:"source"`
	State           string           `yaml:"state"`
	Tags            []string         `yaml:"tags"`
	TermsOfService  []TermsOfService `yaml:"terms_of_service"`
}

type LogoSource struct {
	Link string `yaml:"link"`
}
type Links struct {
	Title string `yaml:"title"`
	URL   string `yaml:"url"`
}
type MarketingInfoTranslation struct {
	Description      string  `yaml:"description,omitempty"`
	Links            []Links `yaml:"links,omitempty"`
	Name             string  `yaml:"name,omitempty"`
	ShortDescription string  `yaml:"short_description,omitempty"`
	Support          string  `yaml:"support,omitempty"`
	Tutorial         string  `yaml:"tutorial,omitempty"`
	UseCases         string  `yaml:"use_cases,omitempty"`
	YaSupport        string  `yaml:"ya_support,omitempty"`
	Title            string  `yaml:"title,omitempty"`
}
type Translations struct {
	En MarketingInfoTranslation `yaml:"en"`
	Ru MarketingInfoTranslation `yaml:"ru"`
}
type MarketingInfo struct {
	LogoSource   LogoSource   `yaml:"logo_source"`
	Translations Translations `yaml:"translations"`
}
type Metadata struct {
	TicketID string `yaml:"ticket_id"`
}
type Os struct {
	Family  string `yaml:"family"`
	Name    string `yaml:"name"`
	Version string `yaml:"version"`
}
type PackageContents struct {
	Name    string `yaml:"name"`
	Version string `yaml:"version"`
}
type PackageInfo struct {
	Os              Os                `yaml:"os"`
	PackageContents []PackageContents `yaml:"package_contents"`
}
type interval struct {
	Max         *int `yaml:"max,omitempty"`
	Min         *int `yaml:"min,omitempty"`
	Recommended *int `yaml:"recommended,omitempty"`
}

func (i interval) GetMin() *int {
	if i.Min == nil {
		return nil
	}
	return i.Min
}

func (i interval) GetMax() *int {
	if i.Max == nil {
		return nil
	}
	return i.Max
}

func (i interval) GetRecommended() *int {
	if i.Recommended == nil {
		return nil
	}
	return i.Recommended
}

type CPU struct {
	interval `yaml:",inline"` // hello 2014 https://github.com/go-yaml/yaml/issues/63
}
type CPUFraction struct {
	interval `yaml:",inline"`
}
type DiskSize struct {
	interval `yaml:",inline"`
}
type Memory struct {
	interval `yaml:",inline"`
}
type GPU struct {
	interval `yaml:",inline"`
}
type NetworkInterfaces struct {
	interval `yaml:",inline"`
}
type ResourceSpec struct {
	ComputePlatforms  []string           `yaml:"compute_platforms"`
	CPU               CPU                `yaml:"cpu"`
	CPUFraction       CPUFraction        `yaml:"cpu_fraction"`
	DiskSize          DiskSize           `yaml:"disk_size"`
	Memory            Memory             `yaml:"memory"`
	GPU               *GPU               `yaml:"gpu,omitempty"`
	NetworkInterfaces *NetworkInterfaces `yaml:"network_interfaces,omitempty"`
}
type ComputeImagePayload struct {
	Family         string       `yaml:"family"`
	FormID         string       `yaml:"form_id"`
	ImageID        string       `yaml:"image_id"`
	PackageInfo    PackageInfo  `yaml:"package_info"`
	ResourceSpec   ResourceSpec `yaml:"resource_spec"`
	ValidBaseImage bool         `yaml:"valid_base_image"`
}
type Payload struct {
	ComputeImage ComputeImagePayload `yaml:"compute_image"`
}

type Pricing struct {
	Skus     []interface{} `yaml:"skus"`
	TariffID string        `yaml:"tariff_id"`
	Type     string        `yaml:"type"`
}

type Restrictions struct {
}

type ComputeImageSource struct {
	BuildID       string `yaml:"build_id"`
	ImageID       string `yaml:"image_id"`
	SealedImageID string `yaml:"sealed_image_id"`
	S3Link        string `yaml:"s3_link"`
}

type Source struct {
	ComputeImage ComputeImageSource `yaml:"compute_image"`
}

type TermsOfService struct {
	Translations Translations `yaml:"translations"`
	Type         string       `yaml:"type"`
	URL          string       `yaml:"url"`
}

func (v VersionYaml) Validate() error {
	if v.MarketingInfo.LogoSource.Link == "" {
		return fmt.Errorf("version.marketing_info.logo_source.link is missing")
	}
	return nil
}

func MarketingInfoYamlToRequest(m MarketingInfo) marketplace.MarketingInfo {
	req := marketplace.MarketingInfo{
		UseCases: marketplace.LocalizedString{
			En: m.Translations.En.UseCases,
			Ru: m.Translations.Ru.UseCases,
		},
		Name: marketplace.LocalizedString{
			En: m.Translations.En.Name,
			Ru: m.Translations.Ru.Name,
		},
		Description: marketplace.LocalizedString{
			En: m.Translations.En.Description,
			Ru: m.Translations.Ru.Description,
		},
		Support: marketplace.LocalizedString{
			En: m.Translations.En.Support,
			Ru: m.Translations.Ru.Support,
		},
		ShortDescription: marketplace.LocalizedString{
			En: m.Translations.En.ShortDescription,
			Ru: m.Translations.Ru.ShortDescription,
		},
		Links: marketplace.LocalizedLinks{
			En: marketplace.Links{},
			Ru: marketplace.Links{},
		},
		Tutorial: marketplace.LocalizedString{
			En: m.Translations.En.Tutorial,
			Ru: m.Translations.Ru.Tutorial,
		},
	}
	for _, link := range m.Translations.En.Links {
		req.Links.En.Links = append(req.Links.En.Links, marketplace.Link{
			Title: link.Title,
			URL:   link.URL,
		})
	}
	for _, link := range m.Translations.Ru.Links {
		req.Links.Ru.Links = append(req.Links.Ru.Links, marketplace.Link{
			Title: link.Title,
			URL:   link.URL,
		})
	}

	return req
}

func TermsOfServiceYamlToRequest(version VersionYaml) []marketplace.TermsOfService {
	var res []marketplace.TermsOfService
	for _, t := range version.TermsOfService {
		tos := marketplace.TermsOfService{
			Title: marketplace.LocalizedString{
				En: t.Translations.En.Title,
				Ru: t.Translations.Ru.Title,
			},
			Type: t.Type,
			URL:  t.URL,
		}
		res = append(res, tos)
	}
	return res
}

func PackageInfoYamlToRequest(contents []PackageContents) []marketplace.PackageContents {
	var res []marketplace.PackageContents
	for _, item := range contents {
		res = append(res, marketplace.PackageContents{
			Version: item.Version,
			Name:    item.Name,
		})
	}
	return res
}
