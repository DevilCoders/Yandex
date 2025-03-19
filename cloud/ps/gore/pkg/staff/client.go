package staff

type Client struct {
	BaseURL     string
	Persons     string
	Token       string
	Transitions string
	Queues      string
	Components  string
}

func NewClient(url, issues, transitions, queues, components string) *Client {
	return &Client{
		BaseURL:     url,
		Persons:     issues,
		Transitions: transitions,
		Queues:      queues,
		Components:  components,
	}
}

func (c *Client) SetOAuthToken(t string) *Client {
	c.Token = t
	return c
}
