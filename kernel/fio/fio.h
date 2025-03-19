#pragma once

enum ENameType {
    FirstName = 0,
    Surname,
    MiddleName,
    InitialName,
    InitialPatronymic,
    NameTypeCount,
    NoName,
    UnknownNameType,
};

enum EFIOType {
    FIOname = 0 /* "fioname" */,
    FIname /* "finame" */,
    Fname /* "fname" */,
    IOname /* "ioname" */,
    Iname /* "iname" */,
    FIinName /* "fiinname" */,
    FIinOinName /* "fiinoinname" */,
    FIOinName /* "fioinname" */,
    MWFio /* "mwfio" */,
    FIOTypeCount,
    FInameIn,
    IFnameIn,
    IOnameIn,
    IOnameInIn,
    InameIn,
};

const char* FioType2Length(EFIOType fioType);
