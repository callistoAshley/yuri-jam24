.lvl files are split into chunks. Each chunk is identified by a four-character identifier, and followed by its size and an arbitrary array of data.

### Format

| Name       | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| magic      | char\[3\]           | always 'LVL' in ASCII |
| num_chunks | u32                 |                       |
| chunks     | Chunk\[num_chunks\] |                       |

### Chunk

| Field      | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| id         | char\[4\]           |                       |
| data_len   | u32                 |                       |
| data       | u8\[data_len\]      |                       |
