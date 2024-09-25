inff files are split into chunks. Each chunk is identified by a four-character identifier, and followed by its size and an arbitrary array of data.

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

INFF is not currently used anymore. This may change in the future!