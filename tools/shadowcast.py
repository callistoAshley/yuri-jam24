from PIL import Image
from numpy import array, pad
from skimage import measure
import matplotlib.pyplot as plt
import sys

if len(sys.argv) <= 2 or len(sys.argv) >= 6:
    print("Usage: shadowcast.py <image> <output> | <cell_width> <cell_height>")
    sys.exit(1)

image = Image.open(sys.argv[1])

cell_width = image.width
cell_height = image.height

if len(sys.argv) == 5:
    cell_width = int(sys.argv[3])
    cell_height = int(sys.argv[4])

cell_count_x = image.width // cell_width
cell_count_y = image.height // cell_height

# create a list of cells of cell_width x cell_height
cells = []
for y in range(0, image.height // cell_height):
    for x in range(0, image.width // cell_width):
        cell = image.crop((x * cell_width, y * cell_height, (x + 1) * cell_width, (y + 1) * cell_height))
        cells.append(cell)

fig, ax = plt.subplots()
ax.imshow(image)

for i in range(len(cells)):
    img_cell = cells[i]
    cell = array(img_cell)
    cell = cell[:, :, 3]  # get the alpha channel
    cell = pad(cell, pad_width=1, mode='constant', constant_values=0) # pad the cell to avoid border artifacts
    binary_mask = cell > 0 # create a binary mask
    contours = measure.find_contours(binary_mask, level=0.5)  # 0.5 is a typical level for binary images

    cell_x = i % cell_count_x * cell_width
    cell_y = i // cell_count_x * cell_height

    for contour in contours:
        ax.plot(contour[:, 1] + cell_x, contour[:, 0] + cell_y, linewidth=1, color='red')

plt.show()
