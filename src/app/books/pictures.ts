/**
 * Book covers and spines as the reader draws them.
 *
 * The reader stores pictures as "FBI1", width and height (uint16 LE), then
 * RGB565 pixels (uint16 LE) — storage/BookExtras.h. It draws them inside the
 * same frame as its default covers: rounded corners, a thin binding line near
 * the left edge of a cover, rules near the top and bottom of a spine and the
 * progress ribbon hanging from the top. The functions here reproduce that
 * frame so the crop template and preview in the app match the reader.
 */

import { PICTURE_SIZE, type PictureKind } from "../device/api";

// NanoScreens.cpp / AppNano.inl: the reader's colours for default covers.
const READER_COVER_COLORS = [0x99e3, 0x1af5, 0x0b6a, 0x7b98, 0x4490, 0xb4cd, 0x9a49, 0x32fa];

export function rgb565ToCss(value: number): string {
  const r = ((value >> 11) & 0x1f) * 255 / 31;
  const g = ((value >> 5) & 0x3f) * 255 / 63;
  const b = (value & 0x1f) * 255 / 31;
  return `rgb(${Math.round(r)}, ${Math.round(g)}, ${Math.round(b)})`;
}

/**
 * Colour the reader gives a book without a cover picture: FNV-1a of the
 * book's SD path without its extension (nanoCoverColor in AppNano.inl).
 * `name` is the library name from the reader ("books/Quo Vadis.epub").
 */
export function readerCoverColor(name: string): string {
  let key = `/books/${name}`;
  const slash = key.lastIndexOf("/");
  const dot = key.lastIndexOf(".");
  if (dot > slash) key = key.slice(0, dot);
  let hash = 2166136261;
  for (const byte of new TextEncoder().encode(key)) {
    hash = Math.imul(hash ^ byte, 16777619) >>> 0;
  }
  return rgb565ToCss(READER_COVER_COLORS[hash % READER_COVER_COLORS.length]);
}

/** Up to two capitals from the first words, Polish letters folded (nanoInitials). */
export function readerInitials(title: string): string {
  const folded = title
    .normalize("NFD")
    .replace(/[̀-ͯ]/g, "")
    .replace(/[łŁ]/g, "L");
  const words = folded.split(/[^A-Za-z0-9]+/).filter(Boolean);
  return words
    .slice(0, 2)
    .map((w) => w[0].toUpperCase())
    .join("");
}

/** Canvas pixels -> the reader's picture file. */
export function encodePicture(canvas: HTMLCanvasElement): Blob {
  const { width, height } = canvas;
  const pixels = canvas.getContext("2d")!.getImageData(0, 0, width, height).data;
  const out = new DataView(new ArrayBuffer(8 + width * height * 2));
  out.setUint8(0, 0x46); // F
  out.setUint8(1, 0x42); // B
  out.setUint8(2, 0x49); // I
  out.setUint8(3, 0x31); // 1
  out.setUint16(4, width, true);
  out.setUint16(6, height, true);
  for (let i = 0, o = 8; i < pixels.length; i += 4, o += 2) {
    const value = ((pixels[i] >> 3) << 11) | ((pixels[i + 1] >> 2) << 5) | (pixels[i + 2] >> 3);
    out.setUint16(o, value, true);
  }
  return new Blob([out.buffer], { type: "application/octet-stream" });
}

/** The reader's picture file -> canvas, null if it is not one. */
export async function decodePicture(blob: Blob): Promise<HTMLCanvasElement | null> {
  const view = new DataView(await blob.arrayBuffer());
  if (view.byteLength < 8 || view.getUint32(0, false) !== 0x46424931) return null;
  const width = view.getUint16(4, true);
  const height = view.getUint16(6, true);
  if (width === 0 || height === 0 || view.byteLength < 8 + width * height * 2) return null;
  const canvas = document.createElement("canvas");
  canvas.width = width;
  canvas.height = height;
  const ctx = canvas.getContext("2d")!;
  const image = ctx.createImageData(width, height);
  for (let i = 0, o = 8; i < width * height; i++, o += 2) {
    const value = view.getUint16(o, true);
    image.data[i * 4] = Math.round(((value >> 11) & 0x1f) * 255 / 31);
    image.data[i * 4 + 1] = Math.round(((value >> 5) & 0x3f) * 255 / 63);
    image.data[i * 4 + 2] = Math.round((value & 0x1f) * 255 / 31);
    image.data[i * 4 + 3] = 255;
  }
  ctx.putImageData(image, 0, 0);
  return canvas;
}

