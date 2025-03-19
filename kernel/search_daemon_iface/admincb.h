#pragma once
/******************************************************************************
 *         This file is a part of Yandex.Software ver.3.6                     *
 *         Copyright (c) 1996-2009 OOO "Yandex". All rights reserved.         *
 *         Call software@yandex-team.ru for support.                          *
 ******************************************************************************/

#ifndef admincb_h
#define admincb_h

#include <stdio.h>

class IAdminContext
{
    public:
        virtual ~IAdminContext() {}

        virtual void write( const void* buf, size_t length ) = 0;
        //return 0 if no such param found
        virtual const char* getCgiParameter( const char* param, size_t num ) const = 0;
        virtual bool haveCgiParameter( const char* param ) const = 0;
        virtual size_t numCgiParameters( const char* param ) const = 0;
};

#endif
