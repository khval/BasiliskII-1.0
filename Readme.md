# BasiliskII 1.0 for AmigaOS4.x

Date: December-26-2020

This is old version of Basilisk II, I know there are newer more update versions around on GITHUB.
This version focusses on AmigaOS4.1, so is designed around AmigaOS4.1 native graphic feature.

Please read the manual, if you like to mess around with thw configuration files, (you should not need to)

# Recommended hardware specifications 

680x0 interpreted emulation does requires a lot of resources, sadly we do not support JIT (Even if it’s possible on EUAE).
No one has investigated how to integrated it, and the CPU emulation is most complicated part.
We can sadly not expose Petunia (the AmigaOS4.1 integrated JIT) to Basilisk, MacOS needs some memory maps
used by AmigaOS. It was suggesting using MMU to trick it, but not sure how too. Petunia has also limitations, 
it might not be as advanced as it needs to be.

A 1.8GHz PowerPC will give a decent speed for most things.

If you have 800 Mhz G4, the emulation speed will not feel great.

# A few tips

* If you are using Fullscreen, make sure to enable "Screen Lock", if not basilisk II might crash.

* Emulated CPU task priority, range from 0 to -128, the CPU emulation has a tendency to hog the CPU resources,
as a result AmigaOS run into stall states and can completly lock up, setting task priority of -1 for active window
is recommended, if above 1, critical tasks like eventhandlers and other system critical task,
will be starved of CPU resources. (This is way it's limited to max 0.)

* When creating Mac partitions in media Tools box, its recommended that the partition is not above
4Gbytes, if your emulating MacOS7.5.5, if the partition is too big you will get errors, when tying to
copy files to it, under MacOS. If you have lot of space to share with Basilisk create many partitions instead.
You are not supposed to be able to mount the partitions under AmigaOS, so disable automount in media tool box (AmigaOS4.1).
(To be on the safe side use less then 2Gbytes.)

* When selecting partiton in Basilisk II.
DO NOT use Amiga partitions, DH0, DH1, if you do this will be overwritten.
Create new partitions, I will suggest calling this partitions device names MAC0, MAC1, etc.  

* For compressed cd iso images it’s best decompress it on the Amiga side,
you can mount the CD ISO image on the Amiga side, by using diskimagegui / diskimage.device, 
so you don’t need to transferee the files using HFS.

* Virtual directory share is best avoided, it can be buggy some times.

* Running Bailisk II on 16bit screen is not a bad idea, this version of basilisk uses lookup tables,
as result 15bit to 16bit + endiness is done in one operation, 8bit to 16bit is also uses 16bit lookup palette table. 
16bit is 1/2 of data to transferee to the graphic card vs 32bit graphics.

Hot keys
--------

**LALT+LSHIFT+CONTROL+key "+"**
  
    Increment frame skip
    
**LALT+LSHIFT+CONTROL+key "-"**

    Decrement frame skip

**LALT+LSHIFT+CONTROL+key "Print Screen"**

    Power off, (use only if system has crashed).
    
**LALT+LSHIFT+CONTROL+key "Scroll lock"**

    Trigger NMI interrupt. (not sure what this should do.)

Contributing to the translation:
--------------------------------

You do not need any programming skills! 
You do not need to have SDK installed!

The thing you need to do is make directory named your country.
copy the basilisk.ct into the directory of your country.
there is a empty line this where you should write the translation.

The list contains identifiers, this should not be changed,
the line with English text should not be changed, this is original text.  

If you are lazy you can copy the text into google translate, to get a quick result, you should read the translation carefully, to make sure it did not mistranslate, sometimes, one word can how two different meanings. Some times google translate does not create good formulated sentence. 

basilisk.cd file is the original and should not be touched, (unless you find some mistake)

basilisk.cd is used generate the locale.h and locale.c and the basilisk.ct file, in the local directory.
don’t modify the locale.h and locale.c files (this files will be overwritten), and you should only modify the copy of the basilisk.ct file for your language.

If you like to generate catalog file, you should have the SDK installed.

You need to have the tools CATCOMP and FLEXCAT installed. (one of the tools are in the SDK I believe.)
look on OS4Depot.net and Aminet.net if it’s not included in the AmigaOS4 SDK.

Compiling
---------

First of all you need the AmigaOS4 SDK from Hyperion Enteriment.

*Cd src*

*Makelink uae_cpu uae_cpu_old*

You need an assign, its because some files has to generated.

*assign uae_cpu: uae_cpu*

*cd AmigaOS-4*

To build AmigaOS-4 version.

*make*

to build the soft FPU version (this one need GLIB version we do not have on AmigaOS4)

*make -f make-fpusoftcore*

you can get glibc from here.

http://ftp.gnu.org/gnu/glibc/


License
-------

Basilisk II is available under the terms of the GNU General Public License.
See the file "COPYING" that is included in the distribution for details.

Copyright (C) 1997-2008 Christian Bauer et al.

Some of the files are under MIT licence, read the top of files.
(C) 2020 Kjetil Hvalstrand
