pub struct Shdw {
    pub cell_width: u32,
    pub cell_height: u32,

    pub cells: Vec<Cell>,
}

#[derive(Default)]
pub struct Cell {
    pub lines: Vec<Line>,
}

#[repr(C)]
#[derive(bytemuck::Pod, bytemuck::Zeroable, Clone, Copy)]
pub struct Line {
    pub start: Point,
    pub end: Point,
}

#[repr(C)]
#[derive(bytemuck::Pod, bytemuck::Zeroable, Clone, Copy)]
pub struct Point {
    pub x: f32,
    pub y: f32,
}

impl Shdw {
    fn read_raw<T>(reader: &mut impl std::io::Read) -> std::io::Result<T>
    where
        T: Sized + bytemuck::Pod + bytemuck::Zeroable,
    {
        // I *would* like to use a size_of<T> buffer but rust isn't able to make that work yet without using vec![]
        let mut buffer = [0; 256];
        let subslice = &mut buffer[..size_of::<T>()];
        reader.read_exact(subslice)?;

        Ok(*bytemuck::from_bytes(subslice))
    }

    fn write_raw<T>(writer: &mut impl std::io::Write, value: T) -> std::io::Result<()>
    where
        T: Sized + bytemuck::Pod + bytemuck::Zeroable,
    {
        let buffer = bytemuck::bytes_of(&value);
        writer.write_all(buffer)?;

        Ok(())
    }

    pub fn read(mut reader: impl std::io::Read) -> std::io::Result<Self> {
        let mut magic = [0; 4];
        reader.read_exact(&mut magic)?;

        if &magic != b"SHDW" {
            return Err(std::io::Error::other("wrong magic number"));
        }

        let cell_count = Self::read_raw::<u32>(&mut reader)? as usize;
        let cell_width = Self::read_raw::<u32>(&mut reader)?;
        let cell_height = Self::read_raw::<u32>(&mut reader)?;

        let mut cells = Vec::with_capacity(cell_count);
        for _ in 0..cell_count {
            let line_count = Self::read_raw::<u32>(&mut reader)? as usize;
            let mut lines = Vec::with_capacity(line_count);

            for _ in 0..line_count {
                let line = Self::read_raw::<Line>(&mut reader)?;
                lines.push(line);
            }
            cells.push(Cell { lines });
        }

        Ok(Self {
            cell_width,
            cell_height,
            cells,
        })
    }

    pub fn write(&self, mut writer: impl std::io::Write) -> std::io::Result<()> {
        let magic = b"SHDW";
        writer.write_all(magic)?;

        Self::write_raw::<u32>(&mut writer, self.cells.len() as u32)?;
        Self::write_raw::<u32>(&mut writer, self.cell_width)?;
        Self::write_raw::<u32>(&mut writer, self.cell_height)?;

        for cell in &self.cells {
            Self::write_raw::<u32>(&mut writer, cell.lines.len() as u32)?;

            for line in &cell.lines {
                Self::write_raw::<Line>(&mut writer, *line)?;
            }
        }

        writer.flush()
    }
}
