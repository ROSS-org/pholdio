IF(BGPM)
    INCLUDE_DIRECTORIES(${BGPM_LIB})
ENDIF(BGPM)

## ROSS
INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR} ${ROSS_BINARY_DIR})

## RIO
INCLUDE_DIRECTORIES(${RIO_SOURCE_DIR} ${RIO_BINARY_DIR})

## Print PHOLD-IO Hash
LIST(APPEND CMAKE_MODULE_PATH "${ROSS_SOURCE_DIR}/cmake/")
INCLUDE(GetGitRevisionDescription)
GET_GIT_HEAD_REVISION(GIT_RESPEC_PH GIT_SHA1_PH)

## Build Configuration File
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/pholdio-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/pholdio-config.h @ONLY)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/pholdio-config.h.in PROPERTIES GENERATED FALSE)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/pholdio-config.h PROPERTIES GENERATED TRUE)

## PHOLD-IO Model
## this model *requires* RIO to build
SET(pholdio_srcs
    pholdio.c
    pholdio.h
    pholdio-config.h)

ADD_EXECUTABLE(pholdio ${pholdio_srcs})

IF(BGPM)
    TARGET_LINK_LIBRARIES(pholdio ROSS RIO imp_bgpm m)
ELSE(NOT(BGPM))
    TARGET_LINK_LIBRARIES(pholdio ROSS RIO m)
ENDIF(BGPM)

## Delta-PHOLD-IO Model
## this model can be built with or without RIO
SET(dpholdio_srcs
    dpholdio.c
    dpholdio.h
    pholdio-config.h)

ADD_EXECUTABLE(dpholdio ${dpholdio_srcs})

IF(BGPM)
    IF(USE_RIO)
        TARGET_LINK_LIBRARIES(dpholdio ROSS RIO imp_bgpm m)
    ELSE(NOT(USE_RIO))
        TARGET_LINK_LIBRARIES(dpholdio ROSS imp_bgpm m)
    ENDIF(USE_RIO)
ELSE(NOT(BGPM))
    IF(USE_RIO)
        TARGET_LINK_LIBRARIES(dpholdio ROSS RIO m)
    ELSE(NOT(USE_RIO))
        TARGET_LINK_LIBRARIES(dpholdio ROSS m)
    ENDIF(USE_RIO)
ENDIF(BGPM)

