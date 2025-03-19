FROM cr.yandex/crp6ro8l0u0o3qgmvv3r/java:2019-06-25T21-05
RUN     apt-get update; \
        apt-get install \
        apt-transport-https \
        gnupg-agent \
        software-properties-common -y; \
        \
        curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add - ;\
        \
        apt-key fingerprint 0EBFCD88; \
        \
        add-apt-repository \
        "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
        $(lsb_release -cs) \
        stable" ; \
        apt-get update; \
        \
        apt-get install docker-ce docker-ce-cli containerd.io -y