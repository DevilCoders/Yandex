LIBRARY()

OWNER(pg darkk)

SRCS(
    rpslogger.cpp
    rpsschedule.cpp
)

GENERATE_ENUM_SERIALIZATION(schedmode.h)

END()
