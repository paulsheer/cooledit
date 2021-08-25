%define srcversion	4.1.0

Summary: 	Full featured multiple window programmer's text editor
Name: 		cooledit
Icon: 		cooledit.xpm
Version: 	4.1.0
Release: 	2federa
License: 	GPL/BSD
Group: 		Editors
Requires: 	libX11 freetype xorg-x11-fonts man groff-base
BuildRequires:	libX11-devel libXpm freetype-devel

Source: 	ftp://ftp.ibiblio.org/pub/Linux/apps/editors/X/%{name}/%{name}-%{srcversion}.tar.gz
Source1:	%{name}_48x48.xpm
Source2:	%{name}.menu
BuildRoot: 	%_tmppath/%name-%version-%release-root
URL: 		ftp://ftp.ibiblio.org/pub/Linux/apps/editors/X/%{name}/

%description 
Full-featured X Window text editor; multiple edit windows; 3D Motif-ish
look and feel; shift-arrow and mouse text highlighting; column text
highlighting and manipulation; color syntax highlighting for various
sources; buildin Python interpretor for macro program.; interactive
graphical debugger - interface to gdb; key for key undo; macro
recording; regular expression search and replace; pull-down menus; drag
and drop; interactive man page browser; run make and other shell
commands with seamless shell interface; redefine keys with an easy
interactive key learner; full support for proportional fonts;

%prep
%setup -q -n %{name}-%{srcversion}

%build
%configure --program-prefix='' --disable-nls --disable-shared
%make


%install
rm -fr %buildroot

%makeinstall
%find_lang %{name}

# Mandrake menu entries
install -D -m644 %{SOURCE2} $RPM_BUILD_ROOT%{_menudir}/cooledit

# (sb) installed but unpackaged files
rm -f $RPM_BUILD_ROOT/usr/share/locale/locale.alias

%post
%update_menus
%postun
%clean_menus
%clean
rm -fr %buildroot

%files
%defattr (-,root,root)
%doc AUTHORS BUGS COPYRIGHT COPYRIGHT.gpl INSTALL
%doc NEWS TODO VERSION
%doc cooledit.spec
%doc cooledit_16x16.xpm cooledit_32x32.xpm cooledit_48x48.xpm
%dir %_datadir/cooledit/
%_datadir/cooledit/*
%_bindir/*
%_mandir/man1/*
%_menudir/%{name}

%files -n %lib_name
%defattr(-,root,root)

%files -n %lib_name-devel
%defattr(-,root,root)


