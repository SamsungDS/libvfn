#!/bin/bash

apt-get -y update
apt-get -y install build-essential python3-pip

pip3 install --upgrade pip
pip3 install meson ninja
