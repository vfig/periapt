Object Script Module (osmdemo.zip) by Tom N Harris.

Small tweaks to get it building with [TDM GCC 9.2.0](https://jmeubank.github.io/tdm-gcc/articles/2020-03/9.2.0-release).

Added echo.cpp, a stripped down Demo.cpp with just the Echo script.

To build:

    make
    cd demo
    make echo.osm empty.osm Demo.osm

Status:

* echo.osm: builds and works ok.

* empty.osm: builds, but (ofc) does nothing.

* Demo.osm: builds, but won't load. Don't know why.
