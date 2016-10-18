# Simple Alembic to glTF converter example

## Features

* Polygon mesh.

## Limitations

* Alembic data with Ogawa backend only
* Simple poly mesh only
* Simple curve only
* Static mesh only(Use first time sample. no animation)

## Compile

OpenEXR(ilmbase), and Alembic 1.6 or later are equired to compile the converter.

Edit include and lib paths for Alembic OpenEXR(ilmbase) in `Makefile`, then simply:

    $ make

## Alembic data

I am testing with Alembic data using Blender's Alembic exporter(feature available from Blender 2.78)

## Extensions

### Curves

Curves are reprenseted as point primitive(mode = 0) in glTF level for the backward compatibility of existing glTF specification.
The number of vertices per curve information is aded to `NVERRTS` attribute.
"ext_mode" extra property is added and set it to "curves" string to specify the object as curves primitive.

Here is an example JSON description of curves primitive.


    "meshes": {
      "curves_1": {
        "primitives": [
          {
            "attributes": {
              "POSITION": "accessor_vertices",
              "NVERTS": "accessor_nverts",
            },
            "material": "material_1",
            "mode": 0,
            "extras" {
              "ext_mode" : "curves"
            } 
          }
        ]
      }
    },


## TODO

* [ ] Support normals and texcoords
* [ ] Support mutiple meshes
* [ ] Support animation(time samples)
* [ ] Support scene graph(node hierarchy)
* [ ] Support Point and SubD?
