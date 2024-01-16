Periapt Script Module: for the Thief 2 fan mission, codename 'Dual'.

Based on Object Script Module (osmdemo.zip) by Tom N Harris.

Builds with mingw32 gcc; I'm using TDM GCC 9.2.0:

    https://jmeubank.github.io/tdm-gcc/articles/2020-03/9.2.0-release

To build:

    make


KNOWN BUGS
----------

* Too many to list! This is a work in progress.


DLL LOAD FAILURES AND TLS
-------------------------

A sporadic issue existed where this .osm would suddenly begin failing to load;
sometimes restarting DromEd.exe/Thief.exe would resolve it, but sometimes it
would only resolve after restarting Windows. It turned out this failure would
only happen when the .osm was relocated away from its default base address.

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

The possible solutions I identified were:

    * rebuild libmingw32 to not use thread local storage (iffy)
    * port the dll to visual c++
    * ignore it and hope it goes away

Previously, I chose the third option, specifying a custom base address for the
.osm, hoping to minimize the chance of an address space clash that would require
relocation.

However, Xanfre informed me that it was possible to prevent the static TLS
initialization without messing with libmingw32:

    While I am aware that you are not focused on this right now, regarding static TLS causing a protection fault during relocation, it is possible to resolve this without going through the pain of moving to Visual C++ or hacking away at the MinGW runtime. The culprit here is, in fact, the MinGW runtime, specifically the object tlssup.o being brought in with the CRT initialization routines, which, unsurprisingly, uses TLS. In particular, it is the TLS initialization hook (__dyn_tls_init_callback) that does this. This is a function pointer that is called during startup if it is non-null. It is declared extern in crtdll.c, though the only definition that will satisfy it is in tlssup.c. This means that the hook is always called, and the noted object is also always brought into the binary. Other CRT implementations provide an additional uninitialized definition, over which the non-zero definition takes precedence if user code employs any __declspec(thread) identifiers. For MinGW, however, it is sufficient to simply supply a null definition, which will prevent the hook from being called and tlssup.o from being brought in by the linker. Consequently, simply adding the following somewhere in the sources will resolve this, with ScriptModule.cpp likely being a sensible choice.

    const void *__dyn_tls_init_callback = NULL;

    This should mitigate the need to carefully choose a base address, as relocation can now succeed. Analyzing the resulting binary should also show that no .tls section is present anymore.

So at the moment, I have defined __dyn_tls_init_callback in ScriptModule.cpp as
suggested, and confirmed that the resulting .osm no longer contains the static
.tls section. And reverted back to the auto base address generation in the
build (though this is not strictly necessary).

After a few brief test runs, the issue has not yet appeared again, but this is
not sufficient to show that it is fixed, as it only happened sporadically, and
usually after a significant number of game mode/edit mode cycles. Additionally,
it would be necessary to demonstrate that the .osm *is* getting relocated
and still loading successfully, in order to have confidence that the issue is
resolved. Building with a fixed base address that is known to clash is one way
to force a relocation to occur.
