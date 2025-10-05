Name:           zenamp
Version:        @VERSION@
Release:        1%{?dist}
Summary:        ZenAmp Audio Player

License:        MIT
URL:            https://github.com/jasonbrianhall/midi_player
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
BuildRequires:  desktop-file-utils
BuildRequires:  ImageMagick

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

# Convert icon.png to 48x48 if it exists
if [ -f icon.png ]; then
    convert icon.png -resize 48x48 %{name}.png
fi

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

# Install desktop file
mkdir -p %{buildroot}%{_datadir}/applications
cat > %{buildroot}%{_datadir}/applications/%{name}.desktop << EOF
[Desktop Entry]
Name=ZenAmp
GenericName=Audio Player
Comment=Minimalist music player with visualizations and OPL3 MIDI synthesis
Exec=%{name} %F
Icon=%{name}
Terminal=false
Type=Application
Categories=AudioVideo;Audio;Player;
MimeType=audio/mpeg;audio/x-mp3;audio/ogg;audio/x-vorbis+ogg;audio/flac;audio/x-flac;audio/x-wav;audio/midi;audio/x-midi;audio/opus;audio/mp4;audio/x-m4a;audio/x-aiff;audio/x-ms-wma;application/x-cdg;audio/x-mpegurl;
Keywords=music;audio;player;midi;ogg;mp3;flac;opus;
EOF

# Validate desktop file
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop

%files
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%doc %{_docdir}/%{name}/README.md
%license %{_docdir}/%{name}/LICENSE
%{_datadir}/icons/hicolor/48x48/apps/%{name}.png

%changelog
* Sun Oct 05 2025 Jason Hall <jasonbrianhall@yahoo.com>
- Initial RPM release
- Added desktop entry for application menu integration
