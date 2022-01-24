Periapt Script Module: for the Thief 2 fan mission, codename 'Dual'.

Based on Object Script Module (osmdemo.zip) by Tom N Harris.

Builds with mingw32 gcc; I'm using TDM GCC 9.2.0:

    https://jmeubank.github.io/tdm-gcc/articles/2020-03/9.2.0-release

To build:

    make


KNOWN BUGS
----------

* Too many to list! This is a work in progress.


DLL BASE ADDRESS
----------------

We specify a custom base address, hoping to maximise the chance that the
.osm can be loaded without relocation. Here's why:

The mingw build does its tls (thread local storage) initialisation via the
static tls section of the dll. This is _not_ a supported configuration for
dlls that will be loaded via LoadLibrary():

    Statically declared TLS data objects can be used only in
    statically loaded image files. This fact makes it unreliable to
    use static TLS data in a DLL unless you know that the DLL, or
    anything statically linked with it, will never be loaded
    dynamically with the LoadLibrary API function.
    -- <https://docs.microsoft.com/en-us/windows/win32/debug/pe-format>

A little more detail is provided by KB118816:

    A dynamic-link library (DLL) uses __declspec(thread) to allocate
    static thread local storage (TLS). ... On Windows 2000, the
    LoadLibrary function succeeds. However, any attempt to access the
    TLS data causes an access violation (AV).
    -- <https://www.betaarchive.com/wiki/index.php?title=Microsoft_KB_Archive/118816>

Although KB118816 only describes Windows 2000 and earlier, the underlying
problem remains in some form:

    On Windows operating systems before Windows Vista,
    __declspec( thread ) has some limitations. If a DLL declares any
    data or object as __declspec( thread ), it can cause a protection
    fault if dynamically loaded. After the DLL is loaded with
    LoadLibrary, it causes system failure whenever the code references
    the __declspec( thread ) data.
    -- <https://docs.microsoft.com/en-us/cpp/parallel/thread-local-storage-tls?view=msvc-170>

The behaviour I have observed is a little different: When the .osm is loaded
at its default base address, then the LoadLibrary() call succeeds, and
everything appears to work correctly. However, if the .osm is relocated
during loading, then an access violation occurs within the LoadLibrary() call
(during LdrpAllocateTlsEntry()), which is caught by the OS, and the
LoadLibrary() call returns an error.

The NewDark .exes have been compiled to support Windows 2000 or later, which
is possibly responsible for the unusual behaviour I am seeing on Windows 10,
despite the "before Windows Vista" qualification above.

After identifying the relocation as triggering the load failure, I found
MinGW bug 1557 which describes the exact behaviour I am seeing:

    This is exactly what happened to my DLLs. My hook DLL is being
    loaded at the start time of any executable (via LoadPerProcess
    registry entry), and in some cases loading my DLL will trigger
    a segmentation fault (Windows will hide this fault, thus user
    won't observe it). However, the functionality of my DLL will be,
    obviously, missing.
    -- <https://sourceforge.net/p/mingw/bugs/1557/>

So why does this issue arise at all, since I am not using __declspec(thread)
or any other thread local storage in my .osm? According to the comments on
MinGW bug 1557, it is libmingw32 that is the culprit:

    I suspect the mingw runtime is the culprit, instead:

    $ objdump -h /mingw/lib/libmingw32.a | grep tls
    tlsmcrt.o: file format pe-i386
    tlsmthread.o: file format pe-i386
    tlssup.o: file format pe-i386
    6 .tls$AAA 00000004 00000000 00000000 00000fa6 2**2
    7 .tls$ZZZ 00000004 00000000 00000000 00000faa 2**2
    10 .tls 00000018 00000000 00000000 00000fb6 2**2
    tlsthrd.o: file format pe-i386

So, the possible solutions are:

    * rebuild libmingw32 to not use thread local storage (iffy)
    * port the dll to visual c++
    * ignore it and hope it goes away

In classic Benny fashion, I am choosing to do the latter. Well, I am not
entirely ignoring it; I am instead defining a default base address that I
am _guessing_ will not often clash and require relocation. This is based
only on a handful of runs I have done on my machine, which honestly is
a pretty poor sample size. However, come beta time, I can perhaps write
a small utility to dump the loaded modules of Thief2.exe and their base
addresses, and make the .mis detect if periapt.osm did not load, and include
a message asking players to run the utility (without exiting thief!) and
notify me of its output. That would give me more data to choose an even
less likely to clash address.
