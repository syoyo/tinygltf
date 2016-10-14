# Simple Alembic to glTF converter example

## Features

* Polygon mesh.

## Limitations

* Alembic data with Ogawa backend only
* Simple poly mesh only
* Static mesh only(Use first time sample. no animation)

## Compile

OpenEXR(ilmbase), and Alembic 1.6 or later are equired to compile the converter.

Edit include and lib paths for Alembic OpenEXR(ilmbase) in `Makefile`, then simply:

    $ make

## Alembic data

I am testing with Alembic data using Blender's Alembic exporter(feature available from Blender 2.78)

## TODO

* [ ] Support normals and texcoords
* [ ] Support mutiple meshes
* [ ] Support animation(time samples)
* [ ] Support scene graph(node hierarchy)
* [ ] Support Point, Curve and SubD?
