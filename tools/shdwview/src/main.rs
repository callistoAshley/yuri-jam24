use clap::Parser;

use egui::Color32;
use egui_plot::{PlotPoint, PlotPoints};
use shdw::Shdw;

struct App {
    texture_handle: egui::TextureHandle,

    shdw: Shdw,
}

/// Views a shdw file against a reference image.
#[derive(clap::Parser)]
struct Args {
    /// The path to the reference image
    image: String,
    /// The path to the shdw file
    shdw: String,
}

type Error = Box<dyn std::error::Error + Send + Sync>;

impl App {
    fn new(args: Args, cc: &eframe::CreationContext<'_>) -> Result<Self, Error> {
        let mut image = image::open(args.image)?.into_rgba8();
        // discard any transparent pixels (egui renders them weird)
        for pixel in image.pixels_mut() {
            if pixel.0[3] == 0 {
                pixel.0 = [0; 4];
            }
        }

        let mut shdw_file = std::fs::File::open(args.shdw)?;
        let shdw = Shdw::read(&mut shdw_file)?;

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
        })
    }
}

impl eframe::App for App {
    fn update(&mut self, ctx: &eframe::egui::Context, _: &mut eframe::Frame) {
        let [texture_width, _] = self.texture_handle.size();
        let cell_count_x = texture_width / self.shdw.cell_width as usize;

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
                    plot.image(image);

                    for (i, cell) in self.shdw.cells.iter().enumerate() {
                        let cell_x = i % cell_count_x * self.shdw.cell_width as usize;
                        let cell_x = cell_x as f64;

                        let cell_y = i / cell_count_x * self.shdw.cell_height as usize;
                        let cell_y = cell_y as f64;

                        let cell_w = self.shdw.cell_width as f64;
                        let cell_h = self.shdw.cell_height as f64;

                        let cell_points = [
                            [cell_x, -cell_y],
                            [cell_x + cell_w, -cell_y],
                            [cell_x + cell_w, -(cell_y + cell_h)],
                            [cell_x, -(cell_y + cell_h)],
                        ];
                        let cell_points = cell_points.into_iter().collect::<PlotPoints>();

                        let cell_stroke = egui::Stroke::new(2.0, Color32::LIGHT_RED);
                        let polygon = egui_plot::Polygon::new(cell_points)
                            .fill_color(Color32::TRANSPARENT)
                            .style(egui_plot::LineStyle::dashed_loose())
                            .stroke(cell_stroke);
                        plot.polygon(polygon);

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

                        let arrows =
                            egui_plot::Arrows::new(origins, tips).color(Color32::DEBUG_COLOR);
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
        eprintln!("{e}");
    }
}
