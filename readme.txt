Object Script Module (osmdemo.zip) by Tom N Harris, with modifications
by Andrew Durdin.

Builds with mingw32 gcc; I'm using TDM GCC 9.2.0:

    https://jmeubank.github.io/tdm-gcc/articles/2020-03/9.2.0-release

To build:

    make

By default, this builds three osms:

* empty.osm: a bare example with no scripts.

* echo.osm: an 'Echo' script that displays all messages received on screen.

* demo.osm: the original demo scripts from osmdemo.zip


KNOWN BUGS
----------

* demo.osm builds, but won't load. I don't know why.
