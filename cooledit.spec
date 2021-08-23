%define srcversion	4.0.0
%define lib_major		1
%define lib_name %mklibname Cw %lib_major
%define lib_name_orig libCw

Summary: 	Full featured multiple window programmer's text editor
Name: 		cooledit
Icon: 		cooledit.xpm
Version: 	4.0.0
Release: 	1mdk
License: 	GPL
Group: 		Editors
Requires: 	python, pythonlib, %lib_name = %version-%release
BuildRequires:	XFree86-devel xpm-devel

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

%package -n %lib_name
Group:          Editors
Summary:        Shared library files for cooledit

%description -n %lib_name
Shared library files for cooledit.

%package -n %lib_name-devel
Group:          Development/C
Summary:        Development files for cooledit
PreReq:       	%name = %version-%release, %lib_name = %version-%release
Provides:       %lib_name_orig-devel

%description -n %lib_name-devel
Files for development from the cooledit package.


%prep
%setup -q -n %{name}-%{srcversion}
%patch0 -p1 -b .broke

%build
%configure --program-prefix=''
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
#(sb) no longer exists?
#%{_libdir}/coolicon/modify-xinitrc
%update_menus

# FIXME undo 'modify-xinitrc' above in postun step
%postun
%clean_menus

%post -n %lib_name -p /sbin/ldconfig

%postun -n %lib_name -p /sbin/ldconfig

%clean
rm -fr %buildroot

%files -f %{name}.lang
%defattr (-,root,root)
%doc ABOUT-NLS AUTHORS BUGS COPYING FAQ INSTALL INTERNATIONAL HINTS
%doc NEWS README TODO VERSION ChangeLog
%doc cooledit.lsm coolman.lsm cooledit.spec
%doc cooledit_16x16.xpm cooledit_32x32.xpm rxvt/README.rxvt
#
%dir %_datadir/cooledit/
%_datadir/cooledit/*
#
%_bindir/*
%_mandir/man1/*
%_menudir/%{name}

%files -n %lib_name
%defattr(-,root,root)
%_libdir/*.so.*

%files -n %lib_name-devel
%defattr(-,root,root)
%_libdir/*.a
%_libdir/*.so
%_libdir/*.la


%changelog
* Fri Oct 15 2004 Paul Sheer 3.17.9
- Applied mdk fixes to virgin package

* Sat Sep 04 2004 Stew Benedict <sbenedict@mandrakesoft.com> 3.17.8-1mdk
- 3.17.8, patch broken configure.in (patch0 - thx Christiaan)

* Fri Aug 08 2003 Stew Benedict <sbenedict@mandrakesoft.com> 3.17.7-6mdk
- rebuild for new python

* Tue Aug 05 2003 Stew Benedict <sbenedict@mandrakesoft.com> 3.17.7-5mdk
- fix botched requires

* Tue Aug 05 2003 Stew Benedict <sbenedict@mandrakesoft.com> 3.17.7-4mdk
- mklibname

* Tue Jul 22 2003 Per Øyvind Karlsen <peroyvind@sintrax.net> 3.17.7-3mdk
- rebuild

* Mon Dec 30 2002 Stew Benedict <sbenedict@mandrakesoft.com> 3.17.7-2mdk
- rebuild for new glibc/rpm, installed but unpackaged files

* Fri Oct 25 2002 Stew Benedict <sbenedict@mandrakesoft.com> 3.17.7-1mdk
- new version, shared files move to default datadir
- --program-prefix='' to fix PPC build, drop patch1

* Wed Jan 16 2002 David BAUDENS <baudens@mandrakesoft.com> 3.17.5-3mdk
- Clean after build

* Wed Jan 09 2002 David BAUDENS <baudens@mandrakesoft.com> 3.17.5-2mdk
- Fix menu entry (icon)

* Fri Nov  2 2001 Jeff Garzik <jgarzik@mandrakesoft.com> 3.17.5-1mdk
- new version.
- update URL.
- libification: create packages libCw and libCw-devel

* Tue Sep 11 2001 David BAUDENS <baudens@mandrakesoft.com> 3.17.4-2mdk
- Use standard (from a size point of view) icons
- Add missing files

* Sun Feb 11 2001 Jeff Garzik <jgarzik@mandrakesoft.com> 3.17.4-1mdk
- New version 3.17.4
- Use 'make' macro

* Tue Sep 26 2000 Jeff Garzik <jgarzik@mandrakesoft.com> 3.17.2-2mdk
- Store coolicon.config in sysconfdir

* Thu Sep 07 2000 Geoffrey Lee <snailtalk@mandrakesoft.com> 3.17.2-1mdk
- s|3.17.1|3.17.2|; 

* Wed Sep  6 2000 Guillaume Cottenceau <gc@mandrakesoft.com> 3.17.1-4mdk
- removed Requires to Glide_V3-devel

* Tue Sep 05 2000 Florin Grad <florin@mandrakesoft.com> 3.17.1-3mdk
- fixing the menu entry and adding Requires Glide_V3-devel

* Tue Sep 05 2000 Florin Grad <florin@mandrakesoft.com> 3.17.1-2mdk
- adding some more macros and fixing the modify-xinitrc and adding noreplace

* Wed Aug 30 2000 Geoffrey Lee <snailtalk@mandrakesoft.com> 3.17.1-1mdk
- s|3.17.0|3.17.1|;

* Wed Aug 23 2000 Denis Havlik <denis@mandrakesoft.com> 3.17.0-2mdk
- add find_lang
- add mini and large icons

* Thu Aug 17 2000 Geoffrey Lee <snailtalk@mandrakesoft.com> 3.17.0-1mdk
- build a new and shiny version.

* Thu Aug 17 2000 Geoffrey Lee <snailtalk@mandrakesoft.com> 3.17.0-1mdk
- build a new and shiny version.

* Mon Aug 07 2000 Frederic Lepied <flepied@mandrakesoft.com> 3.15.4-3mdk
- automatically added BuildRequires

* Sat Jul 29 2000 Geoffrey Lee <snailtalk@mandrakesoft.com> 3.15.4-2mdk
- macrosificaigtons
- rebuild for the BM

* Thu Jul 06 2000 Christian Zoffoli <czoffoli@linux-mandrake.com> 3.15.4-1mdk
- new version 3.15.4-1mdk
- some changes in spec

* Tue Jun 20 2000 Chmouel Boudjnah <chmouel@mandrakesoft.com> 3.14.0-5mdk
- User makeinstal macros.

* Wed May 24 2000 Chmouel Boudjnah <chmouel@mandrakesoft.com> 3.14.0-4mdk
- use % configure.
- Clean up specs.

* Fri Apr 28 2000 Vincent Saugey <vince@mandrakesoft.com> 3.14.0-3mdk
- Clean spec file
- Install icon in /usr/share/icon

* Thu Apr 20 2000 Jeff Garzik <jgarzik@mandrakesoft.com> 3.14.0-2mdk
- Rebuild tarball with sane directory permissions (thanks fredl)

* Mon Apr 10 2000 Jeff Garzik <jgarzik@mandrakesoft.com> 3.14.0-1mdk
- version 3.14.0
- menu entry
- BuildRoot update
- URL update
- Redo filelist to be based on prefix
- Correct bug which prevented proper CFLAGS from showing up
- .a library removed, unnecessary

* Wed Dec 29 1999 Lenny Cartier <lenny@mandrakesoft.com>
- only add the homepage URL

* Wed Dec 29 1999 Lenny Cartier <lenny@mandrakesoft.com>
- new in contribs
- used and modify specfile from RH contribs
- added a new filelist
- bzip man pages
- clean specfile
