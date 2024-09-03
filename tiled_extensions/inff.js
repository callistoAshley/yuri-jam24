// @ts-check
/// <reference types="@mapeditor/tiled-api" />

// .inff (Inff's Not a File Format) file extension
// based on riff!


class ChunkDataBuilder {
  constructor() {
    this.length = 0;
  }

  addUint32() {
    this.length += 4;
  }

  addFloat32() {
    this.length += 4;
  }

  addString(str) {
    this.addUint32();
    this.length += str.length;
  }

  build() {
    var buffer = new ArrayBuffer(this.length);
    return new ChunkDataWriter(buffer);
  }
}

class ChunkDataWriter {
  constructor(buffer) {
    this.view = new DataView(buffer);
    this.offset = 0;
  }

  writeUint32(value) {
    this.view.setUint32(this.offset, value, true);
    this.offset += 4;
  }

  writeFloat32(value) {
    this.view.setFloat32(this.offset, value, true);
    this.offset += 4;
  }

  writeString(str) {
    this.writeUint32(str.length);
    for (var i = 0; i < str.length; i++) {
      this.view.setUint8(this.offset + i, str.charCodeAt(i));
    }
    this.offset += str.length;
  }

  finish() {
    return this.view.buffer;
  }
}

var chunk_count = 0;
/**
 * @param {TileLayer} tile_layer
 * @param {BinaryFile} file
 */
function write_tile_layer_chunks(tile_layer, file) {
  {
    // the layer info and layer data chunks are separate
    // layer info chunk:
    var chunk_builder = new ChunkDataBuilder();
    chunk_builder.addString(tile_layer.name);
    chunk_builder.addString(tile_layer.className);
    // parallax factor
    chunk_builder.addFloat32();
    chunk_builder.addFloat32();

    var chunk_writer = chunk_builder.build();

    chunk_writer.writeString(tile_layer.name);
    chunk_writer.writeString(tile_layer.className);
    chunk_writer.writeFloat32(tile_layer.parallaxFactor.x);
    chunk_writer.writeFloat32(tile_layer.parallaxFactor.y);

    writeChunk("TINF", chunk_writer.finish(), file);
    chunk_count++;
  }

  {
    // write layer data chunk (we can just use Uint32Array for this)
    var layer_data_size = tile_layer.width * tile_layer.height;

    var layer_data_buffer = new Uint32Array(layer_data_size);
    for (var y = 0; y < tile_layer.height; y++) {
      for (var x = 0; x < tile_layer.width; x++) {
        var tile = tile_layer.cellAt(x, y);
        layer_data_buffer[y * tile_layer.width + x] = tile.tileId
      }
    }

    writeChunk("TDAT", layer_data_buffer.buffer, file);
    chunk_count++;
  }
}

/**
 * @param {ObjectGroup} object_layer
 * @param {BinaryFile} file
 */
function write_object_layer_chunks(object_layer, file) {
  // layer info chunk
  {
    var chunk_builder = new ChunkDataBuilder();
    chunk_builder.addString(object_layer.name);
    chunk_builder.addString(object_layer.className);
    chunk_builder.addUint32(); // number of objects in the layer

    var chunk_writer = chunk_builder.build();
    chunk_writer.writeString(object_layer.name);
    chunk_writer.writeString(object_layer.className);
    chunk_writer.writeUint32(object_layer.objects.length);

    writeChunk("OINF", chunk_writer.finish(), file);
  }
  // layer data chunk
  {
    var chunk_builder = new ChunkDataBuilder();
    for (var object of object_layer.objects) {
      chunk_builder.addString(object.name);
      chunk_builder.addString(object.className);
      chunk_builder.addUint32(); // ID

      // object position
      chunk_builder.addFloat32();
      chunk_builder.addFloat32();

      // object size
      chunk_builder.addFloat32();
      chunk_builder.addFloat32();

      // object type
      chunk_builder.addUint32();

      // polygon points
      chunk_builder.addUint32();
      for (var point of object.polygon) {
        chunk_builder.addFloat32();
        chunk_builder.addFloat32();
      }
    }

    var chunk_writer = chunk_builder.build();
    for (var object of object_layer.objects) {
      chunk_writer.writeString(object.name);
      chunk_writer.writeString(object.className);
      chunk_writer.writeUint32(object.id);

      chunk_writer.writeFloat32(object.x);
      chunk_writer.writeFloat32(object.y);

      chunk_writer.writeFloat32(object.width);
      chunk_writer.writeFloat32(object.height);

      var shape;
      switch (object.shape) {
        case MapObject.Rectangle:
          shape = 0;
        case MapObject.Polygon:
          shape = 1;
        case MapObject.Polyline:
          shape = 2;
        case MapObject.Ellipse:
          shape = 3;
        case MapObject.Text:
          shape = 4;
        case MapObject.Point:
          shape = 5;
      };
      chunk_writer.writeUint32(shape);

      chunk_writer.writeUint32(object.polygon.length);
      for (var point of object.polygon) {
        chunk_writer.writeFloat32(point.x);
        chunk_writer.writeFloat32(point.y);
      }
    }
    writeChunk("ODAT", chunk_writer.finish(), file);
  }
}

