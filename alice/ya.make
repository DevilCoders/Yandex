OWNER(g:alice)

RECURSE(
    acceptance
    amanda
    amelie
    analytics
    apphost
    bass
    begemot
    beggins
    bitbucket
    boltalka
    cachalot
    cuttlefish
    divkt_renderer
    divkt_templates
    doc
    gamma
    gproxy
    hollywood
    iot
    joker
    json_schema_builder
    kronstadt
    library
    matrix
    megamind
    monitoring
    nlg
    nlu
    notification_creator
    paskills
    personal_cards
    quality
    review_bot
    rtlog
    scenarios
    tasklet
    tests
    tools
    uniproxy
    vins
    wonderlogs
)

IF(JDK_VERSION >= 15)
    RECURSE(memento)
ENDIF()
