package main

import (
	"errors"
	"os"
)

////////////////////////////////////////////////////////////////////////////////

type completedDisksIface interface {
	load(filename string) error
	close()

	addCompletedDisk(diskID string) error
	isDiskCompleted(diskID string) bool
}

////////////////////////////////////////////////////////////////////////////////

type completedDisks struct {
	disks   map[string]struct{}
	logFile *os.File
}

func (c *completedDisks) load(filename string) error {
	c.disks = make(map[string]struct{})

	lines, err := readFile(filename)
	if err != nil && !errors.Is(err, os.ErrNotExist) {
		return err
	}

	for _, l := range lines {
		c.disks[l] = struct{}{}
	}

	c.logFile, err = os.OpenFile(filename, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)

	return err
}

func (c *completedDisks) addCompletedDisk(diskID string) error {
	c.disks[diskID] = struct{}{}
	if c.logFile == nil {
		return nil
	}
	_, err := c.logFile.WriteString(diskID + "\n")
	return err
}

func (c *completedDisks) close() {
	if c.logFile != nil {
		_ = c.logFile.Close()
	}
}

func (c *completedDisks) isDiskCompleted(diskID string) bool {
	_, found := c.disks[diskID]
	return found
}
