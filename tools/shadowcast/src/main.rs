use clap::Parser;

use egui::{Color32, Widget};
use egui_plot::{PlotPoint, PlotPoints};

struct App {
    texture_handle: egui::TextureHandle,

    shdw: Shdw,
    shdw_file: std::fs::File,
}

#[derive(clap::Parser)]
struct Args {
    /// The path to the reference image
    image: String,
    /// The path to the output shdw file
    shdw: String,
}

struct Shdw {
    cell_width: usize,
    cell_height: usize,

    cells: Vec<Cell>,
}

#[derive(Default)]
struct Cell {
    lines: Vec<Line>,
}

#[repr(C)]
#[derive(bytemuck::Pod, bytemuck::Zeroable, Clone, Copy)]
struct Line {
    start: Point,
    end: Point,
}

#[repr(C)]
#[derive(bytemuck::Pod, bytemuck::Zeroable, Clone, Copy)]
struct Point {
    x: f32,
    y: f32,
}

impl Shdw {
    fn read_raw<T>(reader: &mut impl std::io::Read) -> Result<T, std::io::Error>
    where
        T: Sized + bytemuck::Pod + bytemuck::Zeroable,
    {
        // I *would* like to use a size_of<T> buffer but rust isn't able to make that work yet without using vec![]
        let mut buffer = [0; 256];
        let subslice = &mut buffer[..size_of::<T>()];
        reader.read_exact(subslice)?;

        Ok(*bytemuck::from_bytes(subslice))
    }

    pub fn read(mut reader: impl std::io::Read) -> Result<Self, std::io::Error> {
        let mut magic = [0; 4];
        reader.read_exact(&mut magic)?;

        if &magic != b"SHDW" {
            return Err(std::io::Error::other("wrong magic number"));
        }

        let cell_count = Self::read_raw::<u32>(&mut reader)? as usize;
        let cell_width = Self::read_raw::<u32>(&mut reader)? as usize;
        let cell_height = Self::read_raw::<u32>(&mut reader)? as usize;

        let mut cells = Vec::with_capacity(cell_count);
        for _ in 0..cell_count {
            let line_count = Self::read_raw::<u32>(&mut reader)? as usize / 2; // TODO remove the division by 2
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

    pub fn shdw_for(image: &image::RgbaImage) -> Self {
        Self {
            cell_width: image.width() as usize,
            cell_height: image.height() as usize,

            cells: vec![Default::default()],
        }
    }
}

type Error = Box<dyn std::error::Error + Send + Sync>;

impl App {
    fn new(args: Args, cc: &eframe::CreationContext<'_>) -> Result<Self, Error> {
        let image = image::open(args.image)?.into_rgba8();

        let mut shdw_file = std::fs::File::options()
            .create(true)
            .truncate(false)
            .read(true)
            .write(true)
            .open(args.shdw)?;
        let shdw = Shdw::read(&mut shdw_file).unwrap_or_else(|err| {
            eprintln!("Failed to load shdw: {err}");
            eprintln!("Using default instead");
            Shdw::shdw_for(&image)
        });

        let color_image = egui::ColorImage {
            size: [image.width() as _, image.height() as _],
            pixels: bytemuck::cast_vec(image.into_raw()),
        };
        let texture_handle =
            cc.egui_ctx
                .load_texture("the-image", color_image, egui::TextureOptions::NEAREST);

        Ok(App {
            texture_handle,

            shdw,
            shdw_file,
        })
    }
}

impl eframe::App for App {
    fn update(&mut self, ctx: &eframe::egui::Context, frame: &mut eframe::Frame) {
        egui::TopBottomPanel::top("top menubar").show(ctx, |ui| {
            ui.horizontal(|ui| {
                ui.label("Cell width");
                egui::DragValue::new(&mut self.shdw.cell_width)
                    .range(0..=100)
                    .ui(ui);

                ui.label("Cell height");
                egui::DragValue::new(&mut self.shdw.cell_height)
                    .range(0..=100)
                    .ui(ui);
            });
        });

        let [texture_width, texture_height] = self.texture_handle.size();
        let cell_count_x = texture_width / self.shdw.cell_width;
        let cell_count_y = texture_height / self.shdw.cell_height;

        self.shdw
            .cells
            .resize_with(cell_count_x * cell_count_y, Default::default);

        egui::CentralPanel::default().show(ctx, |ui| {
            egui_plot::Plot::new("cell_plot")
                .data_aspect(1.0)
                .show(ui, |plot| {
                    let texture_size = self.texture_handle.size_vec2();

                    let image_center = PlotPoint::new(texture_size.x / 2.0, -texture_size.y / 2.0);
                    let image = egui_plot::PlotImage::new(
                        self.texture_handle.id(),
                        image_center,
                        texture_size,
                    );
                    let image = image.bg_fill(Color32::GRAY);
                    plot.image(image);

                    for (i, cell) in self.shdw.cells.iter().enumerate() {
                        let cell_x = i % cell_count_x * self.shdw.cell_width;
                        let cell_x = cell_x as f64;

                        let cell_y = i / cell_count_x * self.shdw.cell_height;
                        let cell_y = cell_y as f64;

                        let origins = cell
                            .lines
                            .iter()
                            .map(|l| {
                                let x = l.start.x as f64 + cell_x;
                                let y = l.start.y as f64 + cell_y;
                                [x, -y]
                            })
                            .collect::<PlotPoints>();
                        let tips = cell
                            .lines
                            .iter()
                            .map(|l| {
                                let x = l.end.x as f64 + cell_x;
                                let y = l.end.y as f64 + cell_y;
                                [x, -y]
                            })
                            .collect::<PlotPoints>();

                        let arrows = egui_plot::Arrows::new(origins, tips);
                        plot.arrows(arrows);
                    }
                });
        });
    }
}

fn main() {
    let args = Args::parse();

    let native_options = eframe::NativeOptions::default();

    let result = eframe::run_native(
        "shadowcast",
        native_options,
        Box::new(|cc| App::new(args, cc).map(|a| Box::new(a) as Box<_>)),
    );

    if let Err(e) = result {
        eprintln!("Error: {e}");
    }
}
