package prometheus

import (
	"bytes"
	"fmt"
	"strings"
	"sync"

	dto "github.com/prometheus/client_model/go"
)

var bufPool sync.Pool

func getBuf() *bytes.Buffer {
	buf := bufPool.Get()
	if buf == nil {
		return &bytes.Buffer{}
	}

	return buf.(*bytes.Buffer)
}

func putBuf(buf *bytes.Buffer) {
	buf.Reset()
	bufPool.Put(buf)
}

// LabelPairsToText formats labels to a string
func LabelPairsToText(
	labels []*dto.LabelPair,
	additionalLabelName, additionalLabelValue string,
) (string, error) {
	buf := getBuf()
	defer putBuf(buf)
	if len(labels) == 0 && additionalLabelName == "" {
		return buf.String(), nil
	}

	separator := '_'
	for _, lp := range labels {
		_, err := fmt.Fprintf(
			buf, "%c%s%c%s",
			separator, lp.GetName(), separator, escapeString(lp.GetValue(), true),
		)
		if err != nil {
			return buf.String(), err
		}
	}

	if additionalLabelName != "" {
		_, err := fmt.Fprintf(
			buf, "%c%s%c%s",
			separator, additionalLabelName, separator,
			escapeString(additionalLabelValue, true),
		)
		if err != nil {
			return buf.String(), err
		}
	}

	return buf.String(), nil
}

var (
	escape                = strings.NewReplacer("\\", `\\`, "\n", `\n`)
	escapeWithDoubleQuote = strings.NewReplacer("\\", `\\`, "\n", `\n`, "\"", `\"`)
)

func escapeString(v string, includeDoubleQuote bool) string {
	if includeDoubleQuote {
		return escapeWithDoubleQuote.Replace(v)
	}

	return escape.Replace(v)
}
