%global kversion 3.2.2

Name: kernel
Summary: The Linux Kernel
Version: 3.2
Release: 1%{?dist}
License: GPL
Group: System Environment/Kernel
Vendor: The Linux Community
URL: http://www.kernel.org

Source0: linux-%{version}.tar.gz
Source1: linux-%{version}-patches.tar.gz

Source10: kconfig.py
Source11: Makefile.config
Source20: config-generic
Source21: config-x86_32-generic
Source22: config-x86_64-generic

# __PATCHFILE_TEMPLATE__

BuildRoot: %{_tmppath}/%{name}-%{PACKAGE_VERSION}-root
Provides: kernel-drm kernel-%{kversion}
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
patch_command='patch -p1 -F1 -s'

ApplyNoCheckPatch()
{
  local patch=$1
  shift
  case "$patch" in
    *.bz2) bunzip2 < "$RPM_SOURCE_DIR/$patch" | $patch_command ${1+"$@"} ;;
    *.gz) gunzip < "$RPM_SOURCE_DIR/$patch" | $patch_command ${1+"$@"} ;;
    *) $patch_command ${1+"$@"} < $patch ;;
  esac
}

ApplyPatch()
{
  local patch=$1
  shift
  if [ ! -f $RPM_SOURCE_DIR/$patch ]; then
    exit 1
  fi
  if ! egrep "^Patch[0-9]+: $patch\$" %{_specdir}/${RPM_PACKAGE_NAME%%%%%{?variant}}.spec ; then
    if [ "${patch:0:10}" != "patch-2.6." ] ; then
      echo "ERROR: Patch  $patch  not listed as a source patch in specfile"
      exit 1
    fi
  fi 2>/dev/null
  case "$patch" in
    *.bz2) bunzip2 < "$RPM_SOURCE_DIR/$patch" | $patch_command ${1+"$@"} ;;
    *.gz) gunzip < "$RPM_SOURCE_DIR/$patch" | $patch_command ${1+"$@"} ;;
    *) $patch_command ${1+"$@"} < "$RPM_SOURCE_DIR/$patch" ;;
  esac
}

%setup -q -a1 -n linux-%{version}
patch_list=$(basename %{SOURCE1})
patch_list=${patch_list%.*}.list
for p in `cat $patch_list` ; do
  ApplyNoCheckPatch ${p}
done

# __APPLYFILE_TEMPLATE__

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
cp $KBUILD_IMAGE $RPM_BUILD_ROOT/boot/vmlinuz-%{kversion}

make %{?_smp_mflags} INSTALL_HDR_PATH=$RPM_BUILD_ROOT/usr headers_install
cp System.map $RPM_BUILD_ROOT/boot/System.map-%{kversion}
cp .config $RPM_BUILD_ROOT/boot/config-%{kversion}

cp vmlinux vmlinux.orig
bzip2 -9 vmlinux
mv vmlinux.bz2 $RPM_BUILD_ROOT/boot/vmlinux-%{kversion}.bz2
mv vmlinux.orig vmlinux

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr (-, root, root)
%dir /lib/modules
/lib/modules/%{kversion}
/lib/firmware
/boot/*

%files headers
%defattr (-, root, root)
/usr/include

%changelog
* Thu Jan 19 2012 Cristian Gafton <gafton@amazon.com> - 3.2.1-1
- create template spec file

