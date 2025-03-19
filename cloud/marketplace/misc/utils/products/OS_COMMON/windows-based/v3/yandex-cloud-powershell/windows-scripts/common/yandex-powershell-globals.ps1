$global:__BOOTSTRAP_DIR               = "C:\BOOTSTRAP"
$global:__BOOTSTRAP_APPS_DIR          = "$global:__BOOTSTRAP_DIR\APPS"
$global:__BOOTSTRAP_LIBS_DIR          = "$global:__BOOTSTRAP_DIR\WINDOWS-SCRIPTS"
$global:__BOOTSTRAP_LIBS_UNATTEND_DIR = "$global:__BOOTSTRAP_LIBS_DIR\UNATTEND"
$global:__BOOTSTRAP_LIBS_COMMON_DIR   = "$global:__BOOTSTRAP_LIBS_DIR\COMMON"

$global:__SETUP_DIR                       = "C:\Windows\Setup"
$global:__SETUPCOMPLETE_DIR               = "$global:__SETUP_DIR\Scripts"
$global:__SETUPCOMPLETE_SUPPLEMENTARY_DIR = "$global:__SETUP_DIR\SupplementaryScripts"
$global:__SETUP_SYSPREP_TAG               = "C:\Windows\System32\Sysprep\Sysprep_succeeded.tag"

$global:__KMSServer = "kms.cloud.yandex.net:1688"

# remove after corecting its usage, for backward compatability
$global:__BOOTSTRAP_DIR__      = $global:__BOOTSTRAP_DIR
$global:__BOOTSTRAP_LIBS_DIR__ = $global:__BOOTSTRAP_LIBS_DIR
$global:__BOOTSTRAP_APPS_DIR__ = $global:__BOOTSTRAP_APPS_DIR
