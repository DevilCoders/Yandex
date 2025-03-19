package startrek

var ServiceName string = "StarTrek"

type Client struct {
	BaseURL       string
	Issues        string
	DefaultValues string
	Token         string
	Transitions   string
	Queues        string
	Components    string
	Login         string
}

func NewClient(url, issues, transitions, queues, components string) *Client {
	return &Client{
		BaseURL:     url,
		Issues:      issues,
		Transitions: transitions,
		Queues:      queues,
		Components:  components,
	}
}

func (c *Client) SetOAuthToken(t string) *Client {
	c.Token = t
	return c
}

func (c *Client) SetLogin(l string) *Client {
	c.Login = l
	return c
}
