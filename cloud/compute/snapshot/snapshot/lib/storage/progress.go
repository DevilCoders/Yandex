package storage

import "fmt"

const (
	progressFmtPrint = "%05.2f%% completed"
	progressFmtScan  = "%f%% completed"
)

// GetProgressFromDescription parses progress from state description.
func GetProgressFromDescription(desc string) *float32 {
	var f float32
	n, err := fmt.Sscanf(desc, progressFmtScan, &f)
	if n == 0 || err != nil {
		return nil
	}
	return &f
}

// BuildDescriptionFromProgress formats progress into state description.
func BuildDescriptionFromProgress(progress float32) string {
	return fmt.Sprintf(progressFmtPrint, progress)
}
