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
    let buffer = new ArrayBuffer(this.length);
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

  writeInt32(value) {
    this.view.setInt32(this.offset, value, true);
    this.offset += 4;
  }

  writeFloat32(value) {
    this.view.setFloat32(this.offset, value, true);
    this.offset += 4;
  }

  writeString(str) {
    this.writeUint32(str.length);
    for (let i = 0; i < str.length; i++) {
      this.view.setUint8(this.offset + i, str.charCodeAt(i));
    }
    this.offset += str.length;
  }

  finish() {
    return this.view.buffer;
  }
}

class ChunkDataReader {
  constructor(buffer) {
    this.view = new DataView(buffer);
    this.offset = 0;
  }

  readUint32() {
    let value = this.view.getUint32(this.offset, true);
    this.offset += 4;
    return value;
  }

  readInt32() {
    let value = this.view.getInt32(this.offset, true);
    this.offset += 4;
    return value;
  }

  readFloat32() {
    let value = this.view.getFloat32(this.offset, true);
    this.offset += 4;
    return value;
  }

  readString() {
    let length = this.readUint32();
    let str = "";
    for (let i = 0; i < length; i++) {
      str += String.fromCharCode(this.view.getUint8(this.offset + i));
    }
    this.offset += length;
    return str;
  }

  readId() {
    let id = "";
    for (let i = 0; i < 4; i++) {
      id += String.fromCharCode(this.view.getUint8(this.offset + i));
    }
    this.offset += 4;
    return id;
  }

  readN(n) {
    let slice = this.view.buffer.slice(this.offset, this.offset + n);
    this.offset += n;
    return slice;
  }
}

let chunk_count = 0;
/**
 * @param {TileLayer} tile_layer
 * @param {BinaryFile} file
 */
function write_tile_layer_chunks(tile_layer, file) {
  {
    // the layer info and layer data chunks are separate
    // layer info chunk:
    let chunk_builder = new ChunkDataBuilder();
    chunk_builder.addString(tile_layer.name);
    chunk_builder.addString(tile_layer.className);
    // parallax factor
    chunk_builder.addFloat32();
    chunk_builder.addFloat32();

    let chunk_writer = chunk_builder.build();

    chunk_writer.writeString(tile_layer.name);
    chunk_writer.writeString(tile_layer.className);
    chunk_writer.writeFloat32(tile_layer.parallaxFactor.x);
    chunk_writer.writeFloat32(tile_layer.parallaxFactor.y);

    writeChunk("TINF", chunk_writer.finish(), file);
  }

  {
    // write layer data chunk (we can just use Uint32Array for this)
    let layer_data_size = tile_layer.width * tile_layer.height;

    let layer_data_buffer = new Int32Array(layer_data_size);
    for (let y = 0; y < tile_layer.height; y++) {
      for (let x = 0; x < tile_layer.width; x++) {
        let tile = tile_layer.cellAt(x, y);
        layer_data_buffer[y * tile_layer.width + x] = tile.tileId
      }
    }

    writeChunk("TDAT", layer_data_buffer.buffer, file);
  }
}

/**
 * @param {ObjectGroup} object_layer
 * @param {BinaryFile} file
 */
function write_object_layer_chunks(object_layer, file) {
  // layer info chunk
  {
    let chunk_builder = new ChunkDataBuilder();
    chunk_builder.addString(object_layer.name);
    chunk_builder.addString(object_layer.className);
    chunk_builder.addUint32(); // number of objects in the layer

    let chunk_writer = chunk_builder.build();
    chunk_writer.writeString(object_layer.name);
    chunk_writer.writeString(object_layer.className);
    chunk_writer.writeUint32(object_layer.objects.length);

    writeChunk("OINF", chunk_writer.finish(), file);
  }
  // layer data chunk
  {
    let chunk_builder = new ChunkDataBuilder();
    for (let object of object_layer.objects) {
      chunk_builder.addString(object.name);
      chunk_builder.addString(object.className);

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
      for (let point of object.polygon) {
        chunk_builder.addFloat32();
        chunk_builder.addFloat32();
      }
    }

    let chunk_writer = chunk_builder.build();
    for (let object of object_layer.objects) {
      chunk_writer.writeString(object.name);
      chunk_writer.writeString(object.className);

      chunk_writer.writeFloat32(object.x);
      chunk_writer.writeFloat32(object.y);

      chunk_writer.writeFloat32(object.width);
      chunk_writer.writeFloat32(object.height);

      let shape;
      switch (object.shape) {
        case MapObject.Rectangle:
          shape = 0;
          break
        case MapObject.Polygon:
          shape = 1;
          break;
        case MapObject.Polyline:
          shape = 2;
          break;
        case MapObject.Ellipse:
          shape = 3;
          break;
        case MapObject.Text:
          shape = 4;
          break;
        case MapObject.Point:
          shape = 5;
          break;
      };
      chunk_writer.writeUint32(shape);

      chunk_writer.writeUint32(object.polygon.length);
      for (let point of object.polygon) {
        chunk_writer.writeFloat32(point.x);
        chunk_writer.writeFloat32(point.y);
      }
    }
    writeChunk("ODAT", chunk_writer.finish(), file);
  }
}

