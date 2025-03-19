This module is used only if Mr. Prober's control plane is deployed in 
environment other than data plane.

This module CAN NOT contain any yandex-provider resources, because
yandex provider depends on Public API which is unavailable on some 
environments (i.e. Testing).