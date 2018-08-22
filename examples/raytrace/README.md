# NanoSG

Simple, minimal and header-only scene graph library for NanoRT.

NanoSG itself shoud be compiled with C++-03 compiler, but demo code uses C++11 features.

![screenshot of the demo program](images/nanosg-demo.png)

![Animation showing node manipulation](https://media.giphy.com/media/l3JDO29fMFndyObHW/giphy.gif)

## Build

### Linux or macOS

```bash
premake5 gmake
make
```

### Windows

```bash
premake5 vs2015
```

## Data structure

### Node

Node represents scene graph node. Tansformation node or Mesh(shape) node.
Node is interpreted as transformation node when passing `nullptr` to Node class constructure.

Node can contain multiple children.

### Scene

Scene contains root nodes and provides the method to find an intersection of nodes.

## User defined data structure

Following are required in user application.

### Mesh class

Current example code assumes mesh is all composed of triangle meshes.

Following method must be implemented for `Scene::Traversal`.

```cpp
///
/// Get the geometric normal and the shading normal at `face_idx' th face.
///
template<typename T>
void GetNormal(T Ng[3], T Ns[3], const unsigned int face_idx, const T u, const T v) const;
```

### Intersection class

Represents intersection(hit) information.

### Transform

Transformation is done in the following procedure.

`M' = parent_xform x local_xform x local_pivot`

## Memory management

`Scene` and `Node` does not create a copy of asset data(e.g. vertices, indices). Thus user must care about memory management of scene assets in user side.

## API

API is still subject to change.

### Node

```cpp
void Node::SetName(const std::string &name);
```

Set (unique) name for the node.

```cpp
void Node::AddChild(const type &child);
```

Add node as child node.

```cpp
void Node::SetLocalXform(const T xform[4][4]) {
```

Set local transformation matrix. Default is identity matrix.

### Scene

```cpp
bool Scene::AddNode(const Node<T, M> &node);
```

Add a node to the scene.

```cpp
bool Scene::Commit() {
```

Commit the scene. After adding nodes to the scene or changed transformation matrix, call this `Commit` before tracing rays.
`Commit` triggers BVH build in each nodes and updates node's transformation matrix.

```cpp
template<class H>
bool Scene::Traverse(nanort::Ray<T> &ray, H *isect, const bool cull_back_face = false) const;
```

Trace ray into the scene and find an intersection.
Returns `true` when there is an intersection and hit information is stored in `isect`.

## TODO

* [ ] Compute pivot point of each node(mesh).

## Third party libraries and its icenses

* picojson : BSD license.
* bt3gui : zlib license.
* glew : BSD/MIT license.
* tinyobjloader : MIT license.
* glm : The Happy Bunny License (Modified MIT License). Copyright (c) 2005 - 2017 G-Truc Creation
* ImGui : The MIT License (MIT). Copyright (c) 2014-2015 Omar Cornut and ImGui contributors
* ImGuizmo : The MIT License (MIT). Copyright (c) 2016 Cedric Guillemet