/**
 * @param {GroupLayer} group_layer 
 * @param {BinaryFile} file 
 */
function write_group_layer_chunks(group_layer, file) {
  {
    let chunk_builder = new ChunkDataBuilder();
    chunk_builder.addString(group_layer.name);
    chunk_builder.addString(group_layer.className);
    chunk_builder.addUint32(); // number of layers in the group

    let chunk_writer = chunk_builder.build();
    chunk_writer.writeString(group_layer.name);
    chunk_writer.writeString(group_layer.className);
    chunk_writer.writeUint32(group_layer.layerCount);

    writeChunk("GLYR", chunk_writer.finish(), file);
  }

  {
    for (let layer of group_layer.layers) {
      write_layer(layer, file);
    }
  }
}

/**
 * @param {ImageLayer} image_layer
 * @param {BinaryFile} file
 */
function write_image_layer_chunks(image_layer, file) {
  let chunk_builder = new ChunkDataBuilder();
  chunk_builder.addString(image_layer.name);
  chunk_builder.addString(image_layer.className);
  chunk_builder.addString(image_layer.imageFileName);
  chunk_builder.addFloat32(); // parallax factor x
  chunk_builder.addFloat32(); // parallax factor y

  let chunk_writer = chunk_builder.build();
  chunk_writer.writeString(image_layer.name);
  chunk_writer.writeString(image_layer.className);
  chunk_writer.writeString(image_layer.imageFileName);
  chunk_writer.writeFloat32(image_layer.parallaxFactor.x);
  chunk_writer.writeFloat32(image_layer.parallaxFactor.y);

  writeChunk("ILYR", chunk_writer.finish(), file);
}

/**
 * @param {Layer} layer
 * @param {BinaryFile} file
 */
function write_layer(layer, file) {
  if (layer.isTileLayer)
    // @ts-ignore
    write_tile_layer_chunks(layer, file);

  if (layer.isObjectLayer)
    // @ts-ignore
    write_object_layer_chunks(layer, file);

  if (layer.isGroupLayer)
    // @ts-ignore
    write_group_layer_chunks(layer, file);

  if (layer.isImageLayer)
    // @ts-ignore
    write_image_layer_chunks(layer, file);

  // TODO figure out how to write tile layer properties
}


/**
 * @param {TileMap} map 
 * @param {string} filename 
 * @returns {undefined}
 */
function write_map(map, filename) {
  let file = new BinaryFile(filename, BinaryFile.WriteOnly);

  // write magic number
  // file only supports writing ArrayBuffer, so we need to convert the string to ArrayBuffer
  // also TextEncoder is not supported in Tiled, so we need to use the old method
  let buffer = new Uint8Array(4);
  for (let i = 0; i < 4; i++) {
    buffer[i] = "INFF".charCodeAt(i);
  }
  file.write(buffer.buffer);

  // we need to know the number of chunks we'll be writing, so we'll write a placeholder for now
  chunk_count = 0;
  let chunk_buffer = new Uint32Array(1);
  file.write(chunk_buffer.buffer);

  // construct map info chunk
  let map_info_buffer = new Uint32Array([
    map.width,
    map.height,
    map.layerCount
  ]);

  writeChunk("INFO", map_info_buffer.buffer, file);

  // write the tileset (there should only be one tileset)
  let tileset = map.tilesets[0];

  {
    let chunk_builder = new ChunkDataBuilder();
    chunk_builder.addString(tileset.name);
    chunk_builder.addString(tileset.imageFileName);

    let chunk_writer = chunk_builder.build();
    chunk_writer.writeString(tileset.name);
    chunk_writer.writeString(tileset.imageFileName);

    writeChunk("TSET", chunk_writer.finish(), file);
  }

  for (let layer of map.layers) {
    write_layer(layer, file);
  }

  // write the actual chunk count
  file.seek(4);
  chunk_buffer[0] = chunk_count;
  file.write(chunk_buffer.buffer);

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
  let id_buffer = new Uint8Array(4);
  for (let i = 0; i < 4; i++) {
    id_buffer[i] = chunk_id.charCodeAt(i);
  }
  file.write(id_buffer.buffer);

  // write chunk size
  let size = chunk_data.byteLength;
  let size_buffer = new Uint32Array([size]);
  file.write(size_buffer.buffer);

  // write chunk data
  file.write(chunk_data);

  chunk_count++;
}

