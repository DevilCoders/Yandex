package httpxiva

import "net/url"

type Option func(*Client) error

func WithHTTPHost(host string) Option {
	return func(c *Client) error {
		parsedHost, err := url.Parse(host)
		if err != nil {
			return err
		}
		c.host = parsedHost.Host
		c.httpc.SetBaseURL(host)
		return nil
	}
}

func WithTokens(subscriptionToken, sendToken string) Option {
	return func(c *Client) error {
		c.subscriptionToken = subscriptionToken
		c.sendToken = sendToken
		return nil
	}
}

func WithServiceName(name string) Option {
	return func(c *Client) error {
		c.serviceName = name
		return nil
	}
}