export interface CropState {
  /** Zoom on top of "just covers the frame", 1 = fills it. */
  zoom: number;
  /** Picture centre relative to the frame centre, as a fraction of the frame size. */
  panX: number;
  panY: number;
}

export const INITIAL_CROP: CropState = { zoom: 1, panX: 0, panY: 0 };

/** Where the picture lands in a frame of `frameW` x `frameH` px. */
export function placeImage(
  imageW: number,
  imageH: number,
  frameW: number,
  frameH: number,
  crop: CropState,
): { x: number; y: number; w: number; h: number } {
  const base = Math.max(frameW / imageW, frameH / imageH);
  const w = imageW * base * crop.zoom;
  const h = imageH * base * crop.zoom;
  return {
    x: frameW / 2 - w / 2 + crop.panX * frameW,
    y: frameH / 2 - h / 2 + crop.panY * frameH,
    w,
    h,
  };
}

/** Keeps the picture covering the whole frame (no empty edges). */
export function clampCrop(
  imageW: number,
  imageH: number,
  frameW: number,
  frameH: number,
  crop: CropState,
): CropState {
  const zoom = Math.min(8, Math.max(1, crop.zoom));
  const base = Math.max(frameW / imageW, frameH / imageH);
  const maxX = Math.max(0, (imageW * base * zoom - frameW) / 2 / frameW);
  const maxY = Math.max(0, (imageH * base * zoom - frameH) / 2 / frameH);
  return {
    zoom,
    panX: Math.min(maxX, Math.max(-maxX, crop.panX)),
    panY: Math.min(maxY, Math.max(-maxY, crop.panY)),
  };
}

/**
 * The crop at the reader's pixel size. Drawn large first and halved step by
 * step, so a photo shrunk to 92 px keeps its detail instead of aliasing.
 */
export function renderCrop(source: CanvasImageSource, imageW: number, imageH: number, kind: PictureKind, crop: CropState): HTMLCanvasElement {
  const { width, height } = PICTURE_SIZE[kind];
  let scale = 8;
  let canvas = document.createElement("canvas");
  canvas.width = width * scale;
  canvas.height = height * scale;
  let ctx = canvas.getContext("2d")!;
  ctx.imageSmoothingQuality = "high";
  const place = placeImage(imageW, imageH, canvas.width, canvas.height, crop);
  ctx.drawImage(source, place.x, place.y, place.w, place.h);
  while (scale > 1) {
    scale /= 2;
    const next = document.createElement("canvas");
    next.width = width * scale;
    next.height = height * scale;
    ctx = next.getContext("2d")!;
    ctx.imageSmoothingQuality = "high";
    ctx.drawImage(canvas, 0, 0, next.width, next.height);
    canvas = next;
  }
  return canvas;
}

// The object URL stays alive: the crop view creates new <img> elements with
// it every time the user switches between cover and spine. Call
// releaseImage() when the editor closes.
export async function loadImage(blob: Blob): Promise<HTMLImageElement> {
  const url = URL.createObjectURL(blob);
  const image = new Image();
  image.decoding = "async";
  image.src = url;
  try {
    await image.decode();
  } catch (err) {
    URL.revokeObjectURL(url);
    throw err;
  }
  return image;
}

export function releaseImage(image: HTMLImageElement | null): void {
  if (image?.src.startsWith("blob:")) URL.revokeObjectURL(image.src);
}
