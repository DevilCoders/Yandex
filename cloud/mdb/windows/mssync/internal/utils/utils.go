package utils

import (
	"log"
)

type LoggerType struct {
	InfoLogger    *log.Logger
	WarningLogger *log.Logger
	ErrorLogger   *log.Logger
}
