%include <typemaps.i>
%include <bindings/swiglib/stroka.swg>
%include <bindings/swiglib/defaults.swg>

%begin %{
    #include <util/generic/string.h>
    #include <library/python/messagebus/wrapper/wrapper.h>

    #ifdef __clang__
    #pragma clang diagnostic ignored "-Wself-assign"
    #endif
%}

#if defined(SWIGPYTHON)
%pythoncode %{
    import sys
%}
#endif

%apply TString* OUTPUT {TString* replyName, TString* replyBytes};
%apply bool* OUTPUT {bool* success};
%include <library/python/messagebus/wrapper/wrapper.h>

#if defined(SWIGPYTHON)
%extend NMessageBusWrapper::TSyncSession {
    %pythoncode %{
        def Send(self, msg, responseType=None, destinationHost="", destinationPort=0, needReply=True):
            responseTypeFullName, bytes = self.SendBytes(
                    msg.DESCRIPTOR.full_name,
                    msg.SerializeToString(),
                    destinationHost,
                    destinationPort,
                    needReply)
            if not needReply:
                return
            if responseType is None:
                responseTypeClassName = responseTypeFullName.rsplit('.', 1)[-1]
                # try to find type of response in the module, that contains request
                try:
                    requestModule = sys.modules[msg.__module__]
                    res = getattr(requestModule, responseTypeClassName)()
                except (KeyError, AttributeError):
                    raise RuntimeError("Can't find response type: '{0}'".format(responseTypeFullName))
            else:
                res = responseType()
            res.ParseFromString(bytes)
            return res
    %}
};


%extend NMessageBusWrapper::TSyncServer {
    %pythoncode %{
        def Reply(self, msg, reply):
            return self.SendReply(msg, reply.DESCRIPTOR.full_name, reply.SerializeToString())
    %}
};

%extend NMessageBusWrapper::TSyncServerIterator {
    %pythoncode %{
        def __iter__(self):
            return self

        def next(self):
            got = False
            while not got:
                mess, got = self.Next()
            return mess

    %}
};
#endif
