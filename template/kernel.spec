Name: kernel
Summary: The Linux Kernel
Version: 3.2.1
Release: 1%{?dist}
License: GPL
Group: System Environment/Kernel
Vendor: The Linux Community
URL: http://www.kernel.org
Source: linux-3.2.1.tar.gz

Source10: kconfig.py
Source11: Makefile.config
Source20: config-generic
Source21: config-x86_32-generic
Source22: config-x86_64-generic

BuildRoot: %{_tmppath}/%{name}-%{PACKAGE_VERSION}-root
Provides: kernel-drm kernel-3.2.1
%define __spec_install_post /usr/lib/rpm/brp-compress || :
%define debug_package %{nil}

%description
The Linux Kernel, the operating system core itself

%package headers
Summary: Header files for the Linux kernel for use by glibc
Group: Development/System
Obsoletes: kernel-headers
Provides: kernel-headers = %{version}
%description headers
Kernel-headers includes the C header files that specify the interface
between the Linux kernel and userspace libraries and programs.  The
header files define structures and constants that are needed for
building most standard programs and are also needed for rebuilding the
glibc package.

%prep
%setup -q -n linux-%{version}
# Drop some necessary files from the source dir into the buildroot
cp $RPM_SOURCE_DIR/config-*generic .
cp %{SOURCE10} .
# Dynamically generate kernel .config files from config-* files
make -f %{SOURCE11} VERSION=%{version} config
cp kernel-%{version}-%{_target_cpu}.config .config

%build
make clean && make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
export KBUILD_IMAGE=arch/x86/boot/bzImage

mkdir -p $RPM_BUILD_ROOT/boot $RPM_BUILD_ROOT/lib/modules
mkdir -p $RPM_BUILD_ROOT/lib/firmware

INSTALL_MOD_PATH=$RPM_BUILD_ROOT make %{?_smp_mflags} KBUILD_SRC= modules_install
cp $KBUILD_IMAGE $RPM_BUILD_ROOT/boot/vmlinuz-3.2.1

make %{?_smp_mflags} INSTALL_HDR_PATH=$RPM_BUILD_ROOT/usr headers_install
cp System.map $RPM_BUILD_ROOT/boot/System.map-3.2.1
cp .config $RPM_BUILD_ROOT/boot/config-3.2.1

cp vmlinux vmlinux.orig
bzip2 -9 vmlinux
mv vmlinux.bz2 $RPM_BUILD_ROOT/boot/vmlinux-3.2.1.bz2
mv vmlinux.orig vmlinux

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr (-, root, root)
%dir /lib/modules
/lib/modules/3.2.1
/lib/firmware
/boot/*

%files headers
%defattr (-, root, root)
/usr/include

%changelog
* Thu Jan 19 2012 Cristian Gafton <gafton@amazon.com> - 3.2.1-1
- create template spec file

