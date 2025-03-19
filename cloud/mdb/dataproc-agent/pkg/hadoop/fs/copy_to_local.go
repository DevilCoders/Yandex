package fs

import (
	"net/http"
	"os/exec"
	"regexp"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func CopyToLocal(sourceURL string, localPath string) error {
	// `hadoop fs -copyToLocal` does not return meaningful error message if http file is not found
	// so check this case manually in order to provide user with descriptive error
	re := regexp.MustCompile(`^https?://`)
	if re.MatchString(sourceURL) {
		resp, err := http.Head(sourceURL)
		if err != nil {
			return err
		}
		if resp.StatusCode != http.StatusOK {
			return xerrors.New(resp.Status)
		}
	}

	return doCopyToLocal(sourceURL, localPath)
}

var doCopyToLocal = execCopyToLocal

func execCopyToLocal(sourceURL string, localPath string) error {
	cmd := exec.Command("hadoop", "fs", "-copyToLocal", sourceURL, localPath)
	out, err := cmd.CombinedOutput()
	if err != nil {
		return copyToLocalOutputAsError(out)
	}

	return nil
}

func copyToLocalOutputAsError(output []byte) error {
	lines := strings.Split(string(output), "\n")
	var usefulLines []string
	withinUsage := false
	for _, line := range lines {
		usefulLine := !withinUsage
		if strings.HasPrefix(line, "Usage:") {
			withinUsage = !withinUsage
			usefulLine = false
		}
		if !usefulLine {
			continue
		}
		if strings.Contains(line, " WARN ") || strings.Contains(line, " INFO ") {
			continue
		}

		re := regexp.MustCompile(`^-?copyToLocal:(\s)*`)
		line = re.ReplaceAllLiteralString(line, "")
		line = strings.TrimSpace(line)

		if line != "" {
			usefulLines = append(usefulLines, line)
		}
	}

	errorMessage := strings.Join(usefulLines, ", ")
	return xerrors.New(errorMessage)
}
