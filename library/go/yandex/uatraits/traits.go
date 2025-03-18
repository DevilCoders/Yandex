package uatraits

type Traits map[string]string

func newTraits() Traits {
	return make(Traits)
}

func (t Traits) Get(field string) string {
	return t[field]
}

func (t Traits) Set(field, value string) {
	t[field] = value
}

func (t Traits) HasField(field string) bool {
	_, ok := t[field]
	return ok
}

// A bunch of helper functions.
func (t Traits) isTrueKey(key string) bool {
	return t[key] == "true"
}

func (t Traits) IsMobile() bool {
	return t.isTrueKey("isMobile")
}

func (t Traits) IsTouch() bool {
	return t.isTrueKey("isTouch")
}

func (t Traits) IsMultiTouch() bool {
	return t.isTrueKey("MultiTouch")
}

func (t Traits) IsTablet() bool {
	return t.isTrueKey("isTablet")
}

func (t Traits) IsBrowser() bool {
	return t.isTrueKey("isBrowser")
}

func (t Traits) IsJ2ME() bool {
	return t.isTrueKey("J2ME")
}

func (t Traits) IsX64() bool {
	return t.isTrueKey("x64")
}

func (t Traits) IsInAppBrowser() bool {
	return t.isTrueKey("inAppBrowser")
}

func (t Traits) IsITP() bool {
	return t.isTrueKey("ITP")
}

func (t Traits) IsRobot() bool {
	return t.isTrueKey("isRobot")
}

func (t Traits) CSP1Support() bool {
	return t.isTrueKey("CSP1Support")
}

func (t Traits) CSP2Support() bool {
	return t.isTrueKey("CSP2Support")
}

func (t Traits) BrowserEngine() string {
	return t["BrowserEngine"]
}

func (t Traits) BrowserEngineVersion() string {
	return t["BrowserEngineVersion"]
}

func (t Traits) BrowserBase() string {
	return t["BrowserBase"]
}

func (t Traits) BrowserBaseVersion() string {
	return t["BrowserBaseVersion"]
}

func (t Traits) BrowserName() string {
	return t["BrowserName"]
}

func (t Traits) BrowserVersion() string {
	return t["BrowserVersion"]
}

func (t Traits) OSFamily() string {
	return t["OSFamily"]
}

func (t Traits) OSName() string {
	return t["OSName"]
}

func (t Traits) OSVersion() string {
	return t["OSVersion"]
}

func (t Traits) DeviceVendor() string {
	return t["DeviceVendor"]
}

func (t Traits) DeviceName() string {
	return t["DeviceVendor"]
}

func (t Traits) DeviceModel() string {
	return t["DeviceVendor"]
}

func (t Traits) Copy() Traits {
	if t == nil {
		return nil
	}

	result := make(Traits, len(t))
	for key, value := range t {
		result[key] = value
	}

	return result
}
