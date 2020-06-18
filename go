#!/bin/bash
DEST="/e/dev/T2FM/test_osm/"
make periapt.osm && cp periapt.osm "$DEST" && echo ok
