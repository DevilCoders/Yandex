package main

import (
	"fmt"
	"log"
	"net/http"

	"github.com/spf13/cobra"
)

////////////////////////////////////////////////////////////////////////////////

func main() {
	rootCmd := &cobra.Command{}

	var imageFilePath string
	var port int

	startCmd := &cobra.Command{
		Use:   "start",
		Short: "Start image file server",
		Run: func(cmd *cobra.Command, args []string) {
			http.HandleFunc("/",
				func(w http.ResponseWriter, r *http.Request) {
					http.ServeFile(w, r, imageFilePath)
				},
			)
			log.Fatal(http.ListenAndServe(fmt.Sprintf(":%v", port), nil))
		},
	}

	startCmd.Flags().StringVar(
		&imageFilePath,
		"image-file",
		"",
		"Path to the image file",
	)
	if err := startCmd.MarkFlagRequired("image-file"); err != nil {
		log.Fatalf("Error setting flag image-file as required: %v", err)
	}

	startCmd.Flags().IntVar(
		&port,
		"port",
		8080,
		"Http port to listen on",
	)

	rootCmd.AddCommand(startCmd)

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Failed to run: %v", err)
	}
}
