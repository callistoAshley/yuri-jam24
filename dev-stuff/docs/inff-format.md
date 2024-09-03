.mnff/tnff files are split into chunks. Each chunk is identified by a four-character identifier, and followed by its size and an arbitrary array of data.

When "String" shows up as a data type, it means that there is a u32 length before a u8 array.

### Format

| Name       | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| magic      | char\[3\]           | always 'INFF' in ASCII |
| num_chunks | u32                 |                       |
| chunks     | Chunk\[num_chunks\] |                       |

### Chunk

| Field      | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| id         | char\[4\]           | Must be one of the folowwing chunk types. |
| data_len   | u32                 |                       |
| data       | u8\[data_len\]      |                       |

### Chunk types

##### INFO

Only used in maps, always the first chunk.

| Field      | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| width      | u32                 |                       |
| height     | u32                 |                       |

##### TSET

1 is always expected (though the format could support more.)
Tilesets MUST be 8x8.

| Field      | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| name       | String              |                       |
| img        | String              |                       |

##### TINF

Start of a tilemap layer. Contains layer information.

| Field      | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| name       | String              |                       |
| class_name | String              |                       |
| parallax_x | f32                 |                       |
| parallax_y | f32                 |                       |

##### TDAT

Tilemap layer data. This always comes after a TINF. 

| Field      | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| name       | i32\[map.width * map.height\] | List of tile ids. |

##### OINF

Object layer info.

| Field      | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| name       | String              |                       |
| class_name | String              |                       |
| object_len | u32                 |                       |

##### ODAT

Object layer data. Always comes after a OINF.
Objects are treated as colliders!

Contains an array of `Object[object_len]` (from the previous chunk).
Object layout is as follows:

| Field      | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| name       | String              |                       |
| class_name | String              |                       |
| x          | f32                 |                       |
| y          | f32                 |                       |
| width      | f32                 |                       |
| height     | f32                 |                       |
| type       | u32                 | *See below for vals.  |
| polygonLen | u32                 |                       |
| points     | f32\[polygonLen\]\[2\]  | Points of the polygon |

type:
| Value      | Variant             |
|------------|:-------------------:|
| 0 | Rectangle |
| 1 | Polygon |
| 2 | Polyline |
| 3 | Ellipse |
| 4 | Text |
| 5 | Point |

##### GLYR

A layer group, which contains other layers.

| Field      | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| name       | String              |                       |
| class_name | String              |                       |
| layer_len  | u32                 |                       |

Expect `layer_len` layers to follow!

##### ILYR

An image layer. Contains a single image.

| Field      | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| name       | String              |                       |
| class_name | String              |                       |
| img        | String              |                       |
| parallax_x | f32                 |                       |
| parallax_y | f32                 |                       |

##### TDEF

A tileset definition. Only appears in tnff files.

| Field      | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| name       | String              |                       |
| class_name | String              |                       |

Tile width and height is fixed to 8x8px.