# Mesh dump experiment

Sometimes we want to tweak mesh attributes(e.g. vertex position, uv coord, etc).
glTF itself does not allow ASCII representation of such data.

This example show how to

- Export mesh data from .bin to .csv
- Import mesh data to .bin(update corresponding buffer data) from .csv

## Requirement

Assume Buffer is stored as external file(`.bin`)
