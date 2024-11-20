use clap::Parser;
use image::GenericImage;
use svg::node::element::path::Command;

/// Trace the outline of an image and output it to a shdw file
#[derive(clap::Parser)]
struct Args {
    /// The path to the image
    image: String,
    /// The path to the shdw file
    shdw: String,

    /// The width of an animation cell.
    cell_width: Option<u32>,
    /// The height of an animation cell.
    cell_height: Option<u32>,
}

type Error = Box<dyn std::error::Error>;

struct Shdw {
    cell_width: u32,
    cell_height: u32,

    cells: Vec<Cell>,
}

struct Cell {
    lines: Vec<Line>,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, bytemuck::Pod, bytemuck::Zeroable)]
struct Line {
    start: Point,
    end: Point,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, bytemuck::Pod, bytemuck::Zeroable)]
struct Point {
    x: f32,
    y: f32,
}

impl Shdw {
    fn write_raw<T>(writer: &mut impl std::io::Write, value: T) -> std::io::Result<()>
    where
        T: Sized + bytemuck::Pod + bytemuck::Zeroable,
    {
        let buffer = bytemuck::bytes_of(&value);
        writer.write_all(buffer)?;

        Ok(())
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

fn lines_of_image(image: &impl image::GenericImageView<Pixel = image::Rgba<u8>>) -> Vec<Line> {
    let mut bits = vec![vec![0; image.width() as usize]; image.height() as usize];
    for (x, y, pixel) in image.pixels() {
        bits[y as usize][x as usize] = (pixel.0[3] > 0) as i8;
    }

    let paths = contour_tracing::array::bits_to_paths(bits, true);
    let path_data =
        svg::node::element::path::Data::parse(&paths).expect("tracer output invalid paths");

    let mut cursor = Point { x: 0.0, y: 0.0 };
    let mut at_move_cursor = cursor;

    let mut lines = vec![];

    for command in path_data.iter() {
        match command {
            // positions are always absolute. no need to handle it
            Command::Move(_, parameters) => {
                cursor.x = parameters[0];
                cursor.y = parameters[1];

                at_move_cursor = cursor;
            }
            Command::HorizontalLine(_, parameters) => {
                let start = cursor;
                cursor.x += parameters[0];
                let end = cursor;
                lines.push(Line { start, end });
            }
            Command::VerticalLine(_, parameters) => {
                let start = cursor;
                cursor.y += parameters[0];
                let end = cursor;
                lines.push(Line { start, end });
            }
            Command::Close => {
                let start = cursor;
                let end: Point = at_move_cursor;
                lines.push(Line { start, end });
                cursor = at_move_cursor;
            }
            _ => unimplemented!(), // the contour tracer doesn't spit these out
        }
    }

    lines
}

fn main() -> Result<(), Error> {
    let args = Args::parse();

    let mut image = image::open(args.image)?.into_rgba8();

    let cell_width = args.cell_width.unwrap_or(image.width());
    let cell_height = args.cell_width.unwrap_or(image.height());

    let cell_count_x = image.width() / cell_width;
    let cell_count_y = image.height() / cell_height;

    let mut cells = Vec::with_capacity(cell_count_x as usize * cell_count_y as usize);
    for y in 0..cell_count_y {
        for x in 0..cell_count_x {
            let sub_image =
                image.sub_image(x * cell_width, y * cell_height, cell_width, cell_height);
            let lines = lines_of_image(&*sub_image);
            cells.push(Cell { lines });
        }
    }

    let shdw = Shdw {
        cell_width,
        cell_height,
        cells,
    };

    let file = std::fs::File::create(args.shdw)?;
    shdw.write(file)?;

    Ok(())
}
