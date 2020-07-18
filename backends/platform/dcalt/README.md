# Alternate ScummVM backend for Dreamcast

This is an unofficial Dreamcast backend for ScummVM that uses KallistiOS for
Dreamcast support.  Its purpose is to provide a standard backend for the
Dreamcast, so the UI, themes, virtual keyboard, configuration, saves, etc. work
the same way they do in other standard backends.  It also provides hardware
support for various after-market Dreamcast mods.

This is a work in progress.  Many backend features are still unimplemented or
untested.  Please do not use this with any filesystems or VMUs you care about
until more testing has been done.  It is probably also a good idea to take
frequent backups of saved games for now.

   * [Dreamcast hardware support](#dreamcast-hardware-support)
   * [Usage](#usage)
      * [Notes on burning a CD](#notes-on-burning-a-cd)
      * [Burning a CD from a release](#burning-a-cd-from-a-release)
         * [Prepare the cd directory](#prepare-the-cd-directory)
         * [Burn a CD (Unix)](#burn-a-cd-unix)
         * [Burn a CD (Windows)](#burn-a-cd-windows)
      * [Controller button mapping](#controller-button-mapping)
      * [Saved games](#saved-games)
      * [scummvm.ini paths](#scummvmini-paths)
      * [VGA options](#vga-options)
   * [Building from source](#building-from-source)
      * [Get the (custom) KallistiOS source](#get-the-custom-kallistios-source)
      * [Build the toolchain](#build-the-toolchain)
      * [Configure KallistiOS/environ.sh](#configure-kallistiosenvironsh)
      * [Build KallistiOS](#build-kallistios)
      * [Build zlib](#build-zlib)
      * [Build FLAC](#build-flac)
      * [Build libjpeg](#build-libjpeg)
      * [Build libpng](#build-libpng)
      * [Build libmad](#build-libmad)
      * [Build freetype](#build-freetype)
      * [Build makeip](#build-makeip)
      * [Build ScummVM](#build-scummvm)

## Dreamcast hardware support

This backend supports unmodified Dreamcasts - it can load games from CD and
store configuration and save files on VMUs.

It also supports some additional hardware and modifications:

* An SD card attached to the SCIF port can be used for game data and for
  configuration and save files.

* An ATA drive attached to the G1 bus can be used for game data and for
  configuration and save files.

* scummvm.ini and saved games can be written to VMU, SD card, or ATA drive
  (see "Saved games" and "scummvm.ini paths" below).

* It can output MIDI over a Dreamcast MIDI Interface Cable or the AICA MIDI
  out port so an external synth, like an MT-32, can be used.  The AICA MIDI
  signals are available on the G2 port, so a [MIDI connector can be added to
  a Dreamcast modem](
  https://tsowell.github.io/2020/07/09/dreamcast-aici-midi.html).

* It can change VGA modes to output a 320x200 image so that the display can
  handle the vertical scaling.  (see the "VGA options" section below).

* It supports Dreamcasts with either 16MB or [32MB system RAM](
  https://tsowell.github.io/2020/06/21/dreamcast-32mb-ram-upgrade.html)

Talkies run okay from SD card, but there can be slight delays whenever samples
are loaded.  There are no noticeable delays from a CF card connected via ATA.

KallistiOS only updates the first superblock on a FAT filesystem, so it's best
to use "mkfs.vfat -f 1" when creating a FAT filesystem for use with it.

## Usage

### Notes on burning a CD

When you add a game through ScummVM's launcher, it has to load plugins
one-at-a-time to find one that will work with the game.  If you're loading the
plugins from the CD, this can take a long time - like 2 minutes.  You can avoid
this by keeping only a small number of plugins on your CD image or by putting
the plugins on an SD card or ATA drive.  You can edit pluginspath in
scummvm.ini or in the Options menu to tell ScummVM to look for plugins in an
alternate path.

The official Dreamcast backend has a great mechanism for caching plugins that
allows you to swap the ScummVM disc for a game disc and load games from it.
The only way you can do anything like that with this backend is by compiling
ScummVM with a static plugin or by storing the plugins on an SD card or ATA
drive.

### Burning a CD from a release

#### Prepare the cd directory

 1. Download and extract [the latest release tarball](https://github.com/tsowell/scummvm-dreamcast/releases/latest).
    It contains a dcalt-dist directory.  Some of the contents are:

    * dcalt-dist/README.md: this file
    * dcalt-dist/scummvm.elf: the main ScummVM binary (ELF)
    * dcalt-dist/scummvm.bin: the main ScummVM binary (unscrambled)
    * dcalt-dist/cd: files for the root directory of a CD
    * dcalt-dist/cd/scummvm.bin: the main ScummVM binary (scrambled)
    * dcalt-dist/cd/plugins: plugins for each ScummVM engine
    * dcalt-dist/IP.BIN: [CD bootstrap](https://mc.pp.se/dc/ip.bin.html)

 2. If you want to include any game files on your CD, add them to dcalt-dist/cd.

 3. Remove any unwanted plugins from dcalt-dist/cd/plugins.

#### Burn a CD (Unix)

 1. Take a look at dcalt-dist/burn.sh before running it.  You might need to
 change "dev=/dev/cdrom" to your CD drive's device path.

 2. Run dcalt-dist/burn.sh:

        $ cd dcalt-dist
        $ ./burn.sh

#### Burn a CD (Windows)

 1. Download mkisofs.exe and cdi4dc.exe to the dcalt-dist directory.
    You can use these links to the [DreamSDK](https://www.dreamsdk.org/) repo:

    * [mkisofs.exe](https://github.com/dreamsdk/system-objects/raw/master/msys/opt/dreamsdk/helpers/mkisofs.exe)
    * [cdi4dc.exe](https://github.com/dreamsdk/system-objects/raw/master/msys/opt/dreamsdk/helpers/cdi4dc.exe)

 2. Create a CDI file:

        > cd dcalt-dist
        > mkisofs -C 0,11702 -l -G IP.BIN -o scummvm.iso cd
        > cdi4dc scummvm.iso scummvm.cdi

 3. Use your favorite cd-burning application to burn scummvm.cdi to a CD.

### Controller button mapping

The button mapping is based mostly on the SDL backend button mapping.

* A: Left mouse button
* Right trigger + A: Virtual keyboard
* B: Right mouse button
* Right trigger + B: Predictive input dialog
* X: Period
* Right trigger + X: Space
* Y: Escape
* Right trigger + Y: Return
* Start: ScummVM in game menu
* Left trigger: Game menu
* Right trigger: Shift
* Up/Down/Left/Right: Numeric keyboard 8, 2, 4, 6
* Right trigger + Up/Down/Left/Right: Numeric keyboard 9, 1, 7, 3

### Saved games

The default save path is /vmu/a1, the path for the VMU in the first socket of
controller port A.  You can use a different VMU by changing the path in the
launcher screen.  To save to other storage, change the path to a directory
under /sd or /ata.

Keep in mind that VMU storage is limited - The Secret of Monkey Island, for
example, needs ~22 blocks per saved game out of the VMU's 200 blocks - so a VMU
can only hold a handful of saved games.

VMU saved games created in this backend are not compatible with the official
Dreamcast backend, and vice versa.  backends/platform/saves.h has more
information on how long filename support is implemented for VMUs.

### scummvm.ini paths

ScummVM tries to read scummvm.ini from the following locations:

Reading priority:
 1. /sd/scummvm/scummvm.ini
 2. /ata/scummvm/scummvm.ini
 3. /vmu/*/scummvm.ini
 4. /cd/scummvm/scummvm.ini

Writing priority:
 1. The path from which scummvm.ini was originally read (if writable)
 2. /sd/scummvm/scummvm.ini
 3. /ata/scummvm/scummvm.ini
 4. /vmu/*/scummvm.ini

It prefers SD card and ATA because they can be easily edited on a computer
later.  SD card is the highest priority because it is (probably) the easiest of
the two to physically remove.

### VGA options

The backend has some custom options for controlling VGA output.

Please check my work before using any of these with a CRT monitor!  Certain
very old CRTs can be permanently damaged by bad video timing.

The default graphics mode is "Scale to 320x240 when aspect ratio correction is
enabled" which performs aspect ratio correction by scaling 320x200 graphics to
320x240.  If the graphics mode is set to "Output 320x200 when aspect ratio
correction is enabled", aspect mode correction enables a 320x200 video signal
for 320x200 graphics.  This allows the display (preferably a CRT) to do the
vertical scaling instead of doing it on the Dreamcast.

If dcalt_vga_25175 is enabled, the Dreamcast will output VGA with standard
video timing.  This requires a hardware modification to clock the GPU at 50.350
MHz.  This is useful for driving older monitors that don't work with the
Dreamcast's unusual video timing.

If dcalt_vga_polarity is enabled, the Dreamcast will output a positive vsync
signal when outputing a 320x200 signal.  The Dreamcast's RAMDAC only supports
negative polarity, so this will only display an image on Dreamcasts that have
been modified to force the RAMDAC vsync negative.  This is useful for driving
older monitors that depend on sync polarity to determine video resolution.

## Building from source

### Get the (custom) KallistiOS source

    $ git clone "https://github.com/tsowell/KallistiOS-scummvm" KallistiOS
    $ git clone "https://github.com/tsowell/kos-ports-scummvm" kos-ports

From here on out, the cloned KallistiOS repository will be referred to as
$KALLISTIOS.

### Build the toolchain

KallistiOS comes with scripts for building a Dreamcast toolchain.

By default the toolchain is installed under /opt/toolchains/dc.  Edit sh_prefix
and arm_prefix in KallistiOS/utils/dc-chain/Makefile if you want to use a
different path.  You may also want to configure the makejobs variable.

    $ cd $KALLISTIOS/utils/dc-chain
    $ ./download.sh
    $ ./unpack.sh
    $ make

### Configure KallistiOS/environ.sh

    $ cd $KALLISTIOS
    $ cp doc/environ.sh.sample environ.sh

Make these changes in environ.sh:

* Set KOS_BASE to the KallistiOS source path
* Set KOS_CC_BASE to match sh_prefix in utils/dc-chain/Makefile
* Set KOS_ARM_BASE to match arm_prefix in utils/dc-chain/Makefile

### Build KallistiOS

    $ cd $KALLISTIOS
    $ . environ.sh
    $ make

### Build zlib

    $ cd $KALLISTIOS
    $ . environ.sh
    $ cd ../kos-ports/zlib
    $ make install

### Build FLAC

    $ cd $KALLISTIOS
    $ . environ.sh
    $ cd ../kos-ports/flac
    $ make install

### Build libjpeg

    $ cd $KALLISTIOS
    $ . environ.sh
    $ cd ../kos-ports/libjpeg
    $ make install

### Build libpng

    $ cd $KALLISTIOS
    $ . environ.sh
    $ cd ../kos-ports/libpng
    $ make install

### Build libmad

    $ cd $KALLISTIOS
    $ . environ.sh
    $ cd ../kos-ports/libmad
    $ make install

### Build freetype

    $ cd $KALLISTIOS
    $ . environ.sh
    $ cd ../kos-ports/freetype
    $ make install

### Build makeip

    $ mkdir makeip
    $ cd makeip
    $ curl -O "https://mc.pp.se/dc/files/makeip.tar.gz"
    $ tar xvf makeip.tar.gz
    $ gcc makeip.c -o makeip

### Build ScummVM

The ScummVM build system expects to find the makeip binary in your path.  If it
isn't, pass the location to make in the MAKEIP variable or change the MAKEIP
variable in backends/platform/dcalt/dcalt.mk.

    $ git clone "https://github.com/tsowell/scummvm-dreamcast"
    $ cd scummvm-dreamcast
    $ . $KALLISTIOS/environ.sh
    $ ./configure --backend=dcalt --host=dreamcast-alt --enable-plugins \
       --default-dynamic --disable-mt32emu --disable-engines=wintermute
    $ make dcalt-dist

This produces CD files in dcalt-dist/cd and IP.BIN in dcalt-dist.
