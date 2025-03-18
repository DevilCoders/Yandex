// Package uatraits provides native implementation of C++ uatraits library which uses XML files with traits definitions.
package uatraits

import (
	"io/ioutil"
	"net/http"
	"strings"

	lru "github.com/hashicorp/golang-lru"

	"a.yandex-team.ru/library/go/core/resource"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	defaultCacheSize = 10000
)

type xmlLocationType int

const (
	undefinedLocation = xmlLocationType(iota)
	externalFileLocation
	resourceKeyLocation
)

type xmlLocation struct {
	Path     string
	Location xmlLocationType
}

func newXMLLocation() xmlLocation {
	return xmlLocation{
		Path:     "",
		Location: undefinedLocation,
	}
}

type detectorParams struct {
	// If some path is set to empty string, default embedded XML from resources will be loaded.

	// Path to the main XML file with traits declarations.
	browserRulesFilePath xmlLocation
	// Path to the file with device specifications
	extraRulesFilePath xmlLocation
	// Path to the file with additional browser features and its definition rules.
	profilesFilePath xmlLocation
	// LRU cache size. if zero is passed, the default value (10000) is applied
	cacheSize int
}

// By default a detector will be constructed using built-it resources
func newDefaultDetectorParams() detectorParams {
	return detectorParams{
		browserRulesFilePath: xmlLocation{
			Path:     "browser.xml",
			Location: resourceKeyLocation,
		},
		extraRulesFilePath: xmlLocation{
			Path:     "extra.xml",
			Location: resourceKeyLocation,
		},
		profilesFilePath: xmlLocation{
			Path:     "profiles.xml",
			Location: resourceKeyLocation,
		},
		cacheSize: defaultCacheSize,
	}
}

type Detector struct {
	detectorParams

	browserRules *browserRules    // from browser.xml
	extraRules   *extraRules      // from extra.xml
	profileRules *profilesStorage // from profiles.xml

	cache           *lru.Cache
	cacheHitMetrics CacheHitMetrics
}

type CacheHitMetrics interface {
	HitIncrement()
	MissIncrement()
}

// newDefaultCacheHitMetrics by default cache metrics disabled
func newDefaultCacheHitMetrics() *cacheHitMetricsStub {
	return &cacheHitMetricsStub{}
}

// cacheHitMetricsStub stub for disabling cache hit counter
type cacheHitMetricsStub struct{}

func (c *cacheHitMetricsStub) HitIncrement() {}

func (c *cacheHitMetricsStub) MissIncrement() {}

/*
Functional options.
*/

// Browsers is the functional option loads browsers rules from external XML file.
func Browsers(xmlFilePath string) func(*Detector) {
	return func(d *Detector) {
		d.browserRulesFilePath = xmlLocation{
			Path:     xmlFilePath,
			Location: externalFileLocation,
		}
	}
}

// Profiles is the functional option loads profiles info from external XML file.
func Profiles(xmlFilePath string) func(*Detector) {
	return func(d *Detector) {
		d.profilesFilePath = xmlLocation{
			Path:     xmlFilePath,
			Location: externalFileLocation,
		}
	}
}

// Extras is the functional option loads extra definitions info from external XML file.
func Extras(xmlFilePath string) func(*Detector) {
	return func(d *Detector) {
		d.extraRulesFilePath = xmlLocation{
			Path:     xmlFilePath,
			Location: externalFileLocation,
		}
	}
}

// CacheSize sets custom cache size measured in items.
func CacheSize(size int) func(*Detector) {
	return func(d *Detector) {
		d.cacheSize = size
	}
}

// EnableMetrics enables cache hit counting.
func EnableMetrics(metrics CacheHitMetrics) func(*Detector) {
	return func(d *Detector) {
		d.cacheHitMetrics = metrics
	}
}

