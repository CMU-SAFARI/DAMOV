#!/bin/bash
apt-get -y install build-essential
apt-get -y install scons
apt-get -y install automake
apt-get -y install autoconf
apt-get -y install m4
apt-get -y install perl
apt-get -y install flex
apt-get -y install bison
apt-get -y install byacc
#apt-get -y install libconfig-dev
#apt-get -y install libconfig++-dev
apt-get -y install libhdf5-dev
apt-get -y install libelf-dev

ln -s /usr/include/asm-generic /usr/include/asm

if ! which lrztar > /dev/null; then
  echo "Installing the lrztar tool ..."
  apt-get -y install tar git zlib1g-dev libbz2-dev liblzo2-dev liblz4-dev coreutils nasm libtool
  cd ~ && git clone -v https://github.com/ckolivas/lrzip.git && cd -
  cd ~/lrzip && ./autogen.sh && ./configure && make -j `nproc` && make install && cd -
else
  echo "lrztar is already installed."
fi
