package bitbucket

import (
	"net/http"
	"net/url"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/auth"
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/rehttp"
)

const ServiceName string = "bitbucket"

type BitbucketResponse struct {
	Values []string `json:"values"`
}

type BitbucketClient struct {
	URL   string
	Token string
}

func NewClient(url, token string) *BitbucketClient {
	return &BitbucketClient{
		URL:   url,
		Token: token,
	}
}

func (c *BitbucketClient) GetCSVFile() (string, error) {
	u, err := url.ParseRequestURI(c.URL)
	if err != nil {
		return "", err
	}

	rh := rehttp.Client{
		ServiceName:  ServiceName,
		Token:        c.Token,
		AuthType:     auth.BasicAuth,
		ExpectedCode: []int{http.StatusOK},
	}

	return rh.GetText(u, nil)
}
