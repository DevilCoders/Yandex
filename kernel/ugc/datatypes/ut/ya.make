OWNER(g:ugc)

UNITTEST_FOR(kernel/ugc/datatypes)

SRCS(
    objectid_ut.cpp
    props_ut.cpp
    update_ut.cpp
    userid_ut.cpp
)

RESOURCE(
    data/update_01.txt    /UpdateFull
    data/update_02.txt    /UpdateWithoutApp
    data/update_03.txt    /UpdateWithoutUserId
    data/update_04.txt    /UpdateWithoutTime
    data/update_05.txt    /UpdateWithoutUpdateId
    data/update_06.txt    /UpdateWithoutVersion
    data/update_07.txt    /UpdateWithoutType
    data/update_08.txt    /UpdateOnlyRequiredFields
)

END()
