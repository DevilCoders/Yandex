package federation

import (
	"bufio"
	"context"
	"fmt"
	"io"
	"net/url"
	"path"

	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/browser"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/server"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const beforeBrowserOpenNote = `
You are going to be authenticated via federation-id '%s'.
Your federation authentication web site will be opened.
After your successful authentication, you will be redirected to '%s'.
Press Enter

`

func federationURL(federationID, federationEndpoint, redirectURL string) (string, error) {
	baseURL, err := url.Parse(federationEndpoint)
	if err != nil {
		return "", err
	}

	baseURL.Path = path.Join(baseURL.Path, federationID)

	params := url.Values{}
	params.Add("redirectUrl", redirectURL)

	baseURL.RawQuery = params.Encode() // Escape Query Parameters

	return baseURL.String(), nil
}

func (ft *federationToken) urlRequest(serverAddr string) error {
	requestURL, err := federationURL(
		ft.cfg.DeploymentConfig().Federation.ID,
		ft.cfg.DeploymentConfig().Federation.Endpoint,
		fmt.Sprintf("http://%s", serverAddr))
	if err != nil {
		return err
	}

	if err := browser.OpenURL(requestURL); err != nil {
		return err
	}

	return nil
}

func (ft *federationToken) consoleURL() (string, error) {
	endpointURL, err := url.Parse(ft.cfg.DeploymentConfig().Federation.Endpoint)
	if err != nil {
		return "", err
	}
	endpointURL.Path = ""
	return endpointURL.String(), nil
}

func NewTokenGetter(cfg *config.Config, configPath string, in io.Reader, l log.Logger) grpc.TokenFunc {
	ft := federationToken{
		cfg:        cfg,
		configPath: configPath,
		r:          bufio.NewReader(in),
		l:          l,
	}
	return ft.IAMToken
}

type federationToken struct {
	cfg        *config.Config
	configPath string
	r          *bufio.Reader
	l          log.Logger
}

func (ft *federationToken) IAMToken(ctx context.Context) (iam.Token, error) {
	oldToken := ft.cfg.DeploymentConfig().Token
	err := oldToken.Validate()
	if err == nil {
		return iam.Token{
			Value:     oldToken.IAMToken,
			ExpiresAt: oldToken.ExpiresAt,
		}, nil
	}

	consoleURL, err := ft.consoleURL()
	if err != nil {
		return iam.Token{}, err
	}
	ft.l.Infof(beforeBrowserOpenNote, ft.cfg.DeploymentConfig().Federation.ID, consoleURL)
	_, _, err = ft.r.ReadLine()
	if err != nil {
		return iam.Token{}, xerrors.New("read answer error")
	}
	token, err := server.GetToken(ctx, ft.urlRequest, consoleURL, ft.l)
	if err != nil {
		return iam.Token{}, err
	}

	ft.l.Infof("Save token to deployment %s", ft.cfg.SelectedDeployment)
	deploymentCfg := ft.cfg.DeploymentConfig()
	deploymentCfg.Token = config.DeploymentToken{
		IAMToken:  token.IamToken,
		ExpiresAt: token.ExpiresAt.AsTime(),
	}
	ft.cfg.Deployments[ft.cfg.DeploymentName()] = deploymentCfg

	ft.l.Infof("Write updated config to %s", ft.configPath)
	if err = config.WriteConfig(*ft.cfg, ft.configPath); err != nil {
		return iam.Token{}, err
	}

	ft.l.Infof("Federation successfully finished, token will expire at %s", token.ExpiresAt.AsTime())
	iamToken := iam.Token{
		Value:     token.IamToken,
		ExpiresAt: token.ExpiresAt.AsTime(),
	}

	return iamToken, nil
}