class ChunkIter {
  constructor(chunks) {
    this.chunks = chunks;
    this.i = 0;
    this.done = false;
  }

  next() {
    if (this.i >= this.chunks.length) {
      this.done = true;
      return;
    }
    return this.chunks[this.i++];
  }
}

function read_map_layers(into, chunk_iter) {
  let chunk = chunk_iter.next();
  if (!chunk)
    return;
  let reader = new ChunkDataReader(chunk.data);

  switch (chunk.id) {
    case "TSET":
      let tileset = new Tileset();
      tileset.name = reader.readString();
      tileset.tileWidth = 8;
      tileset.tileHeight = 8;
      tileset.imageFileName = reader.readString();
      into.addTileset(tileset);
      break;
    case "TINF":
      let tile_layer = new TileLayer();
      tile_layer.name = reader.readString();
      tile_layer.className = reader.readString();
      tile_layer.parallaxFactor.x = reader.readFloat32();
      tile_layer.parallaxFactor.y = reader.readFloat32();

      tile_layer.width = into.width;
      tile_layer.height = into.height;

      chunk = chunk_iter.next();
      if (chunk.id !== "TDAT")
        throw new Error("Expected TDAT chunk");
      reader = new ChunkDataReader(chunk.data);

      let editor = tile_layer.edit();

      for (let y = 0; y < into.height; y++) {
        for (let x = 0; x < into.width; x++) {
          let tile_id = reader.readInt32();
          let tile = into.tilesets[0].findTile(tile_id);
          editor.setTile(x, y, tile);
        }
      }

      editor.apply();

      into.addLayer(tile_layer);
      break;
    case "OINF":
      let layer = new ObjectGroup();
      layer.name = reader.readString();
      layer.className = reader.readString();

      let object_count = reader.readUint32();

      chunk = chunk_iter.next();
      if (chunk.id !== "ODAT")
        throw new Error("Expected ODAT chunk");
      reader = new ChunkDataReader(chunk.data);

      for (let j = 0; j < object_count; j++) {
        let object = new MapObject();
        object.name = reader.readString();
        object.className = reader.readString();
        object.x = reader.readFloat32();
        object.y = reader.readFloat32();
        object.width = reader.readFloat32();
        object.height = reader.readFloat32();

        let shape = reader.readUint32();
        switch (shape) {
          case 0:
            object.shape = MapObject.Rectangle;
            break;
          case 1:
            object.shape = MapObject.Polygon;
            break;
          case 2:
            object.shape = MapObject.Polyline;
            break;
          case 3:
            object.shape = MapObject.Ellipse;
            break;
          case 4:
            object.shape = MapObject.Text;
            break;
          case 5:
            object.shape = MapObject.Point;
            break;
          default:
            throw new Error("Invalid shape");
        }

        let point_count = reader.readUint32();
        let polygons = [];
        for (let k = 0; k < point_count; k++) {
          polygons.push({ x: reader.readFloat32(), y: reader.readFloat32() });
        }
        object.polygon = polygons; // can't append to object.polygon directly

        tiled.warn(`Ty ${object.shape.toString()} Polygon ${polygons.toString()}`, () => { });

        layer.addObject(object);
      }

      into.addLayer(layer);
      break;
    case "GLYR":
      let group_layer = new GroupLayer();
      group_layer.name = reader.readString();
      group_layer.className = reader.readString();

      let layer_count = reader.readUint32();

      for (let i = 0; i < layer_count; i++) {
        read_map_layers(group_layer, chunk_iter);
      }

      into.addLayer(group_layer);
      break;
    case "ILYR":
      let image_layer = new ImageLayer();
      image_layer.name = reader.readString();
      image_layer.className = reader.readString();
      image_layer.imageFileName = reader.readString();
      image_layer.parallaxFactor.x = reader.readFloat32();
      image_layer.parallaxFactor.y = reader.readFloat32();

      into.addLayer(image_layer);
      break;
  }
}

