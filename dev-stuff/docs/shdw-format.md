shdw files are split into cells. Each cell contains a list of lines that are used for the purposes of shadowcasting.

### Format

| Name       | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| magic      | char\[3\]           | always 'SHDW' in ASCII |
| cell_count | u32                 |                       |
| cell_width | u32                 |                       |
| cell_height| u32                 |                       |
| cells      | Cell\[cell_count\]  |                       |

### Cell

| Field      | Type                | Notes                 |
|------------|:-------------------:|----------------------:|
| point_count| u32                 |                       |
| points     | f32\[point_count\]\[2\]|                       |
