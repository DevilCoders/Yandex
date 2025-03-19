#pragma once
/******************************************************************************
 *         This file is a part of Yandex.Software ver.3.6                     *
 *         Copyright (c) 1996-2009 OOO "Yandex". All rights reserved.         *
 *         Call software@yandex-team.ru for support.                          *
 ******************************************************************************/
#include "yandind.h"

#ifdef __cplusplus
extern "C" {
#endif

class IParsedDocProperties;

typedef void *DATAOBJ;

typedef enum YDS_STATUS {
    YDS_ERROR = -2,
    YDS_EOF   = -1,
    YDS_OK    =  0
} YDS_STATUS;

typedef int (*FUNC_DataSrc_OpenIndexingSession)(DATAOBJ *DataObj, const char *Config, const INDEX_CONFIG *Ic);

typedef int (*FUNC_DataSrc_OpenSearchSession)(DATAOBJ *DataObj, const char *config, const char *indexprefix, YX_LOGNOTIFY YxLogNotify, YX_LOGOBJ LogObj);

typedef int (*FUNC_DataSrc_OpenDoc)(DATAOBJ DataObj, DOCINPUT *di);

typedef int (*FUNC_DataSrc_ReadDocumentBytes)(DATAOBJ DataObj, YX_DOCOBJ ReaderObj, void *toFill, size_t maxToRead, size_t *Read);

typedef int (*FUNC_DataSrc_CloseDoc)(DATAOBJ DataObj, DOCINPUT *di, const IParsedDocProperties *pars);

typedef int (*FUNC_DataSrc_CloseSession)(DATAOBJ DataObj);

typedef
enum YDS_PROPERTY {
    YDSP_EXTENDID,
    YDSP_SAVEDFILES = 2,
} YDS_PROPERTY;

typedef void *YDSPVALUE;

typedef int (*FUNC_DataSrc_GetProperty)(DATAOBJ DataObj, YDS_PROPERTY PropName, YDSPVALUE *pPropValue);

typedef
struct DATASRC_LIB {
    FUNC_DataSrc_OpenIndexingSession OpenIndexingSession;
    FUNC_DataSrc_OpenSearchSession   OpenSearchSession;
    FUNC_DataSrc_OpenDoc             OpenDoc;
    FUNC_DataSrc_ReadDocumentBytes   ReadDocumentBytes;
    FUNC_DataSrc_CloseDoc            CloseDoc;
    FUNC_DataSrc_CloseSession        CloseSession;
    FUNC_DataSrc_GetProperty         GetProperty;
} DATASRC_LIB;

#ifdef __cplusplus
} /* extern "C" */
#endif
