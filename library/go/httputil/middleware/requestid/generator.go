package requestid

import (
	"fmt"
	"os"
	"strings"
	"sync/atomic"
	"time"

	"github.com/gofrs/uuid"
)

// GeneratorFunc can be used to replace/mutate original request ID
type GeneratorFunc = func(reqID string) string

// NopGenerator returns unmodified request ID
func NopGenerator(reqID string) string {
	return reqID
}

// RandomOnEmptyGenerator returns original request ID if it is not empty and
// will generate new random string otherwise
func RandomOnEmptyGenerator(reqID string) string {
	if reqID != "" {
		return reqID
	}
	return uuid.Must(uuid.NewV4()).String()
}

// NewAppHostGenerator returns GeneratorFunc for request ID as a string of the form "time-randomnumberpid-hostname".
// @see https://a.yandex-team.ru/arc/trunk/arcadia/apphost/lib/http_adapter/request/request_system.cpp?rev=6773102#L123
func NewAppHostGenerator() GeneratorFunc {
	var reqid uint64

	var suffixes = []string{
		".yandex.ru",
		".search.yandex.net",
		".sas.yp-c.yandex.net", // TODO: SEARCH-10023. For YP reqids. Maybe delete this
		".man.yp-c.yandex.net",
		".vla.yp-c.yandex.net",
		".haze.yandex.net",
		".gencfg-c.yandex.net",
	}

	pid := os.Getpid()

	hostname, err := os.Hostname()
	if err != nil {
		hostname = "localhost"
	}

	for _, suffix := range suffixes {
		if strings.HasSuffix(hostname, suffix) {
			hostname = strings.TrimSuffix(hostname, suffix)
			break
		}
	}

	return func(reqID string) string {
		if reqID != "" && strings.Count(reqID, "-") < 2 {
			return reqID + "-" + hostname
		}

		timePart := time.Now().UnixNano() / 1000
		randomPart := atomic.AddUint64(&reqid, 1)
		return fmt.Sprintf("%d-%d%05d-%s", timePart, randomPart, pid, hostname)
	}
}
