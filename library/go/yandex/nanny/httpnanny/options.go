package httpnanny

type Option func(*Client)

func WithToken(token string) Option {
	return func(c *Client) {
		c.token = token
	}
}

func WithDev() Option {
	return func(c *Client) {
		c.apiEndpoint = devAPIEndpoint
	}
}

func WithDebug() Option {
	return func(c *Client) {
		c.r.Debug = true
	}
}
