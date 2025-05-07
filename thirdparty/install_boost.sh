#!/bin/sh

sudo apt-get install zlib1g-dev libicu-dev

wget https://archives.boost.io/release/1.73.0/source/boost_1_73_0.tar.gz
tar -zxvf boost_1_73_0.tar.gz

cd boost_1_73_0/
sh bootstrap.sh --prefix=/usr/local/ --libdir=/usr/local/lib/

sudo ./b2 install
sudo ldconfig
