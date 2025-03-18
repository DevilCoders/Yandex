package consoleapi

import (
	"fmt"

	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
)

const (
	Error         = "ERROR"
	Timeout       = "TIMEOUT"
	InvalidHeader = "INVALID_HEADER"
	Updating      = "UPDATING"
	HTTP500       = "HTTP_500"
	HTTP502       = "HTTP_502"
	HTTP503       = "HTTP_503"
	HTTP504       = "HTTP_504"
	HTTP403       = "HTTP_403"
	HTTP404       = "HTTP_404"
	HTTP429       = "HTTP_429"
)

func makeServeStaleError(str string) (model.ServeStaleErrorType, error) {
	switch str {
	case Error:
		return model.ServeStaleError, nil
	case Timeout:
		return model.ServeStaleTimeout, nil
	case InvalidHeader:
		return model.ServeStaleInvalidHeader, nil
	case Updating:
		return model.ServeStaleUpdating, nil
	case HTTP500:
		return model.ServeStaleHTTP500, nil
	case HTTP502:
		return model.ServeStaleHTTP502, nil
	case HTTP503:
		return model.ServeStaleHTTP503, nil
	case HTTP504:
		return model.ServeStaleHTTP504, nil
	case HTTP403:
		return model.ServeStaleHTTP403, nil
	case HTTP404:
		return model.ServeStaleHTTP404, nil
	case HTTP429:
		return model.ServeStaleHTTP429, nil
	default:
		return 0, fmt.Errorf("invalid serveStaleError value: %s", str)
	}
}
