#pragma once
/******************************************************************************
 *         This file is a part of Yandex.Software ver.3.6                     *
 *         Copyright (c) 1996-2009 OOO "Yandex". All rights reserved.         *
 *         Call software@yandex-team.ru for support.                          *
 ******************************************************************************/

#ifndef initcb_h
#define initcb_h

class IIndexProperty;
class ICacher;

class ILogCallback
{
public:
    ILogCallback() noexcept {
    };
    virtual ~ILogCallback() {
    };
    virtual void Write(const char* message, unsigned len) = 0;
    virtual void ReopenLog() = 0;
};

class IInitContext
{
public:
    IInitContext() noexcept {
    };
    virtual ~IInitContext() {
    };
    virtual void SetLogCallback(ILogCallback* callback) = 0;
    virtual const IIndexProperty* GetIndexProperty() const = 0;
    virtual const char* GetCmdLine() const = 0;
    virtual const char* GetWorkDir() const = 0;
    virtual void SetRequestCacher(ICacher* cacher) const = 0;
};

#endif
