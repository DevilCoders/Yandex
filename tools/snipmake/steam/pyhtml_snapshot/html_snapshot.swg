%module pyhtml_snapshot
%include <typemaps.i>
%include <python/exception.i>
%include <python/std_vector.i>
%include <bindings/swiglib/stroka.swg>
%include <bindings/swiglib/defaults.swg>

%{
    #include <tools/snipmake/steam/pyhtml_snapshot/py_api.h>
    #include <util/generic/yexception.h>

    struct TPyDecRef
    {
        static void Destroy(PyObject* obj)
        {
            Py_DECREF(obj);
        }
    };

    class TPyCallback
    {
    private:
        PyObject *Func;
        TPyCallback(const TPyCallback& other);

    public:
        TPyCallback()
            : Func(NULL)
        {
        }

        void Assign(PyObject *func)
        {
            Py_XINCREF(func);
            Func = func;
        }

        virtual ~TPyCallback()
        {
            Py_XDECREF(Func);
        }

        template<typename... Args>
        TAutoPtr<PyObject, TPyDecRef> BuildArgs(const TStringBuf& format, Args... args)
        {
            return Py_BuildValue(format.data(), args...);
        }

        TString CallWithTuple(TAutoPtr<PyObject, TPyDecRef> args)
        {
            if (!Func || Py_None == Func || !PyCallable_Check(Func)) {
                ythrow yexception() << "The callback function turned out to be not a function at all";
            }

            PyObject *pyResult = PyObject_Call(Func, args.Get(), 0);
            if (pyResult == NULL) {
                if (PyErr_Occurred()) {
                    PyErr_Print();
                    PyErr_Clear();
                    ythrow yexception() << "Callback Python function threw a Python exception; traceback is on stderr";
                }
                else {
                    ythrow yexception() << "Callback Python function failed in a mysterious way, returning NULL";
                }
            }
            if (!PyString_Check(pyResult)) {
                Py_XDECREF(pyResult);
                ythrow yexception() << "Callback Python function returned something that is not a string";
            }
            TString result(PyString_AsString(pyResult));
            Py_XDECREF(pyResult);
            return result;
        }
    };

    class TUrlXform: public TPyCallback, public NSteam::IUrlXform
    {
    public:
        TString Call(const TStringBuf& url) override
        {
            return CallWithTuple(BuildArgs("(s)", url.data()));
        }
    };

    class TCssXform: public TPyCallback, public NSteam::ICssXform
    {
    public:
        TString Call(const TStringBuf& css, const NSteam::IUrlWrapper& baseUrl, ECharset& encoding, bool isInline) override
        {
            PyObject* pyBaseUrl = SWIG_NewPointerObj(SWIG_as_voidptr(&baseUrl), SWIGTYPE_p_NSteam__IUrlWrapper, 0);
            return CallWithTuple(BuildArgs("(sNii)", css.data(), pyBaseUrl, (int)encoding, (int)isInline));
        }
    };
%}

%typemap(in) NSteam::IUrlXform & (TUrlXform cb)
{
    cb.Assign((PyObject*)$input);
    $1 = &cb;
}

%typemap(in) NSteam::ICssXform & (TCssXform cb)
{
    PyObject* in = (PyObject*)$input;
    cb.Assign(in);
    $1 = &cb;
}

%newobject NSteam::TAsyncFetchResultPy::WaitForCompletion;
%newobject NSteam::TWebContentParser::RewriteContent;

%rename(WebContentParser) TWebContentParser;
%rename(AsyncFetchResult) TAsyncFetchResultPy;
%rename(FetchedDoc) TFetchedDoc;
%rename(ZoraFetcher) TZoraFetcherPy;
%rename(DirectFetcher) TDirectFetcherPy;

%include <library/cpp/charset/doccodes.h>
%include <tools/snipmake/steam/html_snapshot/fetched_doc.h>
%include <tools/snipmake/steam/html_snapshot/misc.h>
%thread;
%include "py_api.h"
%nothread;
