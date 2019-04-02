Name:           rff-ftdi-reset
Version:        0.0.1
Release:        1%{?dist}
Summary:        Utility to reset Synopsys ARC development boards via FTDI chip

Group:          TecAdmin
License:        GPL
URL:            https://github.com/foss-for-synopsys-dwc-arc-processors/rff-ftdi-reset
Source0:        https://github.com/foss-for-synopsys-dwc-arc-processors/rff-ftdi-reset/archive/%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  libftdi-devel
Requires:       libftdi

%description
Utility to reset Synopsys ARC development boards via FTDI chip

%global debug_package %{nil}

%prep
%setup -q

%build
make

%install
install -m 0755 -d %{buildroot}/usr/bin
install -m 0755 rff-reset %{buildroot}/usr/bin/rff-reset

%files
/usr/bin/rff-reset

%changelog
* Mon Apr 01 2019 Alexey Brodkin  0.0.1
  - Initial rpm release
