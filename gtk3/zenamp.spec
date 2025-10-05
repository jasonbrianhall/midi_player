Name:           zenamp
Version:        @VERSION@
Release:        1%{?dist}
Summary:        ZenAmp Audio Player

License:        MIT
URL:            https://github.com/jasonbrianhall/midi_playr
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  SDL2-devel
BuildRequires:  SDL2_mixer-devel
BuildRequires:  gtk3-devel
BuildRequires:  libvorbis-devel
BuildRequires:  opusfile-devel
BuildRequires:  taglib-devel
BuildRequires:  flac-devel
BuildRequires:  ffmpeg-devel

Requires:       SDL2
Requires:       SDL2_mixer
Requires:       gtk3
Requires:       libvorbis
Requires:       opusfile
Requires:       taglib
Requires:       flac-libs
Requires:       ffmpeg-libs

# Disable automatic debug package generation
%global debug_package %{nil}

%description
ZenAmp is a feature-rich audio player supporting multiple formats including
MP3, OGG, Opus, FLAC, M4A, MIDI, CD+G, and more. It includes visualizations,
equalizer, and various playback features.

%prep
%setup -q

%build
make linux

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

%files
%{_bindir}/%{name}
%doc %{_docdir}/%{name}/README.md
%license %{_docdir}/%{name}/LICENSE
%{_datadir}/icons/hicolor/48x48/apps/%{name}.png

%changelog
* Sun Oct 05 2025 Jason Hall <jasonbrianhall@yahoo.com>
- Initial RPM release