/**
 * @param {TileMap} map 
 * @param {string} filename 
 * @returns {undefined}
 */
function write(map, filename) {
  var file = new BinaryFile(filename, BinaryFile.WriteOnly);

  // write magic number
  // file only supports writing ArrayBuffer, so we need to convert the string to ArrayBuffer
  // also TextEncoder is not supported in Tiled, so we need to use the old method
  var buffer = new Uint8Array(4);
  for (var i = 0; i < 4; i++) {
    buffer[i] = "INFF".charCodeAt(i);
  }
  file.write(buffer.buffer);

  // we need to know the number of chunks we'll be writing, so we'll write a placeholder for now
  chunk_count = 0;
  var chunk_buffer = new Uint32Array(1);
  // @ts-ignore
  // @ts-ignore
  var chunk_position = file.pos; // We will need to come back here to write the actual value!!!!!!!!
  file.write(chunk_buffer.buffer);

  // construct map info chunk
  var map_info_buffer = new Uint32Array(2);
  map_info_buffer[0] = map.width;
  map_info_buffer[1] = map.height;

  writeChunk("INFO", map_info_buffer.buffer, file);
  chunk_count++;

  // write the tileset (there should only be one tileset)
  var tileset = map.tilesets[0];

  {
    var chunk_builder = new ChunkDataBuilder();
    chunk_builder.addString(tileset.name);
    chunk_builder.addString(tileset.imageFileName);

    var chunk_writer = chunk_builder.build();
    chunk_writer.writeString(tileset.name);
    chunk_writer.writeString(tileset.imageFileName);

    writeChunk("TSET", chunk_writer.finish(), file);
    chunk_count++;
  }

  for (var layer of map.layers) {
    if (layer.isTileLayer)
      // @ts-ignore
      write_tile_layer_chunks(layer, file);

    if (layer.isObjectLayer)
      // @ts-ignore
      write_object_layer_chunks(layer, file);

    // TODO figure out how to write tile layer properties
  }

  // write the actual chunk count
  file.seek(chunk_position);
  chunk_buffer[0] = chunk_count;
  file.write(chunk_buffer);

  file.commit();
}

/**
 * @param {BinaryFile} file
 * @param {string} chunk_id
 * @param {ArrayBuffer} chunk_data,
 */
function writeChunk(chunk_id, chunk_data, file) {
  if (chunk_id.length !== 4) {
    throw new Error("Chunk ID must be 4 characters long");
  }

  // write chunk id
  var id_buffer = new Uint8Array(4);
  for (var i = 0; i < 4; i++) {
    id_buffer[i] = chunk_id.charCodeAt(i);
  }
  file.write(id_buffer.buffer);
  tiled.warn(`Writing chunk ${chunk_id}`, () => { });

  // write chunk size
  var size = chunk_data.byteLength;
  var size_buffer = new Uint32Array(size).buffer;
  file.write(size_buffer);

  // write chunk data
  file.write(chunk_data);
}

/**
 * @param {string} filename 
 * @returns {TileMap}
 */
// @ts-ignore
// @ts-ignore
function read(filename) { throw new Error("Not implemented"); }

var custom_format = {
  name: "Inff's Not a File Format",
  extension: "inff",

  // read: read,
  write: write
};
// @ts-ignore
tiled.registerMapFormat(custom_format.extension, custom_format);