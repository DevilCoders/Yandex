GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    action.go
    analytics.go
    background_image.go
    capability.go
    capability_colorsetting.go
    capability_custom_button.go
    capability_mode.go
    capability_onoff.go
    capability_quasar.go
    capability_quasar_server_action.go
    capability_quasar_value.go
    capability_range.go
    capability_testing.go
    capability_toggle.go
    capability_video_stream.go
    colors.go
    const.go
    deferred_events.go
    device.go
    device_config.go
    device_testing.go
    endpoint.go
    error.go
    favorite.go
    group.go
    group_testing.go
    household.go
    household_testing.go
    hypothesis.go
    hypothesis_testing.go
    intent_state.go
    network.go
    network_testing.go
    nlg.go
    origin.go
    property.go
    property_event.go
    property_float.go
    protocols.go
    room.go
    room_testing.go
    scenario.go
    scenario_step.go
    scenario_step_actions.go
    scenario_step_delay.go
    scenario_testing.go
    scenario_trigger.go
    sharing.go
    skill.go
    sorting.go
    speakers.go
    stereopair.go
    suggestions.go
    tandem.go
    user.go
    user_info.go
    user_storage.go
    user_testing.go
    validation.go
)

GO_TEST_SRCS(
    analytics_test.go
    capability_colorsetting_test.go
    capability_custom_button_test.go
    capability_mode_test.go
    capability_onoff_test.go
    capability_quasar_server_action_test.go
    capability_quasar_test.go
    capability_range_test.go
    capability_toggle_test.go
    capability_video_stream_test.go
    colors_test.go
    const_test.go
    deferred_events_test.go
    group_test.go
    network_test.go
    nlg_test.go
    property_event_test.go
    property_float_test.go
    property_test.go
    protocols_test.go
    scenario_step_test.go
    scenario_test.go
    scenario_trigger_test.go
    sharing_test.go
    sorting_test.go
    stereopair_test.go
    suggestions_test.go
    user_info_test.go
    user_storage_test.go
    validation_test.go
)

GO_XTEST_SRCS(
    capability_test.go
    device_test.go
    hypothesis_test.go
    hypothesis_to_capability_test.go
    tandem_test.go
)

END()

RECURSE(
    directives
    quasar
    suggestions
    test
    tuya
    xiaomi
    yandexio
)

RECURSE_FOR_TESTS(gotest)