func loadRulesBytes(location xmlLocation) ([]byte, error) {
	switch location.Location {
	case externalFileLocation:
		if bytes, err := ioutil.ReadFile(location.Path); err != nil {
			return nil, xerrors.Errorf("cannot load bytes for rules from file: %w", err)
		} else {
			return bytes, nil
		}
	case resourceKeyLocation:
		if bytes := resource.Get(location.Path); bytes == nil {
			return nil, xerrors.Errorf("cannot load bytes from resource %s", location.Path)
		} else {
			return bytes, nil
		}
	default:
		return nil, xerrors.Errorf("invalid xml location type value: %v", location.Location)
	}
}

func NewDetector(options ...func(*Detector)) (*Detector, error) {
	detector := &Detector{
		detectorParams:  newDefaultDetectorParams(),
		cacheHitMetrics: newDefaultCacheHitMetrics(),
	}

	for _, option := range options {
		option(detector)
	}

	// Load browser rules.
	browserBytes, err := loadRulesBytes(detector.browserRulesFilePath)
	if err != nil {
		return nil, err
	}
	if detector.browserRules, err = parseBrowserRulesXMLBytes(browserBytes); err != nil {
		return nil, err
	}

	// Load extra rules.
	extraRulesBytes, err := loadRulesBytes(detector.extraRulesFilePath)
	if err != nil {
		return nil, err
	}
	if detector.extraRules, err = parseExtraRulesXMLBytes(extraRulesBytes); err != nil {
		return nil, err
	}

	// Load profiles.
	profilesBytes, err := loadRulesBytes(detector.profilesFilePath)
	if err != nil {
		return nil, err
	}
	if detector.profileRules, err = parseProfileRulesXMLBytes(profilesBytes); err != nil {
		return nil, err
	}

	// Create cache.
	if detector.cache, err = lru.New(detector.cacheSize); err != nil {
		return nil, err
	}

	return detector, nil
}

func (detector *Detector) doDetect(userAgent string) Traits {
	traits := newTraits()
	detector.browserRules.trigger(userAgent, traits)
	if detector.extraRules != nil {
		detector.extraRules.trigger(traits)
	}
	return traits
}

func (detector *Detector) Detect(userAgent string) Traits {
	if cachedTraits, ok := detector.cache.Get(userAgent); ok {
		detector.cacheHitMetrics.HitIncrement()
		return cachedTraits.(Traits)
	}
	detector.cacheHitMetrics.MissIncrement()
	traits := detector.doDetect(userAgent)
	detector.cache.Add(userAgent, traits)
	return traits
}

const xWapProfileHeader = "X-Wap-Profile"

var (
	// Found in C++ implementaion.
	additionalHeaders = []string{
		"Device-Stock-UA",
		"X-OperaMini-Phone-UA",
	}

	fieldsForAdditionalHeaders = []string{
		"isTablet",
		"OSFamily",
		"OSVersion",
		"isMobile",
		"isTouch",
		"isTV",
		"DeviceVendor",
		"DeviceModel",
		"DeviceName",
	}
)

// DetectByHeaders detects traits using all possible headers including additional third-party values
// like Device-Stock-UA, X-OperaMini-Phone-UA and X-Wap-Profile
// User-Agent, naturally, must be presented.
func (detector *Detector) DetectByHeaders(httpHeaders http.Header) Traits {
	userAgentValue := httpHeaders.Get("User-Agent")
	if userAgentValue == "" {
		return nil
	}

	traits := detector.Detect(userAgentValue).Copy()

	for _, additionalHeader := range additionalHeaders {
		headerValue := httpHeaders.Get(additionalHeader)
		if headerValue == "" {
			continue
		}

		additionalTraits := detector.Detect(headerValue)
		for _, field := range fieldsForAdditionalHeaders {
			traitValue := additionalTraits[field]
			if traitValue == "" {
				continue
			}
			traits[field] = traitValue
		}
	}

	// Try to get hardware specifications using X-Wap-Profile.
	if detector.profileRules != nil {
		if wapProfileValue := httpHeaders.Get(xWapProfileHeader); wapProfileValue != "" {
			value := strings.Trim(wapProfileValue, " \x22")
			detector.profileRules.Trigger(value, traits)
		}
	}

	return traits
}
