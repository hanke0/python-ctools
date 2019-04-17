FROM centos:7

ENV PYENV_ROOT /opt/pyenv
ENV PATH "$PYENV_ROOT/shims:$PYENV_ROOT/bin:$PATH"
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

RUN yum --enablerepo=extras install -y epel-release \
    && bash -c "$(curl -sSL https://setup.ius.io/)" \
    && yum update -y \
    && yum install -y \
                gcc  \
                bzip2-devel \
                zlib-devel \
                openssl-devel \
                readline-devel \
                sqlite-devel \
                libffi-devel \
                git \
                make \
    && yum clean all && rm -rf /var/cache/yum


RUN mkdir -p "$PYENV_ROOT" \
    && git clone https://github.com/pyenv/pyenv.git "$PYENV_ROOT" \
    && cd "$PYENV_ROOT" \
    && git checkout -q ${PYENV_VERSION} \
    && rm -r "$PYENV_ROOT/.git"

RUN PYTHON2_VERSION="2.7.15" \
    && PYTHON34_VERSION="3.4.9" \
    && PYTHON35_VERSION="3.5.6" \
    && PYTHON36_VERSION="3.6.8" \
    && PYTHON37_VERSION="3.7.3" \
    && pyenv install "$PYTHON2_VERSION" \
    && pyenv install "$PYTHON34_VERSION" \
    && pyenv install "$PYTHON35_VERSION" \
    && pyenv install "$PYTHON36_VERSION" \
    && pyenv install "$PYTHON37_VERSION" \
    && pyenv global system "$PYTHON2_VERSION" "$PYTHON37_VERSION" "$PYTHON36_VERSION" "$PYTHON35_VERSION" "$PYTHON34_VERSION"


RUN pip3.4 --no-cache-dir install wheel \
    && pip3.5 --no-cache-dir install wheel \
    && pip3.6 --no-cache-dir install wheel \
    && pip3.7 --no-cache-dir install wheel

CMD /bin/bash
