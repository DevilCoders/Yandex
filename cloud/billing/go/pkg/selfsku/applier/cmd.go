package applier

import "github.com/spf13/cobra"

func ExtractCmd() *cobra.Command {
	return &cobra.Command{
		Use:   "extract [extraction path]",
		Short: "extract bundle content to filesystem",
		Args:  cobra.MaximumNArgs(1),
		RunE:  extractBundle,
	}
}

func StatsCmd() *cobra.Command {
	return &cobra.Command{
		Use:   "stats",
		Short: "show bundle statistics",
		Args:  cobra.NoArgs,
		RunE:  statBundle,
	}
}

func extractBundle(cmd *cobra.Command, args []string) error {
	cmd.SilenceUsage = true

	path := "."
	if len(args) > 0 {
		path = args[0]
	}
	applier, err := NewFSApplier(path)
	if err != nil {
		return err
	}
	return Resources(cmd.Context(), applier)
}

func statBundle(cmd *cobra.Command, _ []string) error {
	cmd.SilenceUsage = true

	applier := NewStatsApplier()
	if err := Resources(cmd.Context(), applier); err != nil {
		return err
	}

	cmd.Printf("Units: %d\n", applier.Units)
	cmd.Printf("Services: %d\n", applier.Services)
	cmd.Printf("Schemas: %d\n", applier.Schemas)
	cmd.Printf("Skus: %d\n", applier.Skus)
	return nil
}
