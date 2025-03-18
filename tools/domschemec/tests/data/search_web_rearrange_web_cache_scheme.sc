namespace NWebCacheRule;

struct TWebCache {
    Enabled : bool(default = false);
    TargetGrouping : string (default = "d");
    CacheDuration : ui64 (default = 15);
    CachePrefix : string (default = "WebCache");
    UidInCache : bool (default = false);
    TopToCache : ui64 (default = 30);
};
