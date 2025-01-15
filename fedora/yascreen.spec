Name:           yascreen
Version:        1.99
Release:        3%{?dist}
Summary:        Yet Another Screen Library (lib(n)curses alternative)

License:        LGPL-3.0-only
URL:            https://github.com/bbonev/yascreen/
Source0:        %{url}releases/download/v%{version}/yascreen-%{version}.tar.xz
Source1:        %{url}releases/download/v%{version}/yascreen-%{version}.tar.xz.asc
Source2:        https://raw.githubusercontent.com/bbonev/yascreen/master/debian/upstream/signing-key.asc

BuildRequires:  gcc
BuildRequires:  gnupg2
BuildRequires:  make

%description
lib(n)curses alternative oriented towards modern terminals.

Suitable for developing terminal applications or daemons with
telnet access and terminal support.

Main features

 * small footprint
 * does not have external dependencies
 * allows both internal and external event loop
 * allows stdin/stdout or external input/output (can work over socket)
 * supports basic set of telnet sequences, making it suitable for built-in
   terminal interfaces for daemons
 * supports a limited set of input keystroke sequences
 * fully Unicode compatible (parts of this depend on wcwidth in libc)
 * supports utf8 verification of input
 * relies only on a limited subset of ANSI/xterm ESC sequences, making it
   compatible with mostly all modern terminals (inspired by linenoise)
 * there is no curses API and ancient terminal compatibility, hence less bloat
 * clean API with opaque private data, usable from C/C++

%package devel
Summary:        Development files for yascreen
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel

This package contains the header files and libraries needed to
compile applications or shared objects that use yascreen.

%global _hardened_build 1

%prep
%{gpgverify} --keyring='%{SOURCE2}' --signature='%{SOURCE1}' --data='%{SOURCE0}'
%autosetup

%build
%set_build_flags
export CFLAGS="${RPM_OPT_FLAGS}"
NO_FLTO=1 DEBUG="" %make_build PREFIX=%{_prefix} LIBDIR=/%{_lib}/

%install
%make_install INSTALL+=" --strip-program=true" PREFIX=%{_prefix} LIBDIR=/%{_lib}/
# allow debug info to be generated
chmod +x $RPM_BUILD_ROOT%{_libdir}/libyascreen.so.0.0.0
# remove unpackaged static library
rm -f $RPM_BUILD_ROOT%{_libdir}/libyascreen.a

%files
%license LICENSE
%doc README.md
%{_libdir}/*.so.*

%files devel
%{_libdir}/*.so
%{_libdir}/pkgconfig/yascreen.pc
%{_mandir}/man3/yascreen.3*
%{_includedir}/yascreen.h

%changelog
* Sat Jul 20 2024 Fedora Release Engineering <releng@fedoraproject.org> - 1.99-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_41_Mass_Rebuild

* Sat Jan 27 2024 Fedora Release Engineering <releng@fedoraproject.org> - 1.99-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_40_Mass_Rebuild

* Fri Nov 10 2023 Boian Bonev <bbonev@ipacct.com> - 1.99-1
- Update to latest ver 1.99

* Sat Jul 22 2023 Fedora Release Engineering <releng@fedoraproject.org> - 1.97-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_39_Mass_Rebuild

* Fri Feb 17 2023 Boian Bonev <bbonev@ipacct.com> - 1.97-1
- Update to latest ver 1.97

* Tue Feb 14 2023 Boian Bonev <bbonev@ipacct.com> - 1.96-2
- SPDX migration

* Sun Feb 5 2023 Boian Bonev <bbonev@ipacct.com> - 1.96-1
- Update to latest ver 1.96

* Sat Jan 21 2023 Fedora Release Engineering <releng@fedoraproject.org> - 1.92-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_38_Mass_Rebuild

* Mon Jan 2 2023 Boian Bonev <bbonev@ipacct.com> - 1.92-1
- Update to latest ver 1.92

* Sat Jul 23 2022 Fedora Release Engineering <releng@fedoraproject.org> - 1.86-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_37_Mass_Rebuild

* Sat Jan 22 2022 Fedora Release Engineering <releng@fedoraproject.org> - 1.86-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_36_Mass_Rebuild

* Tue Sep 7 2021 Boian Bonev <bbonev@ipacct.com> - 1.86-1
- Update to latest ver 1.86

* Fri Jul 23 2021 Fedora Release Engineering <releng@fedoraproject.org> - 1.85-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_35_Mass_Rebuild

* Thu Jan 28 2021 Fedora Release Engineering <releng@fedoraproject.org> - 1.85-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_34_Mass_Rebuild

* Sun Dec 6 2020 Boian Bonev <bbonev@ipacct.com> - 1.85-1
- Initial packaging for Fedora
