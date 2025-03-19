OWNER(g:cloud-kms)

PY3_PROGRAM(deploy)

PEERDIR(
	contrib/python/dateutil
	contrib/python/dnspython
	contrib/python/PyYAML
	contrib/python/requests
)

PY_SRCS(
	balancer.py
	colorterm.py
	MAIN deploy.py
	dnsresolve.py
	fail.py
	instance.py
	playsound.py
	podmanifest.py
	remote.py
	terraform.py
	tokens.py
	valuediff.py
)

END()
