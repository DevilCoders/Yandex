package heredoc

import (
	"fmt"
	"strings"
	"unicode"
)

const maxInt = int(^uint(0) >> 1)

// Doc returns un-indented string as here-document.
func Doc(raw string) string {
	skipFirstLine := false
	if len(raw) > 0 && raw[0] == '\n' {
		raw = raw[1:]
	} else {
		skipFirstLine = true
	}

	lines := strings.Split(raw, "\n")

	minIndentSize := getMinIndent(lines, skipFirstLine)
	lines = removeIndentation(lines, minIndentSize, skipFirstLine)

	return strings.Join(lines, "\n")
}

// getMinIndent calculates the minimum indentation in lines, excluding empty lines.
func getMinIndent(lines []string, skipFirstLine bool) int {
	minIndentSize := maxInt

	for i, line := range lines {
		if i == 0 && skipFirstLine {
			continue
		}

		indentSize := 0
		for _, r := range line {
			if unicode.IsSpace(r) {
				indentSize += 1
			} else {
				break
			}
		}

		if len(line) == indentSize {
			if i == len(lines)-1 && indentSize < minIndentSize {
				lines[i] = ""
			}
		} else if indentSize < minIndentSize {
			minIndentSize = indentSize
		}
	}
	return minIndentSize
}

// removeIndentation removes n characters from the front of each line in lines.
// Skips first line if skipFirstLine is true, skips empty lines.
func removeIndentation(lines []string, n int, skipFirstLine bool) []string {
	for i, line := range lines {
		if i == 0 && skipFirstLine {
			continue
		}

		if len(lines[i]) >= n {
			lines[i] = line[n:]
		}
	}
	return lines
}

// Docf returns unindented and formatted string as here-document.
// Formatting is done as for fmt.Printf().
func Docf(raw string, args ...interface{}) string {
	return fmt.Sprintf(Doc(raw), args...)
}
