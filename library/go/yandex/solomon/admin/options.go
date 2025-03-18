package admin

const (
	// OAuth app for command line applications. Feel free to use this constants in your tool.
	SolomonCLIID     = "5a24478594944d48803a1ffdfc21ac4a"
	SolomonCLISecret = "91a61123f50a44d0a66b119e5b8a03b1"

	ProductionURL = "http://solomon.yandex.net/api/v2"
	PrestableURL  = "http://solomon-prestable.yandex.net/api/v2"
	TestingURL    = "http://solomon-test.yandex.net/api/v2"
)

type Option func(*Client)

func WithURL(url string) Option {
	return func(c *Client) {
		c.solomonURL = url
	}
}

func WithDebugLog() Option {
	return func(c *Client) {
		c.c.SetDebug(true)
	}
}
