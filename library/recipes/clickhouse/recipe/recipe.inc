IF(OS_WINDOWS)
    MESSAGE(FATAL_ERROR "ClickHouse server does not work on Windows")
ENDIF()

# Fetch OS-specific ClickHouse release versions
IF(CLICKHOUSE_VERSION == "18.14.18")
    IF(OS_LINUX)
        DATA(sbr://822203196=clickhouse)
    ELSE() # macOs - no build variable for macOS
        DATA(sbr://861941919=clickhouse)
    ENDIF()
ELSEIF(CLICKHOUSE_VERSION == "19.14.7.15")
    IF(OS_LINUX)
        DATA(sbr://1237366583=clickhouse)
    ELSE() # macOS - no build variable for macOS
        MESSAGE(FATAL_ERROR "Upload MacOS Sandbox resource for ClickHouse version " $CLICKHOUSE_VERSION)
    ENDIF()
ELSEIF(CLICKHOUSE_VERSION == "19.15.3.6")
    IF(OS_LINUX)
        DATA(sbr://1170310743=clickhouse)
    ELSE() # macOS - no build variable for macOS
        MESSAGE(FATAL_ERROR "Upload MacOS Sandbox resource for ClickHouse version " $CLICKHOUSE_VERSION)
    ENDIF()
ELSEIF(CLICKHOUSE_VERSION == "20.3.5.21")
    IF(OS_LINUX)
        DATA(sbr://1444815803=clickhouse)
    ELSE() # macOS - no build variable for macOS
        MESSAGE(FATAL_ERROR "Upload MacOS Sandbox resource for ClickHouse version " $CLICKHOUSE_VERSION)
    ENDIF()
ELSEIF(CLICKHOUSE_VERSION == "20.3.9.70")
    IF(OS_LINUX)
        DATA(sbr://1540350178=clickhouse)
    ELSE() # macOS - no build variable for macOS
        MESSAGE(FATAL_ERROR "Upload MacOS Sandbox resource for ClickHouse version " $CLICKHOUSE_VERSION)
    ENDIF()
ELSEIF(CLICKHOUSE_VERSION == "20.3.15.133")
    IF(OS_LINUX)
        DATA(sbr://1645333850=clickhouse)
    ELSE() # macOS - no build variable for macOS
        MESSAGE(FATAL_ERROR "Upload MacOS Sandbox resource for ClickHouse version " $CLICKHOUSE_VERSION)
    ENDIF()
ELSEIF(CLICKHOUSE_VERSION == "21.3.7.62")
    IF(OS_LINUX)
        DATA(sbr://2118382614=clickhouse)
    ELSE() # macOS - no build variable for macOS
        DATA(sbr://2109392005=clickhouse)
    ENDIF()
ELSEIF(NOT CLICKHOUSE_VERSION)
    SET(CLICKHOUSE_VERSION ARCADIA)
ELSEIF(CLICKHOUSE_VERSION != "ARCADIA")
    MESSAGE(FATAL_ERROR "No Sandbox resource for ClickHouse version " $CLICKHOUSE_VERSION)
ENDIF()

IF(CLICKHOUSE_VERSION == "ARCADIA")
    DEPENDS(contrib/clickhouse/programs)
ENDIF()

DATA(arcadia/library/recipes/clickhouse/recipe/config)
