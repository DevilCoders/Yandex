Overview
===
This library is binding for C++ library with JNI.
It provides ability to operate with TVM. Library is fast enough to get or check tickets for every request without burning CPU.

Usually you don't need to get dynamic library (.so/.dylib/.dll) with your handes: these libs would packed into tvmauth-java.jar.
___
`ya make` builds .jar for tvmauth with dynamic library inside **only** for current platform.

**WARNING!**: if you need to build your project on one platform (e.g., `darwin`) and run it on other platform (e.g., `linux`),  you need to specify `targert-platform` to provide correct dynamic lib in .jar.

For example: `ya package --target-platform linux ...`

Or you can [force](https://a.yandex-team.ru/arc/trunk/arcadia/library/java/tvmauth/build_jar_with_so/jar.json?rev=r7743395#L34-36) `target-platform` in your .json for `ya package`
___

[Home page of project](https://wiki.yandex-team.ru/passport/tvm2/).

You can find some examples in [here](https://a.yandex-team.ru/arc/trunk/arcadia/library/java/tvmauth/examples).

You can ask questions: [PASSPORTDUTY](https://st.yandex-team.ru/createTicket?queue=PASSPORTDUTY&_form=77618)


NativeTvmClient
===
Don't forget to collect logs from client.

If you don't need an instance of client anymore but your process would keep running, please `close()` this instance.
___
`NativeTvmClient` allowes:
1. `getServiceTicketFor()` - to fetch ServiceTicket for outgoing request
2. `checkServiceTicket()` - to check ServiceTicket from incoming request
3. `checkUserTicket()` - to check UserTicket from incoming request

All methods are thread-safe.

You should check status of `CheckedServiceTicket` or `CheckedUserTicket` for equality 'Ok'. You can get ticket fields (src/uids/scopes) only for correct ticket. Otherwise exception will be thrown.
___
You should check status of client with `getStatus()`:
* `OK` - nothing to do here
* `WARNING` - **you should trigger your monitoring alert**

      Normal operation of TvmClient is still possible but there are problems with refreshing cache, so it is expiring.
      Is tvm-api.yandex.net accessible?
      Have you changed your TVM-secret or your backend (dst) deleted its TVM-client?

* `ERROR` - **you should trigger your monitoring alert and close this instance for user-traffic**

      TvmClient's cache is already invalid (expired) or soon will be: you can't check valid CheckedServiceTicket or be authenticated by your backends (dsts)

___
Constructor creates system thread for refreshing cache - so do not fork your proccess after creating `NativeTvmClient` instance. Constructor leads to network I/O. Other methods always use memory.

Exceptions maybe thrown from constructor:
* `RetriableException` - maybe some network trouble: you can try to create client one more time.
* `NonRetriableException` - settings are bad: fix them.

Other methods can throw exception only if you try to use unconfigured abilities (for example, you try to get fetched ServiceTicket for some dst but you didn't configured it in settings).

___
You can choose way for fetching data for your service operation:
* http://localhost:{port}/tvm - recomended way
* https://tvm-api.yandex.net

TvmTool
------------
`TvmClient` uses local http-interface (tvmtool) to get state. This interface can be provided with tvmtool (local daemon) or Qloud/YP (local http api in container).
See more: https://wiki.yandex-team.ru/passport/tvm2/tvm-daemon/.

`TvmClient` fetches configuration from tvmtool, so you need only to tell client how to connect to it and tell which alias of tvm id should be used for this `TvmClient` instance.

TvmApi
------------
`TvmClient` uses https://tvm-api.yandex.net to get state.
First of all: please use `setDiskCacheDir()` - it provides reliability for your service and for tvm-api.
Please check restrictions of this method.
