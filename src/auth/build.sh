#!/bin/bash

###### start config ######

# auto detect for apt and yum.
# also you can specify one.
#pm="apt"

# if unsure, gcc is ok.
# this should be a package name, we will install it later.
cc="gcc"

# version of openssh to be based on
# default download source is https://mirror.leaseweb.com/pub/OpenBSD/OpenSSH/portable/${openssh}.tar.gz
openssh="openssh-9.0p1"
# must be a absolute path
installpath="/baithook"
# what command you want to use as container's daemon.
# must be a absolute path
runsh="/run.sh"
# make -j ${job}
job=4

## we need to install these libs on ubuntu:
# version of zlib (effectes only on ubuntu)
zlib="zlib-1.2.12"
# version of openssl (effectes only on ubuntu)
openssl="openssl-1.1.1h"

###### end config ######

# software package manager auto detect.
if [ ! -x "$(command -v ${pm})" ]; then
     if [ -x "$(command -v yum)" ]; then
          pm="yum"
     fi
     if [ -x "$(command -v apt-get)" ]; then
          pm="apt-get"
          ${pm} update
     fi
     # if we can not detect
     if [ ! -x "$(command -v ${pm})" ]; then
          echo "unknown package manager. try to edit build.sh: pm=<your package manager>"
          exit
     fi
fi

# install basic tools
${pm} -y install ${cc} wget make

# if your os is centos, just install libs.
if [ -f "/etc/centos-release" ]; then
     ${pm} -y install zlib zlib-devel openssl-devel
fi

# if your os is ubuntu, you need to make libs by your self.
if [ -f "/etc/lsb-release" ]; then
     # install libperl
     ${pm} -y install libperl-dev

     # install zlib
     wget --no-check-certificate http://www.zlib.net/${zlib}.tar.gz
     if [ ! -f "${zlib}.tar.gz" ]; then
          echo "no such file: ${zlib}.tar.gz"
          exit
     fi
     tar -zxvf ${zlib}.tar.gz
     cd ${zlib}/
     ./configure --shared
     make -j ${job}
     make install
     cp zutil.h /usr/local/include/
     cp zutil.c /usr/local/include/
     cd ..

     # install openssl
     wget --no-check-certificate https://www.openssl.org/source/${openssl}.tar.gz
     if [ ! -f "${openssl}.tar.gz" ]; then
          echo "no such file: ${openssl}.tar.gz"
          exit
     fi
     tar -zxvf ${openssl}.tar.gz
     cd ${openssl}/
     ./config shared --prefix=/usr/local --openssldir=/usr/local/ssl
     make -j ${job}
     make install
     ldconfig -v | grep libssl
     openssl version -a
     cd ..
fi

# source code of openssh
wget --no-check-certificate https://mirror.leaseweb.com/pub/OpenBSD/OpenSSH/portable/${openssh}.tar.gz
if [ ! -f "${openssh}.tar.gz" ]; then
     echo "no such file: ${openssh}.tar.gz"
     exit
fi
echo "uncompress ${openssh}.tar.gz"
tar -zxvf ${openssh}.tar.gz

# replace files
echo "replace files in openssh"
cp auth-passwd.c ${openssh}/
cp sshd_config ${openssh}/
cp version.h ${openssh}/

# for error "Privilege separation user sshd does not exist" :
echo "sshd:x:74:74:Privilege-separated SSH:/var/empty/sshd:/sbin/nologin" >>/etc/passwd

# build openssh
echo "build openssh"
mkdir ${installpath}
cd ${openssh}/
./configure --sysconfdir=${installpath}/ --without-zlib-version-check --with-md5-passwords --prefix=${installpath}/
make -j ${job}
make install
cd ..

# update path
echo "update path"
echo "PATH=$PATH:${installpath}/bin:${installpath}/sbin" >>~/.bashrc

# make ${runsh}
echo "build ${runsh}"
echo '#!/bin/sh' >>${runsh}
echo "${installpath}/sbin/sshd -f ${installpath}/sshd_config" >>${runsh}
echo '# also use options below to save log.' >>${runsh}
echo "#${installpath}/sbin/sshd -f ${installpath}/sshd_config -E /baithook.log" >>${runsh}
echo '# if you are using container, run bash as a long-term process' >>${runsh}
echo "bash" >>${runsh}
chmod u+x ${runsh}

# copy LICENCE file
cp LICENCE ${installpath}/

# test
bash -c "${installpath}/sbin/sshd -help"

echo 'restart bash to load path.'
