Summary: Molecular viewer
Name: mdview
%define version 3.64.0
Version: %{version}
Release: 1
Group: Applications/Graphics
Source: mdview-%{version}.tar.gz

Copyright: BABA Akinori
Packager: BABA Akinori <bar@aqua.chem.nagoya-u.ac.jp>
URL: http://www.chem.nagoya-u.ac.jp/bar/mdview/
BuildRoot: /var/tmp/%{name}-root
Requires: gtk+ >= 1.2.0

%description
Molecular viewer

%prep
%setup -n mdview-%{version}

rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/man/man1
mkdir -p $RPM_BUILD_ROOT/usr/man/ja/man1
mkdir -p $RPM_BUILD_ROOT/usr/share/mdview

%build
./configure --prefix=/usr
make CFLAGS="$RPM_OPT_FLAGS" LDFLAGS="$RPM_OPT_FLAGS"
strip mdview

%install
make prefix=$RPM_BUILD_ROOT/usr install
install -m 0755 mdview.jp.1 $RPM_BUILD_ROOT/usr/man/ja/man1/mdview.1

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL NEWS README
/usr/bin/mdview
/usr/man/man1/mdview.1.gz
/usr/man/ja/man1/mdview.1.gz
/usr/share/mdview/mdviewrc
