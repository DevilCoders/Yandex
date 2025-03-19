@sweep
Feature: Noop feature for explicit resource sweeping
  # Some behave runners does not support empty-suite runs which we use in Makefile to trigger `after_all` hook
  # In that case that no-op feature can be used
  Scenario: Sweep
      Given cluster name "sweep"
