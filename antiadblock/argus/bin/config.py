import antiadblock.libs.adb_selenium_lib.browsers.extensions as extensions

WORK_TIMEOUT: int = 50  # min

SELENIUM_EXECUTOR: str = "localhost:4444"  # quota "selenium"

DETECT_TIMEOUT: int = 10  # seconds

FILTERS_LISTS: list[str] = [
    "https://filters.adtidy.org/extension/chromium/filters/1.txt",
    "https://filters.adtidy.org/extension/firefox/filters/1.txt",
    "https://filters.adtidy.org/extension/chromium/filters/2.txt",
    "https://filters.adtidy.org/extension/firefox/filters/2.txt",
    "https://dl.opera.com/download/get/?adblocker=adlist&country=ru",
    "https://easylist-downloads.adblockplus.org/ruadlist+easylist.txt",
    "https://easylist-downloads.adblockplus.org/abp-filters-anti-cv.txt",
    "https://easylist-downloads.adblockplus.org/advblock+cssfixes.txt",
]

AVAILABLE_ADBLOCKS_EXT_CHROME: list[extensions.ExtensionAdb] = [
    extensions.AdblockChromeInternal,
    extensions.AdblockPlusChromeInternal,
    extensions.AdguardChromeInternal,
    extensions.UblockOriginChromeInternal,
    extensions.WithoutAdblock,
]

AVAILABLE_ADBLOCKS_EXT_OPERA: list[extensions.ExtensionAdb] = [
    extensions.OperaExtensionInternal,
    extensions.WithoutAdblock,
]

AVAILABLE_ADBLOCKS_EXT_FIREFOX: list[extensions.ExtensionAdb] = [
    extensions.AdblockFirefox,
    extensions.AdblockPlusFirefox,
    extensions.AdguardFirefox,
    extensions.UblockOriginFirefox,
    extensions.WithoutAdblock,
]

AVAILABLE_ADBLOCKS_EXT_YABRO: list[extensions.ExtensionAdb] = [
    extensions.AdblockChromeInternal,
    extensions.AdblockPlusChromeInternal,
    extensions.AdguardChromeInternal,
    extensions.UblockOriginChromeInternal,
    extensions.WithoutAdblock,
]
