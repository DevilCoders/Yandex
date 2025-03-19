import os

SB_TOKEN = os.environ.get("SB_TOKEN")
SOLOMON_TOKEN = os.environ.get("SOLOMON_TOKEN")

SCHEME = os.environ.get("GORE_SCHEME", "http")
GORE_URL = os.environ.get("GORE_URL")
GORE_API_ROOT = "api/v0private"
GORE_SERVICES = "services"
GORE_IMPORT = "import"
GORE_EXPORT = "export"
