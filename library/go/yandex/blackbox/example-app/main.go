package main

import (
	"net/http"

	"github.com/labstack/echo/v4"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
	"a.yandex-team.ru/library/go/yandex/tvm/tvmtool"
)

func newSessionHandler(bb blackbox.Client) echo.HandlerFunc {
	return func(c echo.Context) error {
		sessID, err := c.Cookie("Session_id")
		if err != nil {
			return xerrors.Errorf("failed to get Session_id cookie: %w", err)
		}

		bbResponse, err := bb.SessionID(c.Request().Context(), blackbox.SessionIDRequest{
			SessionID: sessID.Value,
			UserIP:    c.RealIP(),
			Host:      c.Request().Host,
		})

		if err != nil {
			if blackbox.IsUnauthorized(err) {
				// TODO: redirect to proper passport login page
				return c.Redirect(http.StatusFound, "https://passport.yandex-team.ru/auth?retpath=<my_path>")
			}

			err = xerrors.Errorf("failed to check bbResponse session: %w", err)
			return err
		}

		// optional redirect to passport for session resign
		// Documentation: https://doc.yandex-team.ru/Passport/AuthDevGuide/concepts/authorization-policy-resign.html
		//if bbResponse.NeedResign {
		//return c.Redirect(http.StatusFound, "https://passport.yandex-team.ru/auth/update/?retpath=<my_path>")
		//}

		return c.JSON(http.StatusOK, echo.Map{
			"uid":   bbResponse.User.ID,
			"login": bbResponse.User.Login,
		})
	}
}

func newOAuthHandler(bb blackbox.Client) echo.HandlerFunc {
	return func(c echo.Context) error {
		token := c.Request().Header.Get("Authorization")
		if len(token) <= 6 || token[:6] != "OAuth " {
			return xerrors.New("failed to get OAuth token from Authorization header")
		}

		bbResponse, err := bb.OAuth(c.Request().Context(), blackbox.OAuthRequest{
			OAuthToken: token[6:],
			UserIP:     c.RealIP(),
		})

		if err != nil {
			if blackbox.IsUnauthorized(err) {
				// TODO: redirect to proper oauth authorize page
				return c.Redirect(http.StatusFound, "https://passport.yandex-team.ru/auth?retpath=<my_path>")
			}

			err = xerrors.Errorf("failed to check oauth token: %w", err)
			return err
		}

		return c.JSON(http.StatusOK, echo.Map{
			"uid":       bbResponse.User.ID,
			"login":     bbResponse.User.Login,
			"client_id": bbResponse.ClientID,
			"scopes":    bbResponse.Scopes,
		})
	}
}

func main() {
	// Use YandexTeam BlackBox w/ TVM in Y.Deploy
	tvmClient, err := tvmtool.NewDeployClient()
	if err != nil {
		panic(err)
	}

	logger, err := zap.New(zap.ConsoleConfig(log.DebugLevel))
	if err != nil {
		panic(err)
	}

	bb, err := httpbb.NewIntranet(
		httpbb.WithLogger(logger),
		httpbb.WithTVM(tvmClient),
	)
	if err != nil {
		panic(err)
	}

	e := echo.New()
	// Please, don't use debug in production ;)
	e.Debug = true

	e.GET("/", func(c echo.Context) error {
		return c.Redirect(http.StatusFound, "/session")
	})

	e.GET("/ping", func(c echo.Context) error {
		if _, err := tvmClient.GetStatus(c.Request().Context()); err != nil {
			logger.Error("tvmtool ping failed", log.Error(err))
			return err
		}

		return c.String(http.StatusOK, "ok")
	})

	// Example of session check
	e.GET("/session", newSessionHandler(bb))

	// Example of oauth token check
	e.GET("/oauth", newOAuthHandler(bb))

	if err := e.Start(":3000"); err != nil {
		panic(err)
	}
}
