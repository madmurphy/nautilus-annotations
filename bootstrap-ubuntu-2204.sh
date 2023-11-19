#!/bin/bash

[ ! -z "$DEBUG" ] || set -x

set -ue


function ubuntu_2204_install_deps () {
    echo 'Installing required packages for Ubuntu 22.04 with extreme prejudice...'
    (set -x;
     sudo su -c 'apt-get update && \
	 apt-get install -y \
libgtk-4-dev \
libglib2.0-dev-* \
libgtksourceview-5-dev \
libnautilus-extension-dev \
libadwaita-1-dev')
}


function linux_detected_p () {
    echo 'Checking we are on a Linux...'
    if [[ "linux-gnu" == "${OSTYPE}" ]]; then
	return 0
    else
	return 1
    fi
}


function linux_load_etc_os_release {
    echo 'Loading info from /etc/os-release...'
    if test -f /etc/os-release ; then
	set -a && . /etc/os-release && set +a
    else
	echo 'Did not detect an /etc/os-release. Sorry, cannot proceed!' >&2
	exit 1
    fi
}


function ubuntu_2204_detected_p () {
    echo 'Inspecting for Ubuntu 22.04...'
    linux_load_etc_os_release
    if [[ "jammy" == "${UBUNTU_CODENAME}" ]] ; then
	return 0
    else
	return 1
    fi
}


function main () {
    if linux_detected_p && ubuntu_2204_detected_p ; then
	ubuntu_2204_install_deps
  else
    echo "Sorry but this script currently only works for Ubuntu 22.04 Linux."
  fi
}


main