/**
 * @param {string} filename 
 * @returns {TileMap}
 */
// @ts-ignore
// @ts-ignore
function read_map(filename) {
  let file = new BinaryFile(filename, BinaryFile.ReadOnly);
  let buffer = file.readAll();

  let chunk_reader = new ChunkDataReader(buffer);

  // read magic number
  let magic_number = chunk_reader.readId();
  if (magic_number !== "INFF")
    throw new Error(`Invalid magic number ${magic_number}`)

  // read chunk count
  let chunk_count = chunk_reader.readUint32();
  let chunks = [];

  for (let i = 0; i < chunk_count; i++) {
    let chunk_id = chunk_reader.readId();
    let chunk_size = chunk_reader.readUint32();
    let chunk_data = chunk_reader.readN(chunk_size);

    chunks.push({ id: chunk_id, data: chunk_data });
  }

  let map = new TileMap();

  let chunk_iter = new ChunkIter(chunks);

  let info_chunk = chunk_iter.next();
  if (info_chunk.id !== "INFO")
    throw new Error("Expected INFO chunk");
  let info_reader = new ChunkDataReader(info_chunk.data);
  map.width = info_reader.readUint32();
  map.height = info_reader.readUint32();
  map.tileWidth = 8;
  map.tileHeight = 8;

  while (!chunk_iter.done)
    read_map_layers(map, chunk_iter);

  return map;
}

let custom_format = {
  name: "Mnff's Not a File Format",
  extension: "mnff",

  read: read_map,
  write: write_map
};
tiled.registerMapFormat(custom_format.extension, custom_format);

/**
 * @param {Tileset} tileset
 * @param {string} filename
 * @return {undefined}
 */
function write_tileset(tileset, filename) {
  let file = new BinaryFile(filename, BinaryFile.WriteOnly);

  // write magic number
  let buffer = new Uint8Array(4);
  for (let i = 0; i < 4; i++) {
    buffer[i] = "INFF".charCodeAt(i);
  }
  file.write(buffer.buffer);

  // we'll only be writing one chunk
  let chunk_buffer = new Uint32Array([1]);
  file.write(chunk_buffer.buffer);

  {
    let chunk_builder = new ChunkDataBuilder();
    chunk_builder.addString(tileset.name);
    chunk_builder.addString(tileset.className);
    chunk_builder.addString(tileset.imageFileName);
    // tile width and height are fixed at 8

    let chunk_writer = chunk_builder.build();
    chunk_writer.writeString(tileset.name);
    chunk_writer.writeString(tileset.className);
    chunk_writer.writeString(tileset.imageFileName);

    writeChunk("TDEF", chunk_writer.finish(), file);
  }

  file.commit();
}

/**
 * @param {string} filename
 * @returns {Tileset}
 */
function read_tileset(filename) {
  let file = new BinaryFile(filename, BinaryFile.ReadOnly);
  let buffer = file.readAll();

  let chunk_reader = new ChunkDataReader(buffer);

  // read magic number
  let magic_number = chunk_reader.readId();
  if (magic_number !== "INFF")
    throw new Error(`Invalid magic number ${magic_number}`)

  // read chunk count
  let chunk_count = chunk_reader.readUint32();
  if (chunk_count !== 1)
    throw new Error("Invalid chunk count");

  let chunk_id = chunk_reader.readId();
  if (chunk_id !== "TDEF")
    throw new Error("Expected TDEF chunk");

  chunk_count = chunk_reader.readUint32();
  let chunk_data = chunk_reader.readN(chunk_count);

  let reader = new ChunkDataReader(chunk_data);

  let tileset = new Tileset();

  tileset.name = reader.readString();
  tileset.className = reader.readString();
  tileset.imageFileName = reader.readString();
  tileset.tileWidth = 8;
  tileset.tileHeight = 8;

  return tileset;
}

let custom_tileset_format = {
  name: "Tnff's Not a File Format",
  extension: "tnff",

  read: read_tileset,
  write: write_tileset
};
tiled.registerTilesetFormat(custom_tileset_format.extension, custom_tileset_format);