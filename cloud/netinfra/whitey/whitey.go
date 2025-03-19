package main

import (
	"flag"
	"log"

	"whitey/internal"
)

func main() {
	// Getting configuration file
	cfgFilePath := flag.String("config", "/etc/whitey/config.yml", "Whitey configuration file path")
	// logFilePath := flag.String("config", "/var/whitey.log", "Whitey log file path")
	// logLevel := flag.String("level", "error", "Log level")
	flag.Parse()

	// Configuration parameters initialization
	app, err := internal.InitApp(*cfgFilePath)
	if err != nil {
		log.Fatal(err)
	}

	// Create logger
	// appLogger := internal.SetupSimpleLogger(*logFilePath, *logLevel)
	//
	// appLogger.OpenLog()
	// appLogger.LogHeader("** APPLICATION STARTED **")
	//
	// collectors := appCfg.Collectors

	// Output pre-start diagnostic information
	var infoMes string
	if app.Independent {
		infoMes = "Will be used Independent mode with following collectors:"
	} else {
		infoMes = "Will be used Dependent mode with following collectors:"
	}

	// appLogger.LogUndef(infoMes)
	log.Println(infoMes)

	// for _, collector := range collectors {
	// 	appLogger.LogUndef(fmt.Sprint(collector))
	// 	log.Println(collector)
	// }

	// appLogger.LogHeader("Start collecting and pushing metrics")
	// appLogger.CloseLog()

	app.Run()
}
