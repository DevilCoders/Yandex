package main

import (
	"log"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/marketplace/lich/dev/lcl/load"
)

var (
	inFileName  string
	outFileName string

	requestParams load.RequestParams
)

var rootCommand = &cobra.Command{
	Use:   "lcl",
	Short: "Run License Check load tests",
}

var transformInputCommand = &cobra.Command{
	Use:   "transform",
	Short: "Run transform command",

	Run: runTransformCommand,
}

var requestCommand = &cobra.Command{
	Use:   "request",
	Short: "Run requests",

	Run: runRequestCommand,
}

func init() {
	transformInputCommand.Flags().StringVarP(&inFileName, "in-file", "i", "", "input file")
	transformInputCommand.Flags().StringVarP(&outFileName, "out-file", "o", "", "output file")

	requestCommand.Flags().StringVarP(&requestParams.BaseURL, "api", "a", "", "api endpoint with path")
	requestCommand.Flags().StringVarP(&requestParams.InFileName, "in-file", "i", "", "input file")
	requestCommand.Flags().StringVarP(&requestParams.AuthToken, "token", "t", "", "iam auth token")
	requestCommand.Flags().StringVar(&requestParams.HostHeader, "host", "", "host header")
	requestCommand.Flags().DurationVarP(&requestParams.Delay, "delay", "d", 100*time.Millisecond, "delay between requests")

	rootCommand.AddCommand(
		transformInputCommand,
		requestCommand,
	)
}

func runTransformCommand(cmd *cobra.Command, args []string) {
	_, err := load.Transform(inFileName, outFileName)
	if err != nil {
		log.Fatal(err)
	}
}

func runRequestCommand(cmd *cobra.Command, args []string) {
	if err := load.MakeRequests(requestParams); err != nil {
		log.Fatal(err)
	}
}
