# BasiliskII 1.0

Date: October-23-2020

This is old version of Basilisk II, I know there are newer more update versions around on GITHUB.
This version focusses on AmigaOS4.1, so is designed around AmigaOS4.1 native graphic feature.

Please read the manual, if you like to mess around with thw configuration files, (you should not need to)

# Requeued hardware specifications 

680x0 interpreted emulation does requires a lot resources, sadly we do not support JIT (Even if it’s possible on EUAE), 
as result. No one has investigated how to integrated it, and CPU emulation is most complicated part.
We can sadly not expose Petunia (the AmigaOS4.1 integrated JIT) to Basilisk, MacOS needs some memory maps
used by AmigaOS. It was suggesting using MMU trick it, but not sure how too.

A 1.8GHz PowerPC is will give a decent speed for most things.

If you have 800 Mhz G4, the emulation speed will feel not great.

# A few tips

* If you are using Fullscreen, make sure to enable "Screen Lock", if not basilisk II might crash, 
it used to be possible to write directly to screens bitmap, I’m not sure that’s possible anymore.

* Emulated CPU task priority, range from 0 to -128, the CPU emulation has a tendency to hog the CPU resources,
as a result AmigaOS run into stall states and complete lock up, setting task priority of -1 for active window
is recommended, if above 1, critical tasks like CPU, eventhandler and other system critical task,
will be starved for CPU resources. (This is way it's limited to max 0.)

* Creating when creating Mac partitions in media Tools box, its recommended that the partition is not above
4Gbytes, if your emulating MacOS7.5.5, if the partition is too big you will get errors, when tying to
copy files to it, under MacOS. You are not supposed to be able to mount the partitions under AmigaOS,
so disable automount.

* For compressed cd iso images it’s best decompress it on Amiga side,
you mount CD ISO image on Amiga side, and use diskimage.device, 
so you don’t need to transferee the files using HFS.

* Virtual directory share is best avoided, it can be buggy some times.

Best Regards
Kjetil Hvalstrand 
The maintainer of the AmigaOS4.1 build of Basilisk II.

License
-------

Basilisk II is available under the terms of the GNU General Public License.
See the file "COPYING" that is included in the distribution for details.

Copyright (C) 1997-2008 Christian Bauer et al.

Some of the files are under MIT licence, read the top of files.
(C) 2020 Kjetil Hvalstrand
