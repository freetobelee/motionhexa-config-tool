#ifndef PATTERN_H
#define PATTERN_H

#include <vector>
#include <functional>
#include <optional>
#include <EEPROM.h>

#include <FastLED.h>

#include <util.h>
#include <paletting.h>
#include <patterning.h>

#include "ledgraph.h"
#include "drawing.h"
#include "MotionManager.h"
#include "hexaphysics.h"
#include "particles.h"
#include <phaser.h>

struct HexaShells {
  vector<vector<std::optional<PixelIndex> > > shells;

  HexaShells(PixelIndex center, int maxShellCount = 0) {
    // generate a series of hexashells centered around the given pixel
    Axial ax = axial.axialFromPixelIndex(center);
    int centerQ = ax.q();
    int centerR = ax.r();

    int shellCount = min((maxShellCount==0 ? kMeridian : maxShellCount), (kMeridian+1) / 2 + abs(centerQ) + abs(centerR));

    shells.emplace_back();
    shells.back().push_back(axial.indexAtAxial(centerQ, centerR)); // center px

    for (int s = 1; s < shellCount; ++s) {
      shells.emplace_back();
      int q = centerQ + s; // start each shell at q+shellnum to the right
      int r = centerR;
      // go counterclockwise around the shell
      for (int si = 0; si < s; ++si) {
        shells.back().push_back(axial.indexAtAxial(q, --r)); 
      }
      for (int si = 0; si < s; ++si) {
        shells.back().push_back(axial.indexAtAxial(--q, r)); 
      }
      for (int si = 0; si < s; ++si) {
        shells.back().push_back(axial.indexAtAxial(--q, ++r)); 
      }
      for (int si = 0; si < s; ++si) {
        shells.back().push_back(axial.indexAtAxial(q, ++r)); 
      }
      for (int si = 0; si < s; ++si) {
        shells.back().push_back(axial.indexAtAxial(++q, r)); 
      }
      for (int si = 0; si < s; ++si) {
        shells.back().push_back(axial.indexAtAxial(++q, --r)); 
      }
    }
  }
  HexaShells() {
    vector<PixelIndex> shellStarts = {0};
    // get a diagonal line from edge to center
    while (shellStarts.back() != kHexaCenterIndex) {
      shellStarts.push_back(hexGrid[shellStarts.back()]->named.dr->data());
    }
    for (int i = shellStarts.size() - 1; i >= 0; --i) {
      PixelIndex startIndex = shellStarts[i];
      PixelIndex index = startIndex;
      shells.emplace_back();
      while (1) {
        shells.back().push_back(index);
        vector<Edge> edges = ledgraph.adjacencies(index, MakeEdgeTypesQuad(EdgeType::clockwise));
        if (edges.size() == 1) {
          index = edges[0].to;
        } else {
          break;
        }
      };
    }
  }
};

// Shared hex-native pixel font, designed with the Hexa Font Forge tool and
// authored directly on a side-length-5 sub-hexagon (61 cells, 9 rows of
// widths 5,6,7,8,9,8,7,6,5) rather than a rectangular bitmap -- so a glyph is
// itself hexagon-shaped, matching the board it's drawn on. kHexGlyphCellQR
// gives the axial (q,r) offset for row-major cell index i (row r=-4..4, left
// to right within the row); each kFont_* is that same row layout as 9 binary
// values. To actually draw one: for each set bit, take its cell's (q,r),
// add wherever the glyph should be centered, optionally rotate (an exact
// integer multiple of 60deg only -- see any pattern's own rotation logic for
// why), and light the resulting pixel if it's on the board.
static const int8_t kHexGlyphCellQR[61][2] = {
  {0,-4},{1,-4},{2,-4},{3,-4},{4,-4},{-1,-3},{0,-3},{1,-3},{2,-3},{3,-3},
  {4,-3},{-2,-2},{-1,-2},{0,-2},{1,-2},{2,-2},{3,-2},{4,-2},{-3,-1},{-2,-1},
  {-1,-1},{0,-1},{1,-1},{2,-1},{3,-1},{4,-1},{-4,0},{-3,0},{-2,0},{-1,0},
  {0,0},{1,0},{2,0},{3,0},{4,0},{-4,1},{-3,1},{-2,1},{-1,1},{0,1},
  {1,1},{2,1},{3,1},{-4,2},{-3,2},{-2,2},{-1,2},{0,2},{1,2},{2,2},
  {-4,3},{-3,3},{-2,3},{-1,3},{0,3},{1,3},{-4,4},{-3,4},{-2,4},{-1,4},
  {0,4}
};
static const int8_t kHexGlyphRowStart[9] = {0,5,11,18,26,35,43,50,56};
static const int8_t kHexGlyphRowWidth[9] = {5,6,7,8,9,8,7,6,5};

const uint16_t kFont_D0[9] = {0b00000,0b001100,0b0010100,0b00100100,0b001000100,0b00100100,0b0010100,0b001100,0b00000}; // '0'
const uint16_t kFont_D1[9] = {0b00000,0b000100,0b0001000,0b00001000,0b000010000,0b00001000,0b0001000,0b000100,0b00000}; // '1'
const uint16_t kFont_D2[9] = {0b00000,0b001100,0b0010100,0b00000100,0b000001000,0b00011000,0b0010000,0b011110,0b00000}; // '2'
const uint16_t kFont_D3[9] = {0b00000,0b001100,0b0010100,0b00000100,0b000011000,0b00000100,0b0010100,0b001100,0b00000}; // '3'
const uint16_t kFont_D4[9] = {0b00000,0b000100,0b0001000,0b00010000,0b000101000,0b00111100,0b0000100,0b000100,0b00000}; // '4'
const uint16_t kFont_D5[9] = {0b00000,0b001110,0b0010000,0b00100000,0b000111000,0b00000100,0b0000100,0b011100,0b00000}; // '5'
const uint16_t kFont_D6[9] = {0b00000,0b001100,0b0010100,0b00100000,0b000111000,0b00100100,0b0010100,0b001100,0b00000}; // '6'
const uint16_t kFont_D7[9] = {0b00000,0b011110,0b0000010,0b00000100,0b000001000,0b00001000,0b0001000,0b001000,0b00000}; // '7'
const uint16_t kFont_D8[9] = {0b00000,0b001100,0b0010100,0b00100100,0b000111000,0b00100100,0b0010100,0b001100,0b00000}; // '8'
const uint16_t kFont_D9[9] = {0b00000,0b001100,0b0010100,0b00100100,0b000111000,0b00000100,0b0010100,0b001100,0b00000}; // '9'
const uint16_t kFont_COLON[9] = {0b00000,0b000000,0b0001000,0b00000000,0b000000000,0b00000000,0b0001000,0b000000,0b00000}; // ':'
const uint16_t kFont_SEMICOLON[9] = {0b00000,0b000000,0b0001000,0b00000000,0b000000000,0b00000000,0b0001000,0b001000,0b00000}; // ';'
const uint16_t kFont_BANG[9] = {0b00000,0b000100,0b0000100,0b00001000,0b000001000,0b00001000,0b0000000,0b000100,0b00000}; // '!'
const uint16_t kFont_QUESTION[9] = {0b00000,0b001100,0b0010100,0b00000100,0b000011000,0b00010000,0b0000000,0b001000,0b00000}; // '?'
const uint16_t kFont_AMP[9] = {0b00000,0b000100,0b0001000,0b00001000,0b001111110,0b00001000,0b0001000,0b000100,0b00000}; // '&'
const uint16_t kFont_BACKSLASH[9] = {0b00000,0b010000,0b0010000,0b00010000,0b000010000,0b00001000,0b0000100,0b000010,0b00000}; // '\\'
const uint16_t kFont_SLASH[9] = {0b00000,0b000010,0b0000100,0b00001000,0b000010000,0b00010000,0b0010000,0b010000,0b00000}; // '/'
// hand-designed placeholders (not yet made in the Hexa Font Forge tool) for
// the letters "ready?" needs -- swap these out once you've designed them
const uint16_t kFont_R[9] = {0b00000,0b011110,0b0100010,0b00100010,0b001111000,0b00101000,0b0100100,0b010001,0b00000}; // 'R'
const uint16_t kFont_E[9] = {0b00000,0b011111,0b0100000,0b00100000,0b001111000,0b00100000,0b0100000,0b011111,0b00000}; // 'E'
const uint16_t kFont_A[9] = {0b00000,0b001110,0b0100010,0b00100010,0b001111100,0b00100010,0b0100010,0b010001,0b00000}; // 'A'
const uint16_t kFont_D[9] = {0b00000,0b011110,0b0100010,0b00100010,0b001000100,0b00100010,0b0100010,0b011110,0b00000}; // 'D'
const uint16_t kFont_Y[9] = {0b00000,0b010001,0b0100010,0b00010100,0b000010000,0b00001000,0b0001000,0b000100,0b00000}; // 'Y'

const uint16_t *hexFontGlyphForChar(char c) {
  switch (c) {
    case '0': return kFont_D0;
    case '1': return kFont_D1;
    case '2': return kFont_D2;
    case '3': return kFont_D3;
    case '4': return kFont_D4;
    case '5': return kFont_D5;
    case '6': return kFont_D6;
    case '7': return kFont_D7;
    case '8': return kFont_D8;
    case '9': return kFont_D9;
    case ':': return kFont_COLON;
    case ';': return kFont_SEMICOLON;
    case '!': return kFont_BANG;
    case '?': return kFont_QUESTION;
    case '&': return kFont_AMP;
    case '\\': return kFont_BACKSLASH;
    case '/': return kFont_SLASH;
    case 'R': return kFont_R;
    case 'E': return kFont_E;
    case 'A': return kFont_A;
    case 'D': return kFont_D;
    case 'Y': return kFont_Y;
    default:  return kFont_D0;
  }
}

// rotates an axial cell by exactly steps*60deg CCW in rect space (matching
// the same rect-rotation convention drawLocalPixel-style helpers use
// elsewhere): one step is (q,r,s) -> (-s,-q,-r). Exact integer arithmetic,
// no rounding, which is why the hex-native font is rotated this way instead
// of going through rectToHex/cubeRound like rectangular glyphs do.
void rotateAxialSteps(int &q, int &r, int steps) {
  for (int i = 0; i < steps; ++i) {
    int s = -q - r;
    int newQ = -s, newR = -q;
    q = newQ; r = newR;
  }
}

// draws one hex-native glyph (see kFont_D0 etc. and kHexGlyphCellQR above)
// centered originQ,originR cells away from the hexa's center, rotated by an
// exact number of 60deg steps -- shared across any pattern using this font
void drawHexGlyphSteps(PixelStorage<LED_COUNT> &ctx, const uint16_t *rows, int originQ, int originR, int rotSteps, CRGB color) {
  for (int rowIdx = 0; rowIdx < 9; ++rowIdx) {
    int start = kHexGlyphRowStart[rowIdx], width = kHexGlyphRowWidth[rowIdx];
    uint16_t bits = rows[rowIdx];
    for (int c = 0; c < width; ++c) {
      if (!((bits >> (width - 1 - c)) & 1)) continue;
      int q = kHexGlyphCellQR[start + c][0] + originQ;
      int r = kHexGlyphCellQR[start + c][1] + originR;
      rotateAxialSteps(q, r, rotSteps);
      auto pxOpt = axial.indexAtAxial(q, r);
      if (pxOpt.has_value()) {
        ctx.leds[pxOpt.value()] = color;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Bitmask-encoded hex objects (fonts + elements), authored in the "Hexa
// Object Forge" web tool and exported as compact per-pixel bitmasks rather
// than coordinate lists: bit i of an object's mask is cell i of the matching
// kHexCellQR_* table below. That table's cell order comes from the tool's own
// enumeration (by increasing q, not by row), so it does NOT line up with the
// older kHexGlyphCellQR table above -- these are two independent, parallel
// systems, not a shared one. Decode a bit with (mask[i>>3] >> (i&7)) & 1, then
// place/rotate it exactly like drawHexGlyphSteps does; see
// drawHexBitmaskSteps below. No PROGMEM here -- unlike AVR, this RP2040
// target already keeps const data in flash without it, and nothing else in
// this file uses it either.
static const int8_t kHexCellQR_XS[61][2] = {
  {-4,0}, {-4,1}, {-4,2}, {-4,3}, {-4,4}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3},
  {-3,4}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, {-2,4}, {-1,-3}, {-1,-2},
  {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, {0,-4}, {0,-3}, {0,-2}, {0,-1},
  {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0},
  {1,1}, {1,2}, {1,3}, {2,-4}, {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2},
  {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {4,-4}, {4,-3}, {4,-2}, {4,-1},
  {4,0}
};

static const int8_t kHexCellQR_MD[127][2] = {
  {-6,0}, {-6,1}, {-6,2}, {-6,3}, {-6,4}, {-6,5}, {-6,6}, {-5,-1}, {-5,0}, {-5,1},
  {-5,2}, {-5,3}, {-5,4}, {-5,5}, {-5,6}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2},
  {-4,3}, {-4,4}, {-4,5}, {-4,6}, {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2},
  {-3,3}, {-3,4}, {-3,5}, {-3,6}, {-2,-4}, {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1},
  {-2,2}, {-2,3}, {-2,4}, {-2,5}, {-2,6}, {-1,-5}, {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1},
  {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, {-1,5}, {-1,6}, {0,-6}, {0,-5}, {0,-4},
  {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6},
  {1,-6}, {1,-5}, {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3},
  {1,4}, {1,5}, {2,-6}, {2,-5}, {2,-4}, {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1},
  {2,2}, {2,3}, {2,4}, {3,-6}, {3,-5}, {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0},
  {3,1}, {3,2}, {3,3}, {4,-6}, {4,-5}, {4,-4}, {4,-3}, {4,-2}, {4,-1}, {4,0},
  {4,1}, {4,2}, {5,-6}, {5,-5}, {5,-4}, {5,-3}, {5,-2}, {5,-1}, {5,0}, {5,1},
  {6,-6}, {6,-5}, {6,-4}, {6,-3}, {6,-2}, {6,-1}, {6,0}
};

static const int8_t kHexCellQR_LG[271][2] = {
  {-9,0}, {-9,1}, {-9,2}, {-9,3}, {-9,4}, {-9,5}, {-9,6}, {-9,7}, {-9,8}, {-9,9},
  {-8,-1}, {-8,0}, {-8,1}, {-8,2}, {-8,3}, {-8,4}, {-8,5}, {-8,6}, {-8,7}, {-8,8},
  {-8,9}, {-7,-2}, {-7,-1}, {-7,0}, {-7,1}, {-7,2}, {-7,3}, {-7,4}, {-7,5}, {-7,6},
  {-7,7}, {-7,8}, {-7,9}, {-6,-3}, {-6,-2}, {-6,-1}, {-6,0}, {-6,1}, {-6,2}, {-6,3},
  {-6,4}, {-6,5}, {-6,6}, {-6,7}, {-6,8}, {-6,9}, {-5,-4}, {-5,-3}, {-5,-2}, {-5,-1},
  {-5,0}, {-5,1}, {-5,2}, {-5,3}, {-5,4}, {-5,5}, {-5,6}, {-5,7}, {-5,8}, {-5,9},
  {-4,-5}, {-4,-4}, {-4,-3}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2}, {-4,3}, {-4,4},
  {-4,5}, {-4,6}, {-4,7}, {-4,8}, {-4,9}, {-3,-6}, {-3,-5}, {-3,-4}, {-3,-3}, {-3,-2},
  {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, {-3,4}, {-3,5}, {-3,6}, {-3,7}, {-3,8},
  {-3,9}, {-2,-7}, {-2,-6}, {-2,-5}, {-2,-4}, {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1},
  {-2,2}, {-2,3}, {-2,4}, {-2,5}, {-2,6}, {-2,7}, {-2,8}, {-2,9}, {-1,-8}, {-1,-7},
  {-1,-6}, {-1,-5}, {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3},
  {-1,4}, {-1,5}, {-1,6}, {-1,7}, {-1,8}, {-1,9}, {0,-9}, {0,-8}, {0,-7}, {0,-6},
  {0,-5}, {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4},
  {0,5}, {0,6}, {0,7}, {0,8}, {0,9}, {1,-9}, {1,-8}, {1,-7}, {1,-6}, {1,-5},
  {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5},
  {1,6}, {1,7}, {1,8}, {2,-9}, {2,-8}, {2,-7}, {2,-6}, {2,-5}, {2,-4}, {2,-3},
  {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, {2,5}, {2,6}, {2,7},
  {3,-9}, {3,-8}, {3,-7}, {3,-6}, {3,-5}, {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0},
  {3,1}, {3,2}, {3,3}, {3,4}, {3,5}, {3,6}, {4,-9}, {4,-8}, {4,-7}, {4,-6},
  {4,-5}, {4,-4}, {4,-3}, {4,-2}, {4,-1}, {4,0}, {4,1}, {4,2}, {4,3}, {4,4},
  {4,5}, {5,-9}, {5,-8}, {5,-7}, {5,-6}, {5,-5}, {5,-4}, {5,-3}, {5,-2}, {5,-1},
  {5,0}, {5,1}, {5,2}, {5,3}, {5,4}, {6,-9}, {6,-8}, {6,-7}, {6,-6}, {6,-5},
  {6,-4}, {6,-3}, {6,-2}, {6,-1}, {6,0}, {6,1}, {6,2}, {6,3}, {7,-9}, {7,-8},
  {7,-7}, {7,-6}, {7,-5}, {7,-4}, {7,-3}, {7,-2}, {7,-1}, {7,0}, {7,1}, {7,2},
  {8,-9}, {8,-8}, {8,-7}, {8,-6}, {8,-5}, {8,-4}, {8,-3}, {8,-2}, {8,-1}, {8,0},
  {8,1}, {9,-9}, {9,-8}, {9,-7}, {9,-6}, {9,-5}, {9,-4}, {9,-3}, {9,-2}, {9,-1},
  {9,0}
};

void drawHexBitmaskSteps(PixelStorage<LED_COUNT> &ctx, const uint8_t *mask, const int8_t cellQR[][2], int cellCount, int originQ, int originR, int rotSteps, CRGB color) {
  for (int i = 0; i < cellCount; ++i) {
    if (!((mask[i >> 3] >> (i & 7)) & 1)) continue;
    int q = cellQR[i][0] + originQ;
    int r = cellQR[i][1] + originR;
    rotateAxialSteps(q, r, rotSteps);
    auto pxOpt = axial.indexAtAxial(q, r);
    if (pxOpt.has_value()) {
      ctx.leds[pxOpt.value()] = color;
    }
  }
}

// gets the real ink bounds (min/max local q) of a bitmask glyph, same idea as
// the old row-encoded hexGlyphQBounds -- used to kern two glyphs off their
// actual lit pixels rather than the full canvas they're drawn on
void hexBitmaskQBounds(const uint8_t *mask, const int8_t cellQR[][2], int cellCount, int &minQ, int &maxQ) {
  minQ = 100;
  maxQ = -100;
  for (int i = 0; i < cellCount; ++i) {
    if (!((mask[i >> 3] >> (i & 7)) & 1)) continue;
    int q = cellQR[i][0];
    if (q < minQ) minQ = q;
    if (q > maxQ) maxQ = q;
  }
  if (minQ > maxQ) { minQ = 0; maxQ = 0; } // blank glyph guard
}

// Small (61px) digit glyphs -- two of these kern cleanly side by side, unlike
// Medium/Large, so this is what a 2-digit countdown uses.
const uint8_t kFont_XS_0[8] = { 0x80, 0x33, 0x09, 0x09, 0x12, 0x92, 0x39, 0x00 };
const uint8_t kFont_XS_1[8] = { 0x00, 0x00, 0x80, 0xC1, 0x60, 0x10, 0x00, 0x00 };
const uint8_t kFont_XS_2[8] = { 0x00, 0x43, 0x49, 0x89, 0x12, 0x91, 0x38, 0x00 };
const uint8_t kFont_XS_3[8] = { 0x00, 0x03, 0x09, 0x49, 0x92, 0x92, 0x39, 0x00 };
const uint8_t kFont_XS_4[8] = { 0x80, 0x60, 0x50, 0x90, 0x93, 0x71, 0x09, 0x00 };
const uint8_t kFont_XS_5[8] = { 0x00, 0x13, 0x19, 0x29, 0x52, 0xD2, 0x09, 0x00 };
const uint8_t kFont_XS_6[8] = { 0x80, 0x33, 0x29, 0x49, 0x92, 0x92, 0x19, 0x00 };
const uint8_t kFont_XS_7[8] = { 0x00, 0x00, 0x89, 0x88, 0x90, 0x50, 0x18, 0x00 };
const uint8_t kFont_XS_8[8] = { 0x80, 0x33, 0x29, 0x49, 0x92, 0x92, 0x39, 0x00 };
const uint8_t kFont_XS_9[8] = { 0x00, 0x33, 0x29, 0x49, 0x92, 0x92, 0x39, 0x00 };

const uint8_t *hexBitmaskDigitXS(int d) {
  static const uint8_t *digits[10] = { kFont_XS_0, kFont_XS_1, kFont_XS_2, kFont_XS_3, kFont_XS_4, kFont_XS_5, kFont_XS_6, kFont_XS_7, kFont_XS_8, kFont_XS_9 };
  return digits[((d % 10) + 10) % 10];
}

// Small (61px) uppercase letters + '?' -- for multi-character words (affirmations,
// idle prompt) rather than single digits.
const uint8_t kFont_XS_SPACE[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t kFont_XS_QMARK[8] = { 0x00, 0x00, 0x49, 0x48, 0x90, 0x90, 0x38, 0x00 };
const uint8_t kFont_XS_A[8] = { 0x80, 0x21, 0x30, 0x50, 0x90, 0xF2, 0x01, 0x00 };
const uint8_t kFont_XS_B[8] = { 0x00, 0x63, 0x39, 0x49, 0x92, 0xD2, 0x19, 0x00 };
const uint8_t kFont_XS_C[8] = { 0x80, 0x33, 0x09, 0x09, 0x12, 0x12, 0x18, 0x00 };
const uint8_t kFont_XS_D[8] = { 0x00, 0x63, 0x19, 0x09, 0x11, 0xB2, 0x31, 0x00 };
const uint8_t kFont_XS_E[8] = { 0x00, 0x43, 0x39, 0x49, 0x92, 0x10, 0x08, 0x00 };
const uint8_t kFont_XS_F[8] = { 0x00, 0x63, 0x38, 0x48, 0x90, 0x10, 0x08, 0x00 };
const uint8_t kFont_XS_G[8] = { 0x80, 0x33, 0x09, 0x49, 0x92, 0x92, 0x09, 0x00 };
const uint8_t kFont_XS_H[8] = { 0x00, 0x63, 0x38, 0x48, 0x82, 0xC3, 0x18, 0x00 };
const uint8_t kFont_XS_I[8] = { 0x00, 0x02, 0xC1, 0x69, 0x32, 0x10, 0x08, 0x00 };
const uint8_t kFont_XS_J[8] = { 0x00, 0x03, 0x01, 0x09, 0xF1, 0x11, 0x08, 0x00 };
const uint8_t kFont_XS_K[8] = { 0x00, 0x63, 0x38, 0xC8, 0x43, 0x20, 0x08, 0x00 };
const uint8_t kFont_XS_L[8] = { 0x00, 0x63, 0x19, 0x09, 0x02, 0x00, 0x00, 0x00 };
const uint8_t kFont_XS_M[8] = { 0x80, 0x33, 0x08, 0x78, 0x42, 0xA2, 0x39, 0x00 };
const uint8_t kFont_XS_N[8] = { 0x80, 0x33, 0x08, 0xF8, 0x03, 0x82, 0x39, 0x00 };
const uint8_t kFont_XS_O[8] = { 0x80, 0x33, 0x09, 0x09, 0x12, 0x92, 0x39, 0x00 };
const uint8_t kFont_XS_P[8] = { 0x00, 0x63, 0x38, 0x48, 0x90, 0x90, 0x38, 0x00 };
const uint8_t kFont_XS_Q[8] = { 0x80, 0x33, 0x09, 0x89, 0x11, 0xF3, 0x00, 0x00 };
const uint8_t kFont_XS_R[8] = { 0x00, 0x63, 0x78, 0xC8, 0x93, 0x90, 0x38, 0x00 };
const uint8_t kFont_XS_S[8] = { 0x00, 0x33, 0x29, 0x49, 0x92, 0x92, 0x19, 0x00 };
const uint8_t kFont_XS_T[8] = { 0x00, 0x00, 0x80, 0xC9, 0x70, 0x10, 0x08, 0x00 };
const uint8_t kFont_XS_U[8] = { 0x80, 0x33, 0x09, 0x09, 0x02, 0x82, 0x39, 0x00 };
const uint8_t kFont_XS_V[8] = { 0x00, 0xF0, 0x09, 0x01, 0x01, 0x81, 0x30, 0x00 };
const uint8_t kFont_XS_W[8] = { 0x80, 0xB3, 0x48, 0xC0, 0x03, 0x82, 0x31, 0x00 };
const uint8_t kFont_XS_X[8] = { 0x00, 0x82, 0x40, 0xF8, 0x43, 0x20, 0x08, 0x00 };
const uint8_t kFont_XS_Y[8] = { 0x00, 0x00, 0x80, 0xF9, 0x40, 0x20, 0x08, 0x00 };
const uint8_t kFont_XS_Z[8] = { 0x00, 0x82, 0x41, 0x49, 0x52, 0x30, 0x08, 0x00 };

// Small (61px) -- additional punctuation, lowercase, and degree glyphs
const uint8_t kFont_XS_BANG[8] = { 0x00, 0x00, 0x00, 0xC1, 0x60, 0x10, 0x00, 0x00 }; // '!'
const uint8_t kFont_XS_AMP[8] = { 0x00, 0x20, 0xE0, 0x40, 0xE0, 0x80, 0x00, 0x00 }; // '&'
const uint8_t kFont_XS_APOS[8] = { 0x00, 0x00, 0x00, 0x00, 0x20, 0x10, 0x00, 0x00 }; // '''
const uint8_t kFont_XS_LPAREN[8] = { 0x00, 0xE0, 0x11, 0x11, 0x10, 0x10, 0x00, 0x00 }; // '('
const uint8_t kFont_XS_RPAREN[8] = { 0x00, 0x00, 0x01, 0x01, 0x11, 0xF1, 0x00, 0x00 }; // ')'
const uint8_t kFont_XS_PLUS[8] = { 0x00, 0x20, 0xE0, 0x40, 0xE0, 0x80, 0x00, 0x00 }; // '+'
const uint8_t kFont_XS_COMMA[8] = { 0x00, 0x00, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00 }; // ','
const uint8_t kFont_XS_DASH[8] = { 0x00, 0x20, 0x20, 0x40, 0x80, 0x80, 0x00, 0x00 }; // '-'
const uint8_t kFont_XS_PERIOD[8] = { 0x00, 0x00, 0x81, 0x01, 0x00, 0x00, 0x00, 0x00 }; // '.'
const uint8_t kFont_XS_SLASH[8] = { 0x00, 0x82, 0x40, 0x40, 0x40, 0x20, 0x08, 0x00 }; // '/'
const uint8_t kFont_XS_COLON[8] = { 0x00, 0x00, 0x80, 0x00, 0x20, 0x00, 0x00, 0x00 }; // ':'
const uint8_t kFont_XS_SEMI[8] = { 0x00, 0x00, 0x81, 0x00, 0x20, 0x00, 0x00, 0x00 }; // ';'
const uint8_t kFont_XS_EQUALS[8] = { 0x80, 0x50, 0x50, 0xA0, 0x40, 0x41, 0x21, 0x00 }; // '='
const uint8_t kFont_XS_a[8] = { 0x80, 0x03, 0x21, 0x51, 0xA1, 0xA6, 0x21, 0x00 }; // 'a'
const uint8_t kFont_XS_b[8] = { 0x88, 0x33, 0x1D, 0x21, 0x42, 0xC2, 0x01, 0x00 }; // 'b'
const uint8_t kFont_XS_c[8] = { 0x80, 0x23, 0x11, 0x21, 0x42, 0x44, 0x20, 0x00 }; // 'c'
const uint8_t kFont_XS_d[8] = { 0x80, 0x23, 0x11, 0x21, 0x42, 0xC6, 0x31, 0x02 }; // 'd'
const uint8_t kFont_XS_e[8] = { 0x80, 0x33, 0x21, 0x51, 0xA2, 0xA4, 0x20, 0x00 }; // 'e'
const uint8_t kFont_XS_f[8] = { 0x00, 0xD2, 0x30, 0x30, 0x50, 0x50, 0x08, 0x00 }; // 'f'
const uint8_t kFont_XS_g[8] = { 0x00, 0x72, 0x41, 0x91, 0x22, 0xA3, 0x31, 0x00 }; // 'g'
const uint8_t kFont_XS_h[8] = { 0x88, 0x31, 0x1C, 0x20, 0x40, 0xC6, 0x01, 0x00 }; // 'h'
const uint8_t kFont_XS_i[8] = { 0x00, 0x02, 0xD1, 0xE1, 0x52, 0x10, 0x00, 0x00 }; // 'i'
const uint8_t kFont_XS_j[8] = { 0x00, 0x03, 0x01, 0x21, 0xC1, 0x41, 0x08, 0x00 }; // 'j'
const uint8_t kFont_XS_k[8] = { 0x88, 0x71, 0x8C, 0x40, 0x02, 0x40, 0x00, 0x00 }; // 'k'
const uint8_t kFont_XS_l[8] = { 0x00, 0x02, 0xC1, 0xE9, 0x72, 0x10, 0x00, 0x00 }; // 'l'
const uint8_t kFont_XS_m[8] = { 0x88, 0x31, 0x48, 0xF0, 0x40, 0xA6, 0x21, 0x00 }; // 'm'
const uint8_t kFont_XS_n[8] = { 0x88, 0x31, 0x18, 0x00, 0x20, 0xA6, 0x21, 0x00 }; // 'n'
const uint8_t kFont_XS_o[8] = { 0x80, 0x33, 0x01, 0x11, 0x22, 0xA2, 0x21, 0x00 }; // 'o'
const uint8_t kFont_XS_p[8] = { 0x88, 0x71, 0x48, 0x90, 0x20, 0xA1, 0x20, 0x00 }; // 'p'
const uint8_t kFont_XS_q[8] = { 0x00, 0x70, 0x40, 0x90, 0x20, 0xA7, 0x31, 0x00 }; // 'q'
const uint8_t kFont_XS_r[8] = { 0x88, 0x31, 0x18, 0x00, 0x20, 0x20, 0x20, 0x00 }; // 'r'
const uint8_t kFont_XS_s[8] = { 0x08, 0x12, 0x21, 0x51, 0xA2, 0x22, 0x11, 0x00 }; // 's'
const uint8_t kFont_XS_t[8] = { 0x00, 0x00, 0xC0, 0xF0, 0x72, 0x34, 0x00, 0x00 }; // 't'
const uint8_t kFont_XS_u[8] = { 0x80, 0x33, 0x09, 0x01, 0x01, 0x86, 0x31, 0x00 }; // 'u'
const uint8_t kFont_XS_v[8] = { 0x80, 0xB0, 0x09, 0x01, 0x01, 0x80, 0x31, 0x00 }; // 'v'
const uint8_t kFont_XS_w[8] = { 0x80, 0x33, 0xC8, 0xC0, 0x02, 0x82, 0x31, 0x00 }; // 'w'
const uint8_t kFont_XS_x[8] = { 0x08, 0x41, 0x18, 0x40, 0x00, 0x47, 0x10, 0x00 }; // 'x'
const uint8_t kFont_XS_y[8] = { 0x00, 0x72, 0x49, 0x81, 0x02, 0x83, 0x31, 0x00 }; // 'y'
const uint8_t kFont_XS_z[8] = { 0x08, 0x43, 0x09, 0x51, 0x22, 0x64, 0x10, 0x00 }; // 'z'
const uint8_t kFont_XS_DEGREE[8] = { 0x00, 0x00, 0x00, 0x30, 0x50, 0x30, 0x00, 0x00 }; // '°'

const uint8_t *hexBitmaskGlyphForChar(char c) {
  switch (c) {
    case ' ': return kFont_XS_SPACE;
    case '?': return kFont_XS_QMARK;
    case 'A': return kFont_XS_A; case 'B': return kFont_XS_B; case 'C': return kFont_XS_C;
    case 'D': return kFont_XS_D; case 'E': return kFont_XS_E; case 'F': return kFont_XS_F;
    case 'G': return kFont_XS_G; case 'H': return kFont_XS_H; case 'I': return kFont_XS_I;
    case 'J': return kFont_XS_J; case 'K': return kFont_XS_K; case 'L': return kFont_XS_L;
    case 'M': return kFont_XS_M; case 'N': return kFont_XS_N; case 'O': return kFont_XS_O;
    case 'P': return kFont_XS_P; case 'Q': return kFont_XS_Q; case 'R': return kFont_XS_R;
    case 'S': return kFont_XS_S; case 'T': return kFont_XS_T; case 'U': return kFont_XS_U;
    case 'V': return kFont_XS_V; case 'W': return kFont_XS_W; case 'X': return kFont_XS_X;
    case 'Y': return kFont_XS_Y; case 'Z': return kFont_XS_Z;
    case 'a': return kFont_XS_a; case 'b': return kFont_XS_b; case 'c': return kFont_XS_c;
    case 'd': return kFont_XS_d; case 'e': return kFont_XS_e; case 'f': return kFont_XS_f;
    case 'g': return kFont_XS_g; case 'h': return kFont_XS_h; case 'i': return kFont_XS_i;
    case 'j': return kFont_XS_j; case 'k': return kFont_XS_k; case 'l': return kFont_XS_l;
    case 'm': return kFont_XS_m; case 'n': return kFont_XS_n; case 'o': return kFont_XS_o;
    case 'p': return kFont_XS_p; case 'q': return kFont_XS_q; case 'r': return kFont_XS_r;
    case 's': return kFont_XS_s; case 't': return kFont_XS_t; case 'u': return kFont_XS_u;
    case 'v': return kFont_XS_v; case 'w': return kFont_XS_w; case 'x': return kFont_XS_x;
    case 'y': return kFont_XS_y; case 'z': return kFont_XS_z;
    case '!': return kFont_XS_BANG; case '&': return kFont_XS_AMP; case '\'': return kFont_XS_APOS;
    case '(': return kFont_XS_LPAREN; case ')': return kFont_XS_RPAREN; case '+': return kFont_XS_PLUS;
    case ',': return kFont_XS_COMMA; case '-': return kFont_XS_DASH; case '.': return kFont_XS_PERIOD;
    case '/': return kFont_XS_SLASH; case ':': return kFont_XS_COLON; case ';': return kFont_XS_SEMI;
    case '=': return kFont_XS_EQUALS;
    // kFont_XS_DEGREE exists but has no case here -- '°' isn't representable
    // as a single ASCII char, so it's not reachable through this char-keyed switch
    default: return kFont_XS_SPACE;
  }
}

// total local-q width a word would take at Small size -- glyphs are placed
// right after one another with no extra gap or overlap, since each glyph's
// own pixel layout already has kerning baked in
float bitmaskWordWidth(const char *word, int wordLen) {
  const float kGap = 0.0f;
  float w = 0;
  for (int i = 0; i < wordLen; ++i) {
    if (word[i] == ' ') { w += 3.0f; continue; }
    int minQ, maxQ;
    hexBitmaskQBounds(hexBitmaskGlyphForChar(word[i]), kHexCellQR_XS, 61, minQ, maxQ);
    w += (maxQ - minQ) + kGap;
  }
  return w;
}

void drawBitmaskWordAt(PixelStorage<LED_COUNT> &ctx, const char *word, int wordLen, float startQ, int startR, int rotSteps, CRGB color) {
  const float kGap = 0.0f;
  float cursor = startQ;
  for (int i = 0; i < wordLen; ++i) {
    if (word[i] == ' ') { cursor += 3.0f; continue; }
    const uint8_t *mask = hexBitmaskGlyphForChar(word[i]);
    int minQ, maxQ;
    hexBitmaskQBounds(mask, kHexCellQR_XS, 61, minQ, maxQ);
    int origin = (int)roundf(cursor - minQ);
    drawHexBitmaskSteps(ctx, mask, kHexCellQR_XS, 61, origin, startR, rotSteps, color);
    cursor += (maxQ - minQ) + kGap;
  }
}

// one-shot scroll, off the right edge to off the left edge over durationMs --
// same technique WorkoutTimer's old rect-font GOOD/JOB scroll used, just
// reading the bitmask font and its exact-integer axial rotation instead
void drawScrollingBitmaskWord(PixelStorage<LED_COUNT> &ctx, const char *word, int wordLen, float elapsedMs, float durationMs, int rotSteps, CRGB color) {
  float wordWidth = bitmaskWordWidth(word, wordLen);
  float t = constrain(elapsedMs / durationMs, 0.0f, 1.0f);
  float startQ = 12.0f - t * (24.0f + wordWidth);
  drawBitmaskWordAt(ctx, word, wordLen, startQ, 0, rotSteps, color);
}

// Medium (127px) digit glyphs -- a single bold digit's worth of detail;
// see WorkoutTimer for why two of these can't sit side by side on this board.
const uint8_t kFont_MD_0[16] = { 0x00, 0x00, 0x7E, 0x04, 0x11, 0x88, 0x80, 0x08, 0x88, 0x80, 0x08, 0x44, 0x10, 0x3F, 0x00, 0x00 };
const uint8_t kFont_MD_1[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x80, 0x01, 0x0C, 0x30, 0x40, 0x00, 0x00, 0x00, 0x00 };
const uint8_t kFont_MD_2[16] = { 0x00, 0x38, 0x50, 0x20, 0x09, 0x49, 0x88, 0x04, 0x91, 0x10, 0x08, 0x41, 0x04, 0x11, 0x1E, 0x00 };
const uint8_t kFont_MD_3[16] = { 0x00, 0x30, 0x40, 0x00, 0x01, 0x48, 0x80, 0x84, 0x90, 0x10, 0x09, 0x4F, 0x04, 0x09, 0x0E, 0x00 };
const uint8_t kFont_MD_4[16] = { 0x00, 0x04, 0x0C, 0x28, 0x20, 0x01, 0x91, 0x10, 0x0E, 0x61, 0x88, 0x45, 0x23, 0x03, 0x00, 0x00 };
const uint8_t kFont_MD_5[16] = { 0x00, 0x30, 0x40, 0x02, 0x19, 0x48, 0x81, 0x24, 0x90, 0x04, 0x49, 0x48, 0x22, 0x79, 0x02, 0x00 };
const uint8_t kFont_MD_6[16] = { 0x00, 0x3E, 0x43, 0x0A, 0x49, 0x48, 0x84, 0x84, 0x90, 0x10, 0x09, 0x49, 0x38, 0x01, 0x06, 0x00 };
const uint8_t kFont_MD_7[16] = { 0x00, 0x00, 0x40, 0x80, 0x00, 0x02, 0x10, 0x04, 0x81, 0x10, 0x88, 0x40, 0x02, 0x05, 0x06, 0x00 };
const uint8_t kFont_MD_8[16] = { 0x00, 0x38, 0x48, 0x10, 0x79, 0x48, 0x84, 0x84, 0x90, 0x10, 0x09, 0x4F, 0x04, 0x09, 0x0E, 0x00 };
const uint8_t kFont_MD_9[16] = { 0x00, 0x30, 0x40, 0x0E, 0x49, 0x48, 0x84, 0x84, 0x90, 0x10, 0x09, 0x49, 0x28, 0x61, 0x3E, 0x00 };

const uint8_t *hexBitmaskDigitMD(int d) {
  static const uint8_t *digits[10] = { kFont_MD_0, kFont_MD_1, kFont_MD_2, kFont_MD_3, kFont_MD_4, kFont_MD_5, kFont_MD_6, kFont_MD_7, kFont_MD_8, kFont_MD_9 };
  return digits[((d % 10) + 10) % 10];
}

// Medium (127px) -- full letterforms, punctuation, and degree glyph
const uint8_t kFont_MD_SPACE[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 'SPACE'
const uint8_t kFont_MD_BANG[16] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x18, 0xC0, 0x01, 0x0E, 0x38, 0x40, 0x00, 0x00, 0x00, 0x00 }; // '!'
const uint8_t kFont_MD_AMP[16] = { 0x10, 0x1C, 0x5C, 0x16, 0x7D, 0xC8, 0x5C, 0x40, 0x9D, 0x0C, 0x0A, 0xC6, 0x31, 0x02, 0x00, 0x00 }; // '&'
const uint8_t kFont_MD_APOS[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x38, 0x40, 0x00, 0x00, 0x00, 0x00 }; // '''
const uint8_t kFont_MD_LPAREN[16] = { 0x00, 0x00, 0x00, 0x20, 0xC0, 0x07, 0xA7, 0x20, 0x00, 0x02, 0x30, 0x00, 0x00, 0x01, 0x00, 0x00 }; // '('
const uint8_t kFont_MD_RPAREN[16] = { 0x00, 0x00, 0x40, 0x00, 0x00, 0x06, 0x20, 0x00, 0x82, 0x72, 0xF0, 0x01, 0x02, 0x00, 0x00, 0x00 }; // ')'
const uint8_t kFont_MD_PLUS[16] = { 0x00, 0x00, 0x02, 0x08, 0x40, 0x06, 0x3C, 0xC0, 0x01, 0x1E, 0x30, 0x01, 0x08, 0x20, 0x00, 0x00 }; // '+'
const uint8_t kFont_MD_COMMA[16] = { 0x00, 0x00, 0x20, 0xC0, 0x01, 0x0E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // ','
const uint8_t kFont_MD_DASH[16] = { 0x00, 0x00, 0x02, 0x08, 0x40, 0x00, 0x04, 0x80, 0x00, 0x10, 0x00, 0x01, 0x08, 0x20, 0x00, 0x00 }; // '-'
const uint8_t kFont_MD_PERIOD[16] = { 0x00, 0x00, 0x60, 0xC0, 0x01, 0x0E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // '.'
const uint8_t kFont_MD_SLASH[16] = { 0x20, 0x20, 0x20, 0xC0, 0x00, 0x00, 0x18, 0xC0, 0x01, 0x0C, 0x00, 0x80, 0x01, 0x02, 0x02, 0x02 }; // '/'
const uint8_t kFont_MD_COLON[16] = { 0x00, 0x00, 0x20, 0xE0, 0x80, 0x07, 0x3B, 0x78, 0x01, 0x0F, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00 }; // ':'
const uint8_t kFont_MD_SEMI[16] = { 0x00, 0x00, 0x40, 0x20, 0x80, 0x07, 0x3B, 0x78, 0x01, 0x0F, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00 }; // ';'
const uint8_t kFont_MD_EQUALS[16] = { 0x00, 0x04, 0x0C, 0x36, 0xB0, 0x01, 0x1B, 0x60, 0x03, 0x6C, 0xC0, 0x06, 0x36, 0x18, 0x10, 0x00 }; // '='
const uint8_t kFont_MD_QMARK[16] = { 0x00, 0x00, 0x00, 0x00, 0x0D, 0xC8, 0x18, 0x80, 0x81, 0x10, 0x08, 0x41, 0x04, 0x1D, 0x1C, 0x04 }; // '?'
const uint8_t kFont_MD_A[16] = { 0x30, 0x3C, 0x1E, 0x3E, 0xB0, 0x01, 0x18, 0x18, 0x13, 0xE1, 0x0B, 0xDE, 0x3D, 0x3A, 0x10, 0x00 }; // 'A'
const uint8_t kFont_MD_B[16] = { 0x30, 0x3C, 0x5E, 0x1E, 0x7D, 0xE8, 0x84, 0x84, 0x80, 0x90, 0x09, 0x5F, 0x34, 0x1D, 0x1C, 0x04 }; // 'B'
const uint8_t kFont_MD_C[16] = { 0x10, 0x1C, 0x5E, 0x1E, 0x3D, 0xC8, 0x80, 0x00, 0x90, 0x00, 0x0A, 0x40, 0x00, 0x01, 0x02, 0x02 }; // 'C'
const uint8_t kFont_MD_D[16] = { 0x30, 0x3C, 0x5E, 0x1E, 0x3D, 0xE8, 0x80, 0x04, 0x80, 0x80, 0x09, 0x5E, 0x3C, 0x3D, 0x1C, 0x04 }; // 'D'
const uint8_t kFont_MD_E[16] = { 0x30, 0x3C, 0x5E, 0x1E, 0x7D, 0xE8, 0x84, 0x84, 0x90, 0x10, 0x0A, 0x41, 0x00, 0x01, 0x02, 0x02 }; // 'E'
const uint8_t kFont_MD_F[16] = { 0x30, 0x3C, 0x1E, 0x1E, 0x7C, 0xE0, 0x04, 0x84, 0x80, 0x10, 0x08, 0x41, 0x00, 0x01, 0x02, 0x02 }; // 'F'
const uint8_t kFont_MD_G[16] = { 0x10, 0x1C, 0x5E, 0x1E, 0x3D, 0xC8, 0x80, 0x80, 0x90, 0x90, 0x0B, 0x5F, 0x38, 0x21, 0x02, 0x02 }; // 'G'
const uint8_t kFont_MD_H[16] = { 0x30, 0x3C, 0x1E, 0x1E, 0x7C, 0xE0, 0x04, 0x84, 0x10, 0x90, 0x03, 0x1F, 0x3C, 0x3C, 0x1E, 0x06 }; // 'H'
const uint8_t kFont_MD_I[16] = { 0x20, 0x20, 0x40, 0x00, 0x01, 0x2E, 0xB8, 0xC4, 0x91, 0x0E, 0x3A, 0x40, 0x00, 0x01, 0x02, 0x02 }; // 'I'
const uint8_t kFont_MD_J[16] = { 0x10, 0x18, 0x50, 0x00, 0x01, 0x08, 0x40, 0x00, 0x0E, 0x70, 0xC8, 0xC1, 0x03, 0x03, 0x02, 0x02 }; // 'J'
const uint8_t kFont_MD_K[16] = { 0x30, 0x3C, 0x1E, 0x1E, 0x7C, 0xE0, 0x5C, 0x44, 0x1D, 0x0C, 0x02, 0x80, 0x01, 0x02, 0x02, 0x02 }; // 'K'
const uint8_t kFont_MD_L[16] = { 0x30, 0x3C, 0x5E, 0x1E, 0x3D, 0xE8, 0x80, 0x04, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 'L'
const uint8_t kFont_MD_M[16] = { 0x30, 0x3C, 0x1E, 0x1E, 0x3C, 0xE0, 0x00, 0x5C, 0x10, 0x8D, 0x03, 0x9E, 0x3D, 0x3E, 0x1E, 0x06 }; // 'M'
const uint8_t kFont_MD_N[16] = { 0x30, 0x3C, 0x1E, 0x1E, 0x3C, 0xE0, 0x00, 0x5C, 0x10, 0x9D, 0x03, 0x1F, 0x3C, 0x3C, 0x1E, 0x06 }; // 'N'
const uint8_t kFont_MD_O[16] = { 0x10, 0x1C, 0x5E, 0x1E, 0x3D, 0xC8, 0x80, 0x00, 0x80, 0x80, 0x09, 0x5E, 0x3C, 0x3D, 0x1C, 0x04 }; // 'O'
const uint8_t kFont_MD_P[16] = { 0x30, 0x3C, 0x1E, 0x1E, 0x7C, 0xE0, 0x04, 0x84, 0x80, 0x10, 0x08, 0x41, 0x04, 0x1D, 0x1C, 0x04 }; // 'P'
const uint8_t kFont_MD_Q[16] = { 0x10, 0x1C, 0x5E, 0x1E, 0x3D, 0xC8, 0x58, 0x00, 0x9D, 0x00, 0x0A, 0x46, 0x3C, 0x3D, 0x1C, 0x04 }; // 'Q'
const uint8_t kFont_MD_R[16] = { 0x30, 0x3C, 0x1E, 0x1E, 0x7C, 0xE0, 0x5C, 0x84, 0x9D, 0x10, 0x0A, 0x41, 0x04, 0x1D, 0x1C, 0x04 }; // 'R'
const uint8_t kFont_MD_S[16] = { 0x20, 0x20, 0x40, 0x06, 0x7D, 0xC8, 0x84, 0x80, 0x80, 0x90, 0x09, 0x5F, 0x30, 0x01, 0x02, 0x02 }; // 'S'
const uint8_t kFont_MD_T[16] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x2E, 0x38, 0xC4, 0x81, 0x0E, 0x38, 0x40, 0x00, 0x01, 0x02, 0x02 }; // 'T'
const uint8_t kFont_MD_U[16] = { 0x10, 0x1C, 0x5E, 0x1E, 0x3D, 0xE8, 0x80, 0x04, 0x00, 0x80, 0x01, 0x1E, 0x3C, 0x3C, 0x1E, 0x06 }; // 'U'
const uint8_t kFont_MD_V[16] = { 0x00, 0x04, 0x2E, 0xDE, 0x3D, 0xE8, 0x40, 0x04, 0x0C, 0x00, 0x00, 0x06, 0x3C, 0x3C, 0x1E, 0x06 }; // 'V'
const uint8_t kFont_MD_W[16] = { 0x10, 0x1C, 0x5E, 0x1E, 0x3C, 0xE6, 0xB8, 0x84, 0x01, 0x80, 0x01, 0x1E, 0x3C, 0x3C, 0x1E, 0x06 }; // 'W'
const uint8_t kFont_MD_X[16] = { 0x30, 0x38, 0x10, 0x20, 0x8C, 0xE1, 0x03, 0xA4, 0x12, 0xE0, 0xC3, 0x18, 0x02, 0x04, 0x0E, 0x06 }; // 'X'
const uint8_t kFont_MD_Y[16] = { 0x00, 0x00, 0x00, 0x00, 0x0D, 0xEE, 0x3B, 0xA4, 0x01, 0x00, 0xC0, 0x00, 0x02, 0x04, 0x0E, 0x06 }; // 'Y'
const uint8_t kFont_MD_Z[16] = { 0x30, 0x38, 0x50, 0x20, 0x81, 0x29, 0x80, 0x84, 0x90, 0x00, 0xCA, 0x40, 0x02, 0x05, 0x0E, 0x06 }; // 'Z'
const uint8_t kFont_MD_a[16] = { 0x10, 0x1C, 0x5C, 0x10, 0x41, 0x08, 0x44, 0x98, 0x1C, 0x93, 0x33, 0x9F, 0x3D, 0x3A, 0x10, 0x00 }; // 'a'
const uint8_t kFont_MD_b[16] = { 0x30, 0x3C, 0x5E, 0x1E, 0x3D, 0xE8, 0x83, 0x64, 0x00, 0x8C, 0xC1, 0x1E, 0x3A, 0x20, 0x00, 0x00 }; // 'b'
const uint8_t kFont_MD_c[16] = { 0x10, 0x1C, 0x5E, 0x18, 0x01, 0x08, 0x83, 0x60, 0x10, 0x0C, 0xC2, 0x00, 0x06, 0x18, 0x10, 0x00 }; // 'c'
const uint8_t kFont_MD_d[16] = { 0x10, 0x1C, 0x5E, 0x18, 0x01, 0x08, 0x83, 0x60, 0x10, 0x8C, 0xC3, 0x1E, 0x3E, 0x3C, 0x1E, 0x06 }; // 'd'
const uint8_t kFont_MD_e[16] = { 0x10, 0x1C, 0x5E, 0x1E, 0x71, 0x08, 0x84, 0x98, 0x10, 0x13, 0x32, 0x81, 0x0D, 0x3A, 0x10, 0x00 }; // 'e'
const uint8_t kFont_MD_f[16] = { 0x00, 0x00, 0x60, 0xE6, 0xF0, 0x01, 0x07, 0x78, 0x00, 0x0D, 0xC8, 0x40, 0x02, 0x01, 0x00, 0x00 }; // 'f'
const uint8_t kFont_MD_g[16] = { 0x00, 0x00, 0x42, 0x2E, 0xB1, 0x09, 0x98, 0x18, 0x03, 0xE3, 0x31, 0x9E, 0x3D, 0x3E, 0x1C, 0x04 }; // 'g'
const uint8_t kFont_MD_h[16] = { 0x30, 0x3C, 0x1E, 0x1E, 0x3C, 0xE0, 0x03, 0x64, 0x10, 0x8C, 0xC3, 0x1E, 0x3A, 0x20, 0x00, 0x00 }; // 'h'
const uint8_t kFont_MD_i[16] = { 0x00, 0x00, 0x40, 0x00, 0x01, 0x0E, 0xBB, 0xE0, 0x01, 0x0C, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00 }; // 'i'
const uint8_t kFont_MD_j[16] = { 0x10, 0x18, 0x50, 0x00, 0x01, 0x08, 0x40, 0x40, 0x0E, 0x7C, 0xC0, 0x01, 0x02, 0x01, 0x00, 0x00 }; // 'j'
const uint8_t kFont_MD_k[16] = { 0x30, 0x3C, 0x1E, 0x3E, 0xBC, 0xE7, 0xA0, 0x84, 0x00, 0x00, 0xC0, 0x00, 0x02, 0x00, 0x00, 0x00 }; // 'k'
const uint8_t kFont_MD_l[16] = { 0x00, 0x00, 0x40, 0x00, 0x01, 0x0E, 0xB8, 0xC0, 0x81, 0x0E, 0x38, 0x40, 0x00, 0x00, 0x00, 0x00 }; // 'l'
const uint8_t kFont_MD_m[16] = { 0x30, 0x3C, 0x1E, 0x1E, 0x3C, 0xC0, 0x18, 0xD8, 0x11, 0x8D, 0x03, 0x9E, 0x3D, 0x3A, 0x10, 0x00 }; // 'm'
const uint8_t kFont_MD_n[16] = { 0x30, 0x3C, 0x1E, 0x1E, 0x3C, 0xC0, 0x03, 0x20, 0x10, 0x82, 0x33, 0x9E, 0x3D, 0x3A, 0x10, 0x00 }; // 'n'
const uint8_t kFont_MD_o[16] = { 0x10, 0x1C, 0x5E, 0x1E, 0x31, 0x08, 0x80, 0x18, 0x00, 0x83, 0x31, 0x9E, 0x3D, 0x3A, 0x10, 0x00 }; // 'o'
const uint8_t kFont_MD_p[16] = { 0x30, 0x3C, 0x1E, 0x3E, 0xBC, 0xC1, 0x18, 0x18, 0x03, 0x63, 0x30, 0x80, 0x0D, 0x3A, 0x10, 0x00 }; // 'p'
const uint8_t kFont_MD_q[16] = { 0x00, 0x00, 0x02, 0x2E, 0xB0, 0x01, 0x18, 0x18, 0x13, 0xE3, 0x33, 0x9E, 0x3D, 0x3E, 0x1C, 0x04 }; // 'q'
const uint8_t kFont_MD_r[16] = { 0x30, 0x3C, 0x1E, 0x1E, 0x3C, 0xC0, 0x03, 0x20, 0x00, 0x02, 0x30, 0x80, 0x05, 0x1A, 0x10, 0x00 }; // 'r'
const uint8_t kFont_MD_s[16] = { 0x20, 0x20, 0x40, 0x06, 0x71, 0x08, 0x84, 0x98, 0x00, 0x93, 0x31, 0x9F, 0x31, 0x06, 0x0C, 0x04 }; // 's'
const uint8_t kFont_MD_t[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xB8, 0xD8, 0x11, 0x0F, 0x3A, 0xC0, 0x01, 0x02, 0x00, 0x00 }; // 't'
const uint8_t kFont_MD_u[16] = { 0x10, 0x1C, 0x5E, 0x1E, 0x3D, 0xC8, 0x40, 0x00, 0x1C, 0x80, 0x03, 0x1E, 0x3C, 0x3C, 0x1C, 0x04 }; // 'u'
const uint8_t kFont_MD_v[16] = { 0x00, 0x04, 0x2E, 0xDE, 0x3D, 0xC8, 0x40, 0x00, 0x0C, 0x00, 0x00, 0x06, 0x3C, 0x3C, 0x1C, 0x04 }; // 'v'
const uint8_t kFont_MD_w[16] = { 0x10, 0x1C, 0x5E, 0x1E, 0x3C, 0xC6, 0xB8, 0x80, 0x01, 0x80, 0x01, 0x1E, 0x3C, 0x3C, 0x1C, 0x04 }; // 'w'
const uint8_t kFont_MD_x[16] = { 0x30, 0x38, 0x10, 0x20, 0x8C, 0xC1, 0x03, 0xA0, 0x12, 0xE0, 0xC3, 0x18, 0x02, 0x04, 0x0C, 0x04 }; // 'x'
const uint8_t kFont_MD_y[16] = { 0x00, 0x00, 0x42, 0x2E, 0xBD, 0xC9, 0x98, 0x00, 0x03, 0xE0, 0x01, 0x1E, 0x3C, 0x3C, 0x1C, 0x04 }; // 'y'
const uint8_t kFont_MD_z[16] = { 0x30, 0x38, 0x50, 0x20, 0x8D, 0xC9, 0x80, 0x98, 0x10, 0x03, 0xF2, 0x80, 0x03, 0x06, 0x0C, 0x04 }; // 'z'
const uint8_t kFont_MD_DEGREE[16] = { 0x00, 0x00, 0x00, 0x00, 0x0C, 0xC0, 0x03, 0x60, 0x80, 0x0C, 0x08, 0xC0, 0x01, 0x02, 0x00, 0x00 }; // '°'

// Large (271px) digit glyphs -- a single full-screen digit, one at a time
const uint8_t kFont_LG_0[34] = { 0x00, 0x00, 0x00, 0x7F, 0xF8, 0x0F, 0xFF, 0xC3, 0xC1, 0xA1, 0xC0, 0xA1, 0x80, 0x43, 0x01, 0x0A, 0x05, 0x50, 0x28, 0x40, 0xA1, 0x80, 0xC2, 0x81, 0xC3, 0xC1, 0xE1, 0x7F, 0xF8, 0x0F, 0x7F, 0x00, 0x00, 0x00 };
const uint8_t kFont_LG_1[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0xE0, 0x01, 0xF0, 0x01, 0xF0, 0x01, 0xE0, 0x03, 0xC0, 0x06, 0xC0, 0x06, 0x40, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t kFont_LG_2[34] = { 0x00, 0x00, 0x00, 0x78, 0x80, 0x0E, 0xE0, 0x03, 0xE8, 0x21, 0xD8, 0x61, 0xA8, 0x43, 0x61, 0x0E, 0x85, 0x72, 0x28, 0x8C, 0xA3, 0x28, 0xC6, 0x31, 0xC0, 0x39, 0xE0, 0x0F, 0xF8, 0x03, 0x3F, 0x00, 0x00, 0x00 };
const uint8_t kFont_LG_3[34] = { 0x00, 0x00, 0x00, 0x78, 0x00, 0x0F, 0xC0, 0x03, 0xC0, 0x21, 0xC0, 0x61, 0x8C, 0x43, 0x39, 0x0E, 0xC5, 0x71, 0x28, 0x4E, 0xA1, 0xB8, 0xC2, 0xF1, 0xC2, 0x71, 0xE1, 0x7F, 0xF8, 0x03, 0x3F, 0x00, 0x00, 0x00 };
const uint8_t kFont_LG_4[34] = { 0x00, 0x00, 0x00, 0x0F, 0xD0, 0x01, 0x7E, 0x00, 0x3F, 0x80, 0x3A, 0x80, 0x73, 0x03, 0xCE, 0x0F, 0x28, 0x3E, 0xE0, 0x7C, 0x80, 0xFF, 0x81, 0xBE, 0xC3, 0x8F, 0xE1, 0x41, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t kFont_LG_5[34] = { 0x00, 0x00, 0x00, 0x7C, 0x80, 0x0F, 0xE0, 0xC3, 0xC3, 0xA1, 0xC3, 0xA1, 0x87, 0x43, 0x1D, 0x0E, 0xE5, 0x70, 0x38, 0x47, 0xE1, 0x9C, 0xC2, 0xF9, 0xC2, 0xF9, 0xE1, 0x7C, 0x38, 0x00, 0x03, 0x00, 0x00, 0x00 };
const uint8_t kFont_LG_6[34] = { 0x00, 0x00, 0x00, 0x7F, 0xF8, 0x0F, 0xFF, 0xC3, 0xC7, 0xA1, 0xC7, 0xA1, 0x8E, 0x43, 0x39, 0x0E, 0xC5, 0x71, 0x28, 0x4E, 0xA1, 0xB8, 0xC2, 0xF1, 0xC2, 0xF1, 0xE1, 0x78, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t kFont_LG_7[34] = { 0x00, 0x00, 0x00, 0x80, 0x00, 0x18, 0x00, 0x07, 0xC0, 0x01, 0xE0, 0x20, 0xA0, 0xC0, 0x40, 0x01, 0x07, 0x05, 0x38, 0x14, 0xE0, 0x38, 0xC0, 0x39, 0xC0, 0x1D, 0xE0, 0x07, 0xF8, 0x00, 0x0F, 0x00, 0x00, 0x00 };
const uint8_t kFont_LG_8[34] = { 0x00, 0x00, 0x00, 0x7C, 0x40, 0x0F, 0xE8, 0xC3, 0xCB, 0xA1, 0xC7, 0xA1, 0x87, 0x43, 0x19, 0x0E, 0xC5, 0x70, 0x28, 0x46, 0xA1, 0xF8, 0xC2, 0xF1, 0xC2, 0xF9, 0xE1, 0x0F, 0xF8, 0x01, 0x1F, 0x00, 0x00, 0x00 };
const uint8_t kFont_LG_9[34] = { 0x00, 0x00, 0x00, 0x70, 0x38, 0x0E, 0x8F, 0xC3, 0xC7, 0xA1, 0xC7, 0xA1, 0x8E, 0x43, 0x39, 0x0E, 0xC5, 0x71, 0x28, 0x4E, 0xA1, 0xB8, 0xC2, 0xF1, 0xC2, 0xF1, 0xE1, 0x7F, 0xF8, 0x0F, 0x7F, 0x00, 0x00, 0x00 };

const uint8_t *hexBitmaskDigitLG(int d) {
  static const uint8_t *digits[10] = { kFont_LG_0, kFont_LG_1, kFont_LG_2, kFont_LG_3, kFont_LG_4, kFont_LG_5, kFont_LG_6, kFont_LG_7, kFont_LG_8, kFont_LG_9 };
  return digits[((d % 10) + 10) % 10];
}

// Large (271px) -- full letterforms, punctuation, and degree glyph
const uint8_t kFont_LG_SPACE[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 'SPACE'
const uint8_t kFont_LG_BANG[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xC0, 0x01, 0xD0, 0x01, 0xB8, 0x00, 0xF8, 0x00, 0xF0, 0x01, 0x60, 0x03, 0x60, 0x03, 0xE0, 0x01, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // '!'
const uint8_t kFont_LG_AMP[34] = { 0x60, 0xC0, 0x1B, 0xDE, 0xC1, 0xB8, 0x01, 0xFE, 0x06, 0x7F, 0x27, 0x33, 0x7E, 0x00, 0xC8, 0x87, 0x21, 0x32, 0xCF, 0x81, 0x3F, 0x07, 0x78, 0x02, 0x73, 0x81, 0xDB, 0xC0, 0x30, 0x00, 0x02, 0x00, 0x00, 0x00 }; // '&'
const uint8_t kFont_LG_APOS[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0xE0, 0x01, 0xF0, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // '''
const uint8_t kFont_LG_LPAREN[34] = { 0x00, 0x00, 0x80, 0x7F, 0xF8, 0x0F, 0xFF, 0xC3, 0xC1, 0xE1, 0xC0, 0xA0, 0x00, 0x40, 0x01, 0x00, 0x05, 0x00, 0x28, 0x00, 0xE0, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // '('
const uint8_t kFont_LG_RPAREN[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x80, 0x03, 0x00, 0x0E, 0x00, 0x70, 0x00, 0xC0, 0x01, 0x80, 0x82, 0x81, 0xC2, 0x41, 0xE1, 0x7F, 0xF8, 0x0F, 0xFF, 0x00, 0x00, 0x00 }; // ')'
const uint8_t kFont_LG_PLUS[34] = { 0x00, 0x00, 0x80, 0x01, 0x28, 0x00, 0x0A, 0x00, 0x87, 0x01, 0xE7, 0x01, 0xFE, 0x01, 0xF8, 0x01, 0xE0, 0x03, 0xC0, 0x0F, 0xC0, 0x3E, 0x40, 0x73, 0xC0, 0x70, 0x00, 0x38, 0x00, 0x0E, 0xC0, 0x00, 0x00, 0x00 }; // '+'
const uint8_t kFont_LG_COMMA[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x60, 0x01, 0xE0, 0x01, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // ','
const uint8_t kFont_LG_DASH[34] = { 0x00, 0x00, 0x00, 0x01, 0x38, 0x00, 0x0E, 0x00, 0x07, 0x00, 0x07, 0x00, 0x0E, 0x00, 0x38, 0x00, 0xC0, 0x01, 0x00, 0x0E, 0x00, 0x38, 0x00, 0x70, 0x00, 0x70, 0x00, 0x38, 0x00, 0x0E, 0x40, 0x00, 0x00, 0x00 }; // '-'
const uint8_t kFont_LG_PERIOD[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x01, 0xE0, 0x01, 0xE0, 0x01, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // '.'
const uint8_t kFont_LG_SLASH[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0xC0, 0x01, 0x70, 0x00, 0x38, 0x00, 0x38, 0x00, 0x50, 0x00, 0x40, 0x01, 0x00, 0x05, 0x00, 0x0A, 0x00, 0x0A, 0x00, 0x05, 0x40, 0x01, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00 }; // '/'
const uint8_t kFont_LG_COLON[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x01, 0xE0, 0x01, 0xC0, 0x01, 0x00, 0x00, 0x38, 0x00, 0xE0, 0x01, 0xC0, 0x03, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // ':'
const uint8_t kFont_LG_SEMI[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x20, 0x01, 0xB0, 0x00, 0xF0, 0x00, 0xC0, 0x01, 0x38, 0x00, 0xE0, 0x01, 0xC0, 0x03, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // ';'
const uint8_t kFont_LG_EQUALS[34] = { 0x00, 0xC0, 0x00, 0x0A, 0x40, 0x81, 0x71, 0xA0, 0x38, 0xA0, 0x38, 0xC0, 0x71, 0x00, 0xC7, 0x01, 0x38, 0x0E, 0xC0, 0x71, 0x00, 0xC7, 0x01, 0x8E, 0x03, 0x8E, 0x03, 0xC7, 0xC0, 0x01, 0x38, 0x80, 0x01, 0x00 }; // '='
const uint8_t kFont_LG_QMARK[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x61, 0xC0, 0xA1, 0xB0, 0x41, 0xE1, 0x00, 0x85, 0x03, 0x28, 0x0C, 0xE0, 0x30, 0xC0, 0x51, 0xC0, 0x29, 0xE0, 0x0B, 0xF8, 0x01, 0x1F, 0x00, 0x00, 0x00 }; // '?'
const uint8_t kFont_LG_A[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x81, 0x7F, 0xE0, 0x39, 0x60, 0x38, 0x00, 0x70, 0xC0, 0xC0, 0x01, 0x06, 0x0E, 0x31, 0x70, 0x06, 0xC0, 0x6F, 0x80, 0x7F, 0xC1, 0xDF, 0xF8, 0xB0, 0x0F, 0x7A, 0x80, 0x01, 0x00 }; // 'A'
const uint8_t kFont_LG_B[34] = { 0xE0, 0xC3, 0x1F, 0xDF, 0xF9, 0xB8, 0x0F, 0xFE, 0x07, 0x7F, 0x07, 0x3F, 0x0E, 0x3E, 0x38, 0xF8, 0xC1, 0xC1, 0x0E, 0x0E, 0x38, 0x38, 0x76, 0x70, 0x7F, 0xB0, 0x3F, 0xC0, 0x8E, 0xC1, 0x3D, 0xEC, 0x01, 0x03 }; // 'B'
const uint8_t kFont_LG_C[34] = { 0x60, 0xC0, 0x1B, 0xDF, 0xF9, 0xB8, 0x0F, 0xFE, 0x01, 0x7F, 0x00, 0x37, 0x00, 0x0E, 0x00, 0xB8, 0x01, 0xC0, 0x0F, 0x00, 0x3E, 0x00, 0x78, 0x00, 0x70, 0x00, 0x38, 0x00, 0x0E, 0xC0, 0x01, 0x1C, 0xE0, 0x00 }; // 'C'
const uint8_t kFont_LG_D[34] = { 0xE0, 0xC3, 0x1F, 0xDF, 0xF9, 0xB8, 0x0F, 0xFE, 0x01, 0x7F, 0x00, 0x3F, 0x00, 0x3E, 0x00, 0xF8, 0x01, 0xC0, 0x0E, 0x00, 0x38, 0x00, 0x76, 0x00, 0x7F, 0xC0, 0x3F, 0xF8, 0x8E, 0xCF, 0x7D, 0xEC, 0x01, 0x03 }; // 'D'
const uint8_t kFont_LG_E[34] = { 0xE0, 0xC3, 0x1F, 0xDF, 0xF9, 0xB8, 0x0F, 0xFE, 0x07, 0x7F, 0x07, 0x3F, 0x0E, 0x3E, 0x38, 0xF8, 0xC1, 0xC1, 0x0F, 0x0E, 0x3E, 0x38, 0x78, 0x70, 0x70, 0x30, 0x38, 0x00, 0x0E, 0xC0, 0x01, 0x1C, 0xE0, 0x00 }; // 'E'
const uint8_t kFont_LG_F[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x80, 0x0F, 0xF0, 0x07, 0x78, 0x07, 0x38, 0x0E, 0x30, 0x38, 0xC0, 0xC1, 0x01, 0x0E, 0x0E, 0x38, 0x38, 0x70, 0x70, 0x70, 0x30, 0x38, 0x00, 0x0E, 0xC0, 0x01, 0x1C, 0xE0, 0x00 }; // 'F'
const uint8_t kFont_LG_G[34] = { 0x60, 0xC0, 0x1B, 0xDF, 0xF9, 0xB8, 0x0F, 0xFE, 0x01, 0x7F, 0x00, 0x37, 0x00, 0x0E, 0x30, 0xB8, 0xC1, 0xC1, 0x0F, 0x0E, 0x3E, 0x38, 0x7E, 0x70, 0x7F, 0xF0, 0x3F, 0xF8, 0x0E, 0xCE, 0x41, 0x1C, 0xE0, 0x00 }; // 'G'
const uint8_t kFont_LG_H[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x80, 0x0F, 0xF0, 0x07, 0x78, 0x07, 0x38, 0x0E, 0x30, 0x38, 0x40, 0xC0, 0x01, 0x01, 0x0E, 0x06, 0x38, 0x0E, 0x70, 0x0F, 0xF0, 0x07, 0xF8, 0x80, 0x0F, 0x7C, 0xF0, 0xE1, 0x03 }; // 'H'
const uint8_t kFont_LG_I[34] = { 0x80, 0x03, 0x1C, 0xC0, 0x01, 0x38, 0x00, 0x0E, 0x80, 0x07, 0xE0, 0x0F, 0xF0, 0x3F, 0xF0, 0xF9, 0xE1, 0xC3, 0xCF, 0x07, 0xFE, 0x07, 0xF8, 0x03, 0xF0, 0x00, 0x38, 0x00, 0x0E, 0xC0, 0x01, 0x1C, 0xE0, 0x00 }; // 'I'
const uint8_t kFont_LG_J[34] = { 0x60, 0x00, 0x1B, 0xD0, 0x01, 0x38, 0x00, 0x0E, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x06, 0x00, 0x3C, 0x00, 0xF8, 0x01, 0xF8, 0x61, 0xFC, 0x70, 0x3F, 0xF8, 0x07, 0x7E, 0xC0, 0x03, 0x1C, 0xE0, 0x00 }; // 'J'
const uint8_t kFont_LG_K[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x80, 0x0F, 0xF0, 0x07, 0x78, 0x27, 0x38, 0x7E, 0x30, 0xC8, 0x47, 0x20, 0x32, 0xC1, 0x81, 0x07, 0x07, 0x08, 0x02, 0x00, 0x01, 0xC0, 0x00, 0x30, 0x00, 0x02, 0x10, 0xE0, 0x00 }; // 'K'
const uint8_t kFont_LG_L[34] = { 0xE0, 0xC3, 0x1F, 0xDF, 0xF9, 0xB8, 0x0F, 0xFE, 0x01, 0x7F, 0x00, 0x3F, 0x00, 0x3E, 0x00, 0x78, 0x00, 0xC0, 0x01, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 'L'
const uint8_t kFont_LG_M[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x80, 0x0F, 0xF0, 0x01, 0x78, 0x00, 0x38, 0x00, 0xF0, 0x00, 0x40, 0x26, 0x00, 0xF1, 0x01, 0x06, 0x07, 0x0E, 0x02, 0x0F, 0xC1, 0xC7, 0xF8, 0xB0, 0x0F, 0x7E, 0xF0, 0xE1, 0x03 }; // 'M'
const uint8_t kFont_LG_N[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x80, 0x0F, 0xF0, 0x01, 0x78, 0x00, 0x38, 0x00, 0xF0, 0x00, 0x40, 0x26, 0x00, 0xF1, 0x09, 0x06, 0x3F, 0x0E, 0x72, 0x0F, 0xF0, 0x07, 0xF8, 0x80, 0x0F, 0x7C, 0xF0, 0xE1, 0x03 }; // 'N'
const uint8_t kFont_LG_O[34] = { 0x60, 0xC0, 0x1B, 0xDF, 0xF9, 0xB8, 0x0F, 0xFE, 0x01, 0x7F, 0x00, 0x37, 0x00, 0x0E, 0x00, 0xB8, 0x01, 0xC0, 0x0E, 0x00, 0x38, 0x00, 0x76, 0x00, 0x7F, 0xC0, 0x3F, 0xF8, 0x8E, 0xCF, 0x7D, 0xEC, 0x01, 0x03 }; // 'O'
const uint8_t kFont_LG_P[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x80, 0x0F, 0xF0, 0x07, 0x78, 0x07, 0x38, 0x0E, 0x30, 0x38, 0xC0, 0xC1, 0x01, 0x0E, 0x0E, 0x38, 0x38, 0x70, 0x70, 0x70, 0x30, 0x38, 0x00, 0x8E, 0xC1, 0x3D, 0xEC, 0x01, 0x03 }; // 'P'
const uint8_t kFont_LG_Q[34] = { 0x60, 0xC0, 0x1B, 0xDF, 0xF9, 0xB8, 0x0F, 0xFE, 0x01, 0x7F, 0x20, 0x33, 0x70, 0x00, 0xC0, 0x87, 0x01, 0x32, 0x0F, 0x80, 0x3F, 0x00, 0x78, 0x00, 0x73, 0xC0, 0x3B, 0xF8, 0x8E, 0xCF, 0x7D, 0xEC, 0x01, 0x03 }; // 'Q'
const uint8_t kFont_LG_R[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x80, 0x0F, 0xF0, 0x07, 0x78, 0x27, 0x38, 0x7E, 0x30, 0xF8, 0xC7, 0xC1, 0x33, 0x0F, 0x8E, 0x3F, 0x38, 0x78, 0x70, 0x70, 0x30, 0x38, 0x00, 0x8E, 0xC1, 0x3D, 0xEC, 0x01, 0x03 }; // 'R'
const uint8_t kFont_LG_S[34] = { 0x80, 0x03, 0x1C, 0xC0, 0x01, 0xB8, 0x01, 0xFE, 0x06, 0x7F, 0x07, 0x37, 0x0E, 0x0E, 0x38, 0xB8, 0xC1, 0xC1, 0x0E, 0x0E, 0x38, 0x38, 0x76, 0x70, 0x7F, 0xB0, 0x3F, 0xC0, 0x0E, 0xC0, 0x01, 0x1C, 0xE0, 0x00 }; // 'S'
const uint8_t kFont_LG_T[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x80, 0x07, 0xE0, 0x0B, 0xF0, 0x31, 0xF0, 0xC1, 0xE1, 0x03, 0xCE, 0x07, 0xF8, 0x07, 0xF0, 0x03, 0xF0, 0x00, 0x38, 0x00, 0x0E, 0xC0, 0x01, 0x1C, 0xE0, 0x00 }; // 'T'
const uint8_t kFont_LG_U[34] = { 0x60, 0xC0, 0x1B, 0xDF, 0xF9, 0xB8, 0x0F, 0xFE, 0x01, 0x7F, 0x00, 0x3F, 0x00, 0x3E, 0x00, 0x78, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0F, 0xC0, 0x07, 0xF8, 0x80, 0x0F, 0x7C, 0xF0, 0xE1, 0x03 }; // 'U'
const uint8_t kFont_LG_V[34] = { 0x00, 0xC0, 0x00, 0x2F, 0xF8, 0x86, 0x8F, 0xFD, 0x41, 0x7F, 0x00, 0x3B, 0x00, 0x30, 0x00, 0x46, 0x00, 0x30, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x03, 0xF8, 0x80, 0x0F, 0x7C, 0xF0, 0xE1, 0x03 }; // 'V'
const uint8_t kFont_LG_W[34] = { 0x60, 0xC0, 0x1B, 0xDF, 0xF9, 0xB8, 0x0F, 0xF2, 0x81, 0x78, 0xE0, 0x3C, 0xF0, 0x3F, 0xF0, 0x79, 0xC0, 0xC3, 0x00, 0x06, 0x00, 0x00, 0x06, 0x00, 0x0F, 0xC0, 0x07, 0xF8, 0x80, 0x0F, 0x7C, 0xF0, 0xE1, 0x03 }; // 'W'
const uint8_t kFont_LG_X[34] = { 0xE0, 0x03, 0x07, 0x10, 0x00, 0x01, 0x70, 0x10, 0x38, 0x98, 0x18, 0xF8, 0x01, 0x30, 0x37, 0x40, 0xD8, 0x0D, 0x01, 0x76, 0x06, 0xC0, 0x0F, 0x8C, 0x0C, 0x0E, 0x04, 0x07, 0x40, 0x00, 0x04, 0x70, 0xE0, 0x03 }; // 'X'
const uint8_t kFont_LG_Y[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x80, 0x9F, 0xE0, 0xFB, 0xF1, 0x31, 0xF7, 0x41, 0xD8, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x0E, 0x00, 0x07, 0x40, 0x00, 0x04, 0x70, 0xE0, 0x03 }; // 'Y'
const uint8_t kFont_LG_Z[34] = { 0x00, 0x00, 0x00, 0x70, 0x00, 0x0F, 0xE0, 0x03, 0xF8, 0x01, 0xDC, 0x21, 0x98, 0xC3, 0x30, 0x0E, 0xC7, 0x71, 0x38, 0x87, 0xE1, 0x0C, 0xC2, 0x0D, 0xC0, 0x0F, 0xE0, 0x03, 0x78, 0x00, 0x07, 0x00, 0x00, 0x00 }; // 'Z'
const uint8_t kFont_LG_a[34] = { 0x60, 0xC0, 0x1B, 0xDE, 0xC1, 0x38, 0x00, 0x0E, 0x06, 0x07, 0x07, 0x03, 0x0E, 0xC0, 0x38, 0x06, 0xC6, 0x31, 0x31, 0x8E, 0xC7, 0x38, 0x8E, 0x71, 0x8F, 0xF1, 0xC7, 0xF8, 0xB0, 0x0F, 0x7A, 0x80, 0x01, 0x00 }; // 'a'
const uint8_t kFont_LG_b[34] = { 0xE0, 0xC3, 0x1F, 0xDF, 0xF9, 0xB8, 0x0F, 0xFE, 0x01, 0xFF, 0x00, 0xFF, 0x01, 0x3E, 0x07, 0x78, 0x38, 0xC0, 0xC0, 0x01, 0x00, 0x07, 0x06, 0x0E, 0x0F, 0xCE, 0x07, 0xFF, 0x40, 0x0E, 0x40, 0x00, 0x00, 0x00 }; // 'b'
const uint8_t kFont_LG_c[34] = { 0x60, 0xC0, 0x1B, 0xDF, 0xF9, 0x38, 0x0E, 0x0E, 0x01, 0x87, 0x00, 0xC7, 0x01, 0x0E, 0x07, 0x38, 0x38, 0xC0, 0xC1, 0x01, 0x06, 0x07, 0x08, 0x0E, 0x00, 0x0E, 0x00, 0x07, 0xC0, 0x01, 0x38, 0x80, 0x01, 0x00 }; // 'c'
const uint8_t kFont_LG_d[34] = { 0x60, 0xC0, 0x1B, 0xDF, 0xF9, 0x38, 0x0E, 0x0E, 0x01, 0x87, 0x00, 0xC7, 0x01, 0x0E, 0x07, 0x38, 0x38, 0xC0, 0xC1, 0x01, 0x06, 0x07, 0x0E, 0x0E, 0x0F, 0xCE, 0x07, 0xFF, 0xC0, 0x0F, 0x7C, 0xF0, 0xE1, 0x03 }; // 'd'
const uint8_t kFont_LG_e[34] = { 0x60, 0xC0, 0x1B, 0xDF, 0xF9, 0xB8, 0x0F, 0xEE, 0x07, 0x67, 0x07, 0x07, 0x0E, 0xCE, 0x38, 0x38, 0xC6, 0xC1, 0x31, 0x0E, 0xC6, 0x38, 0x88, 0x71, 0x80, 0x71, 0xC0, 0x38, 0xB0, 0x0F, 0x7A, 0x80, 0x01, 0x00 }; // 'e'
const uint8_t kFont_LG_f[34] = { 0x00, 0x00, 0x18, 0xE0, 0x01, 0xBF, 0xF1, 0xE3, 0x7E, 0xE0, 0x1F, 0xC0, 0x0F, 0xC0, 0x0F, 0x00, 0x3E, 0x00, 0xF0, 0x01, 0x00, 0x07, 0x60, 0x0E, 0x70, 0x0E, 0x38, 0x07, 0x4E, 0xC0, 0x01, 0x0C, 0x00, 0x00 }; // 'f'
const uint8_t kFont_LG_g[34] = { 0x00, 0x00, 0x18, 0xC1, 0x39, 0xB9, 0x7F, 0xEE, 0x39, 0x67, 0x38, 0x07, 0x70, 0xCE, 0xC0, 0x39, 0x06, 0xCE, 0x30, 0x70, 0xC0, 0xC0, 0x87, 0x81, 0x8F, 0xC1, 0xC7, 0xF8, 0xB0, 0x0F, 0x7E, 0xE0, 0x01, 0x03 }; // 'g'
const uint8_t kFont_LG_h[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x80, 0x0F, 0xF0, 0x01, 0xF8, 0x00, 0xF8, 0x01, 0x30, 0x07, 0x40, 0x38, 0x00, 0xC1, 0x01, 0x06, 0x07, 0x0E, 0x0E, 0x0F, 0xCE, 0x07, 0xFF, 0x40, 0x0E, 0x40, 0x00, 0x00, 0x00 }; // 'h'
const uint8_t kFont_LG_i[34] = { 0x00, 0x00, 0x18, 0xC0, 0x01, 0x38, 0x00, 0x0E, 0x80, 0x87, 0xE0, 0xC7, 0xF1, 0x0F, 0xF7, 0x39, 0xF8, 0xC3, 0xC0, 0x07, 0x00, 0x07, 0x60, 0x02, 0x70, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 'i'
const uint8_t kFont_LG_j[34] = { 0x60, 0x00, 0x1B, 0xD0, 0x01, 0x38, 0x00, 0x0E, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x06, 0x20, 0x3C, 0xC0, 0xF9, 0x01, 0xFF, 0x01, 0xFE, 0x00, 0x3E, 0x20, 0x07, 0x4E, 0xC0, 0x01, 0x0C, 0x00, 0x00 }; // 'j'
const uint8_t kFont_LG_k[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x81, 0x7F, 0xF0, 0xB9, 0x78, 0xD8, 0x3C, 0x80, 0x3F, 0x30, 0x78, 0xC0, 0xC1, 0x00, 0x06, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x0E, 0x00, 0x07, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 'k'
const uint8_t kFont_LG_l[34] = { 0x00, 0x00, 0x18, 0xC0, 0x01, 0x38, 0x00, 0x0E, 0x80, 0x07, 0xE0, 0x07, 0xF0, 0x0F, 0xF0, 0xB9, 0xE1, 0xC3, 0xCE, 0x07, 0xF8, 0x07, 0xF0, 0x03, 0xF0, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // 'l'
const uint8_t kFont_LG_m[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x80, 0x0F, 0xF0, 0x01, 0x78, 0x20, 0x30, 0x70, 0xC0, 0xF0, 0x01, 0xE6, 0x03, 0xF1, 0x07, 0x06, 0x07, 0x0E, 0x02, 0x0F, 0xC1, 0xC7, 0xF8, 0xB0, 0x0F, 0x7A, 0x80, 0x01, 0x00 }; // 'm'
const uint8_t kFont_LG_n[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x80, 0x0F, 0xF0, 0x01, 0xF8, 0x00, 0xF0, 0x01, 0x00, 0x07, 0x00, 0x18, 0x00, 0x01, 0x00, 0xC6, 0x00, 0x8E, 0x01, 0x8F, 0xC1, 0xC7, 0xF8, 0xB0, 0x0F, 0x7A, 0x80, 0x01, 0x00 }; // 'n'
const uint8_t kFont_LG_o[34] = { 0x60, 0xC0, 0x1B, 0xDF, 0xF9, 0xB8, 0x0F, 0xEE, 0x01, 0x67, 0x00, 0x07, 0x00, 0xCE, 0x00, 0x38, 0x06, 0xC0, 0x30, 0x00, 0xC0, 0x00, 0x86, 0x01, 0x8F, 0xC1, 0xC7, 0xF8, 0xB0, 0x0F, 0x7A, 0x80, 0x01, 0x00 }; // 'o'
const uint8_t kFont_LG_p[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x81, 0x7F, 0xF0, 0x39, 0x78, 0x38, 0x30, 0x70, 0xC0, 0xC0, 0x01, 0x06, 0x0E, 0x30, 0x70, 0xC0, 0xC0, 0x81, 0x81, 0x80, 0x41, 0xC0, 0x38, 0xB0, 0x0F, 0x7A, 0x80, 0x01, 0x00 }; // 'p'
const uint8_t kFont_LG_q[34] = { 0x00, 0x00, 0x00, 0x01, 0x38, 0x81, 0x7F, 0xE0, 0x39, 0x60, 0x38, 0x00, 0x70, 0xC0, 0xC0, 0x01, 0x06, 0x0E, 0x31, 0x70, 0xC6, 0xC0, 0x8F, 0x81, 0x8F, 0xC1, 0xC7, 0xF8, 0xB0, 0x0F, 0x7E, 0xE0, 0x01, 0x03 }; // 'q'
const uint8_t kFont_LG_r[34] = { 0xE0, 0xC3, 0x07, 0x1F, 0xF8, 0x80, 0x0F, 0xF0, 0x01, 0xF8, 0x00, 0xF0, 0x01, 0x00, 0x07, 0x00, 0x18, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x80, 0x01, 0x80, 0x01, 0xC0, 0x00, 0xB0, 0x01, 0x3A, 0x80, 0x01, 0x00 }; // 'r'
const uint8_t kFont_LG_s[34] = { 0x80, 0x03, 0x1C, 0xC0, 0x01, 0xB8, 0x01, 0xEE, 0x06, 0x67, 0x07, 0x07, 0x0E, 0xCE, 0x38, 0x38, 0xC6, 0xC1, 0x30, 0x0E, 0xC0, 0x38, 0x86, 0x71, 0x8F, 0xB1, 0xC7, 0xC0, 0x30, 0x00, 0x06, 0x60, 0x00, 0x03 }; // 's'
const uint8_t kFont_LG_t[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xE0, 0x04, 0xF0, 0xCF, 0xF0, 0x39, 0xE6, 0xC3, 0xF1, 0x07, 0xC6, 0x07, 0xE8, 0x03, 0xF0, 0x01, 0xD8, 0x00, 0x30, 0x00, 0x02, 0x00, 0x00, 0x00 }; // 't'
const uint8_t kFont_LG_u[34] = { 0x60, 0xC0, 0x1B, 0xDF, 0xF9, 0xB8, 0x0F, 0xFE, 0x01, 0x7F, 0x00, 0x33, 0x00, 0x00, 0x00, 0x06, 0x00, 0x30, 0x01, 0x80, 0x07, 0x00, 0x0E, 0x00, 0x0F, 0xC0, 0x07, 0xF8, 0x80, 0x0F, 0x7C, 0xE0, 0x01, 0x03 }; // 'u'
const uint8_t kFont_LG_v[34] = { 0x00, 0xC0, 0x00, 0x2F, 0xF8, 0x86, 0x8F, 0xFD, 0x41, 0x7F, 0x00, 0x33, 0x00, 0x00, 0x00, 0x06, 0x00, 0x30, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x03, 0xF8, 0x80, 0x0F, 0x7C, 0xE0, 0x01, 0x03 }; // 'v'
const uint8_t kFont_LG_w[34] = { 0x60, 0xC0, 0x1B, 0xDF, 0xF9, 0xB8, 0x0F, 0xF2, 0x81, 0x78, 0xE0, 0x34, 0xF0, 0x0F, 0xF0, 0x39, 0xC0, 0xC3, 0x00, 0x06, 0x00, 0x00, 0x06, 0x00, 0x0F, 0xC0, 0x07, 0xF8, 0x80, 0x0F, 0x7C, 0xE0, 0x01, 0x03 }; // 'w'
const uint8_t kFont_LG_x[34] = { 0xE0, 0x03, 0x07, 0x10, 0x00, 0x01, 0x70, 0x10, 0x38, 0x98, 0x18, 0xF0, 0x01, 0x00, 0x37, 0x00, 0xD8, 0x0D, 0x01, 0x76, 0x06, 0xC0, 0x0F, 0x8C, 0x0C, 0x0E, 0x04, 0x07, 0x40, 0x00, 0x04, 0x60, 0x00, 0x03 }; // 'x'
const uint8_t kFont_LG_y[34] = { 0x00, 0x00, 0x18, 0xC1, 0x39, 0xB9, 0x7F, 0xFE, 0x39, 0x7F, 0x38, 0x37, 0x70, 0x0E, 0xC0, 0x39, 0x00, 0xCE, 0x00, 0x70, 0x00, 0xC0, 0x07, 0x80, 0x0F, 0xC0, 0x07, 0xF8, 0x80, 0x0F, 0x7C, 0xE0, 0x01, 0x03 }; // 'y'
const uint8_t kFont_LG_z[34] = { 0xE0, 0x03, 0x1F, 0xD0, 0x01, 0x39, 0x70, 0x1E, 0x38, 0x1F, 0x18, 0x37, 0x00, 0xCE, 0x30, 0x38, 0xC6, 0xC1, 0x31, 0x06, 0xC6, 0x00, 0x88, 0x0D, 0x80, 0x0F, 0xC0, 0x07, 0x70, 0x00, 0x06, 0x60, 0x00, 0x03 }; // 'z'
const uint8_t kFont_LG_DEGREE[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // '°'

// Elements (271px)
const uint8_t kElement_HOURGLASS_LARGE[34] = { 0x00, 0x00, 0x0C, 0xA0, 0x00, 0x12, 0x40, 0x04, 0x10, 0x02, 0x08, 0x02, 0x08, 0xE4, 0x1F, 0x90, 0x00, 0x80, 0x04, 0xFC, 0x13, 0x08, 0x20, 0x08, 0x20, 0x04, 0x10, 0x01, 0x24, 0x80, 0x02, 0x18, 0x00, 0x00 };
const uint8_t kElement_ARROW_UP[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x01, 0xE1, 0x00, 0xE2, 0x00, 0xD8, 0x01, 0xE0, 0x03, 0xC0, 0x03, 0xC0, 0x07, 0xE0, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Additional elements
const uint8_t kElement_MAZE_1[34] = { 0x00, 0xF8, 0x4F, 0x80, 0xF4, 0x97, 0x01, 0xA5, 0xBE, 0x52, 0xA1, 0x52, 0x5F, 0xA5, 0x42, 0x95, 0xEA, 0xAA, 0x54, 0xA9, 0x52, 0x5D, 0xA5, 0x82, 0xA5, 0xBE, 0x52, 0xA0, 0xF4, 0x97, 0x00, 0xB9, 0x0F, 0x02 }; // Maze 1
const uint8_t kElement_SUV[34] = { 0x00, 0x38, 0x40, 0x1E, 0x44, 0x82, 0x48, 0x20, 0x1C, 0x20, 0x08, 0x40, 0x10, 0x00, 0x41, 0x00, 0x08, 0x02, 0x40, 0xF0, 0x00, 0x41, 0x02, 0x4E, 0x02, 0xC8, 0x01, 0x44, 0x00, 0x11, 0xE0, 0x01, 0x00, 0x00 }; // SUV
const uint8_t kElement_TROPHY[34] = { 0x00, 0x00, 0x08, 0xE0, 0x00, 0x14, 0x00, 0x07, 0xC0, 0xE2, 0xBF, 0xF2, 0x1F, 0xE7, 0x7F, 0x17, 0x83, 0xE7, 0x08, 0x18, 0x21, 0x30, 0x40, 0x20, 0x40, 0x10, 0x20, 0x04, 0x88, 0x00, 0x0F, 0x30, 0x00, 0x00 }; // Trophy
const uint8_t kElement_ARROW_DOWN[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x03, 0xF0, 0x01, 0xE0, 0x01, 0xE0, 0x03, 0xC0, 0x0D, 0x80, 0x23, 0x80, 0x43, 0xC0, 0x01, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // arrow down
const uint8_t kElement_CHECK[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x60, 0x00, 0xC0, 0x00, 0x00, 0x03, 0x00, 0x0C, 0x00, 0x18, 0x00, 0x18, 0x00, 0x0C, 0x00, 0x03, 0x60, 0x00, 0x06, 0x20, 0x00, 0x00 }; // check
const uint8_t kElement_CROSS[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x44, 0x80, 0x33, 0xE0, 0x42, 0x70, 0x00, 0x71, 0x00, 0xE8, 0x00, 0xC0, 0x01, 0xD0, 0x01, 0xE0, 0x02, 0x20, 0x02, 0x10, 0x01, 0xC0, 0x00, 0x04, 0x00, 0x00, 0x00 }; // cross
const uint8_t kElement_DIAMOND[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x03, 0xFF, 0x01, 0xFF, 0x01, 0xFE, 0x03, 0xF8, 0x0F, 0xE0, 0x3F, 0xC0, 0x7F, 0xC0, 0x7F, 0xE0, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // diamond
const uint8_t kElement_DIGICLOCKBOUNDS[34] = { 0xFF, 0x0F, 0xB0, 0x00, 0x13, 0x60, 0x04, 0x18, 0x02, 0x0F, 0xC2, 0x0C, 0x64, 0x18, 0x70, 0x60, 0xC0, 0x00, 0x83, 0x05, 0x8C, 0x11, 0xD8, 0x20, 0x38, 0x20, 0x0C, 0x10, 0x03, 0x64, 0x80, 0x06, 0xF8, 0x7F }; // digiclockbounds
const uint8_t kElement_HEART[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFF, 0x80, 0xFF, 0x00, 0xFF, 0x01, 0xFC, 0x07, 0xE0, 0x1F, 0x00, 0x3E, 0x00, 0x3E, 0x00, 0x1F, 0x80, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // heart
const uint8_t kElement_RING[34] = { 0x00, 0x00, 0x00, 0x3F, 0xF8, 0x0F, 0xCF, 0xC3, 0xC1, 0x61, 0x80, 0x61, 0x00, 0xC3, 0x01, 0x0E, 0x06, 0x30, 0x38, 0xC0, 0x61, 0x00, 0xC3, 0x00, 0xC3, 0xC1, 0xE1, 0x79, 0xF8, 0x0F, 0x7E, 0x00, 0x00, 0x00 }; // ring
const uint8_t kElement_STAR[34] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xC0, 0x80, 0x31, 0x80, 0x1F, 0x00, 0x3F, 0x00, 0xFC, 0x00, 0xE0, 0x1F, 0xC0, 0xFF, 0x80, 0x3F, 0x80, 0x3B, 0x00, 0x18, 0x00, 0x0C, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 }; // star

class PulseHexa : public Pattern, PaletteRotation<CRGBPalette256> {
public:
  HexaShells hexaShells;
  PulseHexa() {
    maxColorJump = 30;
    secondsPerPalette = 15;
  }

  void update() {
    for (int s = 0 ; s < hexaShells.shells.size(); ++s) {
      for (std::optional<PixelIndex> pxOpt : hexaShells.shells[s]) {
        if (!pxOpt.has_value()) continue;
        PixelIndex px = pxOpt.value();
        uint8_t brightness = beatsin8(60, 0, 255, 0, -beatsin16(2, 250, 350)*s/hexaShells.shells.size());
        brightness = scale8(brightness, brightness);
        // ctx.leds[px] = CHSV(millis()/20+s*10, 0xFF, brightness);
        CRGB c = this->getMirroredPaletteColor(millis()/100 + s*15);
        c = c.scale8(brightness);
        ctx.leds[px] = c;
      }
    }
  }

  const char *description() {
    return "PulseHexa";
  }
};

class PulseHexaSmooth : public Pattern, AmplitudeReceiver, PaletteRotation<CRGBPalette256> {
public:
  AxialT<int32_t> center;
  PulseHexaSmooth() : AmplitudeReceiver(audioInput) {
    maxColorJump = 30;
    secondsPerPalette = 9;
  }

  vector32 smoothAcc;

  void update() {
    constexpr int mult = 1000; // smooth everything with integer math
    auto agmt = MotionManager::motionFrame.agmt;

    vector32 acc = vector32(agmt.acc.axes.x, agmt.acc.axes.y, agmt.acc.axes.z) * 5;
    const int smoooooth = 10;
    smoothAcc = (smoooooth * smoothAcc + acc) / (smoooooth+1);

    constexpr int kInverseRootThree = mult*1/sqrt(3);
    AxialT<int32_t> offcenter = center;
    int q = offcenter.q() + smoothAcc.x + kInverseRootThree * smoothAcc.y / mult;
    int r = offcenter.r() - smoothAcc.y;
    offcenter.setQR(q,r);

    int amplitude = amplitudeFrame();

    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      AxialT<int32_t> ax(axial.axialFromPixelIndex(px));
      ax *= mult;
      
      const int kAccScale = 20000;
      const int kAmpScale = 600;
      const int kLocScale = 2000000;
      // full static glitch with no fade
      // int glitchIt = (smoothAcc.z>0 ? (ax.q()*ax.r()*ax.s()) * smoothAcc.z/500000. : 0);
      // smooth-transition glitch that also reacts to sound
      int glitchIt = (smoothAcc.z<0 ? (ax.q()*ax.r()*ax.s())/kLocScale * (1 + amplitude/kAmpScale) * smoothAcc.z/kAccScale: 0);
      int distance = max(max(abs(offcenter.q() - ax.q()), abs(offcenter.r() - ax.r())), abs(offcenter.s() - ax.s())) + glitchIt;
      
      uint8_t brightness = beatsin8(60, 0, 255, 0, -beatsin16(2, 250, 350)*distance/(kMeridian/2)/mult);
      CRGB c = this->getMirroredPaletteColor(millis()/100 + distance*15/mult + beatsin8(3, 0, kMeridian));
      c = c.scale8(brightness);
      ctx.leds[px] = c;
    }
  }

  const char *description() {
    return "PulseHexaSmooth";
  }
};


/* Concept
  PulseHexa except each shell is a looped palette which rotates as you rotate the hexagon.
  Hexa zooms in and out with motion along z axis?
  in any case add parameters and link them to motion
*/
// FIXME: this has a continuity issue where the animation jumps across some probably modulus overflow OH or in the accAccum??
class MotionHexa : public Pattern, PaletteRotation<CRGBPalette256> {
public:
  HexaShells hexaShells;
  MotionHexa() {
    secondsPerPalette = 16;
    maxColorJump = 30;
  }

  vector32 gyrAccum32;
  vector32 accAccum32;

  void update() {
    const int accScale = (MotionManager::manager().enableDMP ? 1000 : 2000); // coolcool cool coooool
    const int gyrScale = (MotionManager::manager().enableDMP ? 200 : 2000); // coolcoolcool
    ICM_20948_AGMT_t agmt = MotionManager::motionFrame.agmt;
    gyrAccum32 += vector16(agmt.gyr.axes.x, agmt.gyr.axes.y, agmt.gyr.axes.z);
    accAccum32 += vector16(agmt.acc.axes.x, agmt.acc.axes.y, agmt.acc.axes.z);
    vector32 gyrAccum = gyrAccum32 / gyrScale;
    vector32 accAccum = accAccum32 / accScale;
    // logf("gyr = (%i, %i, %i), gyrAccum = (%i, %i, %i), accel = (%i, %i, %i), accelAccum = (%i, %i, %i)", 
    //         agmt.gyr.axes.x/gyrScale, agmt.gyr.axes.y/gyrScale, agmt.gyr.axes.z/gyrScale,
    //         gyrAccum.x, gyrAccum.y, gyrAccum.z,
    //         agmt.acc.axes.x/accScale, agmt.acc.axes.y/accScale, agmt.acc.axes.z/accScale,
    //         accAccum.x, accAccum.y, accAccum.z);
    
    int index = 0;
    int shellCount = hexaShells.shells.size();
    for (int s = 0 ; s < hexaShells.shells.size(); ++s) {
      uint8_t shellSize = hexaShells.shells[s].size();
      
      const int32_t bandIndex = gyrAccum.x*2; // TODO: tune this so it's roughly one half index change every complete flip
      const int32_t bandRotate = accAccum.x;
      const int32_t bandTwist = accAccum.y;//gyrAccum.z*2;
      const int32_t bandThing = 0;//accAccum.x;
      const int bandCounts[] = {0, 1, 2, 3, 6, 9}; // i like this somewhat better than arbitrary band counts
      int32_t bands = bandCounts[((int32_t)(bandIndex+INT16_MAX) / (1<<12)) % ARRAY_SIZE(bandCounts)];
      int32_t withinBand = (int32_t)(bandIndex+INT16_MAX-(1<<11)) % (1<<12);
      uint8_t bandFadeIn = 0xFF - cos8(0xFF*withinBand / (1<<12));
      
      // fade in at start
      const long fadeinDuration = 1000;
      // uint8_t shellBrightness = runTime() < fadeinDuration ? max(0, min(0xFF, 0xFF * (runTime() - fadeinDuration/hexaShells.shells.size()*s)/fadeinDuration * (hexaShells.shells.size() - s) / hexaShells.shells.size())) : 0xFF;

      uint8_t shellBrightness = 0xFF;
      if (runTime() < fadeinDuration) {
        long fadeOverlap = hexaShells.shells.size()/2;
        long shellFadeTime = fadeinDuration/(hexaShells.shells.size() + fadeOverlap);
        shellBrightness = (runTime() > s * shellFadeTime ? min(0xFF, 0xFF * (runTime() - s*shellFadeTime) / (fadeOverlap * shellFadeTime)) : 0);
      }

      for (int si = 0; si < hexaShells.shells[s].size(); ++si) {
        auto pxOpt = hexaShells.shells[s][si];
        if (!pxOpt.has_value()) continue;
        PixelIndex px = pxOpt.value();

        uint8_t brightness = lerp8by8(sin8(-bandRotate/4 + bands*(0xFF*si - bandTwist) / shellSize - 0xFF * (s-bandThing)/shellCount), 0xFF, bandFadeIn);

        brightness = scale8(brightness, brightness);
        int32_t gyrRotate = (gyrAccum.z/2) % 0x200;
        int32_t radialH =  0x200 * si / shellSize;
        int32_t twistFactor = (s * gyrAccum.y/8 + s * millis()/500) % 0x200;
        int32_t shellH = 0x200 * s/shellCount * beatsin16(3, 0, 0x200, 0, gyrAccum.x) / 0x200;
        int32_t evolve = (millis()/100)%0x200;
        CRGB c = this->getMirroredPaletteColor(gyrRotate + radialH + twistFactor + shellH + evolve);
        
        // improvement: do this in certain accelerometer conditions
        // if (si%2) {
        //   brightness = scale8(brightness, beatsin8(10));
        // } else {
        //   brightness = scale8(brightness, beatsin8(10, 0, 0xFF, 0, 0x7F));
        // }
        c.nscale8(brightness);
        c.nscale8(shellBrightness);
        ctx.leds[px] = c;
      }
    }
  }

  const char *description() {
    return "MotionHexa";
  }
};

/* ------------------------------------------------------------------------------- */

// Ten dials share one clock engine, tilt-controlled only (no microphone). The
// hexa rests standing up on one of its side edges, 6 o'clock pointing down at
// the ground -- that's the reference orientation that keeps true time. Rolling it
// right (the 3 o'clock edge dips) speeds time up; rolling it left (the 9 o'clock
// edge dips) slows it and then reverses it, both exponentially so a small roll
// barely nudges the time but a full roll spins through hours per second, for
// quickly setting the time. Tipping the top of the face away from you cycles to
// the next dial, tipping it toward you cycles to the previous one -- either
// direction, one step per tip. Tipping it all the way onto its back and holding
// for a second snaps the second hand back to 12. Front/back tipping never
// affects the time itself. Uses the same accelerometer smoothing approach as
// IshiharaDigits, the same hexline()-from-center technique that
// TriangleSpin/PridefulSpinnyThing use to draw radiating lines, and the same 5x7
// hex-shear-corrected digit font technique as IshiharaDigits for the two digital
// dials.
// brightened well above the terracotta background (168,99,46) at every stop so
// the hands never wash into it
DEFINE_GRADIENT_PALETTE( EarthToneHands_gp ) {
  0,   0x6E, 0x50, 0x32, // deep umber, brightened
  42,  0xDC, 0x82, 0x3C, // terracotta / rust, brightened
  85,  0xEB, 0xA5, 0x4B, // ochre / amber gold, brightened
  128, 0xA5, 0xB9, 0x69, // olive / sage, brightened
  170, 0xC3, 0x7D, 0x5F, // dusty clay, brightened
  212, 0x82, 0x9B, 0x6E, // deep moss, brightened
  255, 0x6E, 0x50, 0x32, // wraps back to deep umber
};

// 0 Earth:   earth-tone cycling hands, SWEEPING second hand, over a slow organic single-hue terracotta background
// 1 Grey:    white hour/minute hands, TICKING red second hand, slow offset grey background, single-pixel odd-hour ticks
// 2 Black:   fully black face, orange/purple hands, SWEEPING green second hand, white markers at 12/3/6/9
// 3 Lagoon:  deep single-hue teal background, bold white/gold hands, TICKING coral second hand
// 4 Sunset:  warm horizon-to-night-sky gradient that subtly drifts over time, black hour hand, SWEEPING pink second hand, 12 o'clock marker glows hour-hand-color near that hour
// 5 Neon:    black face with a pulsing violet core glow, vivid cyan/magenta hands, SWEEPING chartreuse second hand
// 6 Comet:   deep-space background, hands fade dim-to-bright along their length with a short tail, TICKING white second hand, 12 o'clock marker glows ember-orange near that hour
// 7 Blade:   dual-tone steel gradient with a rotating laser sweep, silver double-edged blade hands with a colored spine, SWEEPING gold second hand
// 8 Circuit: digital HH/MM readout (hex-shear-corrected 5x7 font), all green, white seconds ring
// 9 Amber:   digital HH/MM readout, warm retro all-amber-on-near-black display, TICKING amber seconds ring
enum ClockDial { kEarthDial = 0, kGreyDial, kBlackDial, kLagoonDial, kSunsetDial, kNeonDial, kCometDial, kBladeDial, kCircuitDial, kAmberDial, kDialCount };

class AnalogClock : public Pattern {
public:
  const double kStartSeconds = 6*3600.0 + 20*60.0 + 10.0; // clock starts at 6:20:10
  const unsigned long kColorCycleMS = 90000;      // full earth-tone cycle for the Earth dial's hands, at 1x speed
  const unsigned long kBackgroundCycleMS = 120000; // full background breathing cycle, at 1x speed
  const int kRadialSpread = 24; // how far the background brightness shifts per ring outward from center

  const float kHourHandLength = 5.0f;
  const float kMinuteHandLength = 7.5f;
  const float kSecondHandLength = 9.0f; // reaches the true edge pixel at the odd hour numbers (hex vertices, radius 9)
  const float kMarkerRadius = 8.7f;
  const unsigned long kSlowBackgroundCycleMS = 300000; // Earth/Grey's slower background cycle (5 min vs. the default 2 min)
  // radians; calibrated empirically so 12 o'clock lands where it should when the
  // hexa stands on its correct resting edge. History: pi/2 -> 2*pi/3 (+30 deg,
  // fixed 11 o'clock's position landing where 12 belonged) -> pi (+60 deg more)
  // -> back to 2*pi/3 (-60 deg, pi was reported off by 60 deg) -> pi again
  // (+60 deg CCW, requested together with reversing the tilt axis below).
  const float kClockRotationOffset = M_PI;

  // separate from kClockRotationOffset on purpose -- see update()'s comment.
  // Was 2*pi/3 + pi (reversed from the 2*pi/3 base). Un-reversed back to the
  // 2*pi/3 base, then +pi/3 (60 deg CCW) applied together with the display
  // rotation above, landing back on pi.
  const float kTiltAxisOffset = M_PI;

  // Earth and Grey get their own darker/lighter values below; this pair is only
  // the shared default (currently just Lagoon).
  const uint8_t kBgMinBrightness = 55;
  const uint8_t kBgMaxBrightness = 150;

  // speed is two modes, not a continuous curve: below kTiltThreshold this is
  // standard time keeping and tilt has zero effect at all -- exactly 1x,
  // always. Only past that threshold does tilt do anything (see update()),
  // easing (not linearly) out to kTopSpeedMultiplier at full tilt so a light
  // tilt stays gentle and only a tilt close to vertical reaches the extreme
  // speeds. Front/back tipping (the dial-select gesture) also suppresses speed
  // entirely so switching dials, or holding the reset gesture, never bleeds
  // into the time. Widened from 0.18 -- that let resting noise sneak past it
  // often enough to throw off accuracy.
  const float kTiltThreshold = 0.26f; // small margin easier than 0.32, to make the tilt gesture less demanding
  const float kSpeedSuppressFrontBackThreshold = 0.15f;
  const double kTopSpeedHoursPerSecond = 8.0;
  const double kTopSpeedMultiplier = kTopSpeedHoursPerSecond * 3600.0;

  // dial selection: tip past this threshold to cycle -- toward you = previous,
  // away from you = next, one step per crossing (zone-edge triggered, so holding
  // a tip doesn't repeatedly fire). Tipping much further, essentially flat onto
  // its back, and holding for kResetHoldMS resets the second hand to 12.
  // Widened from 0.35 -- it was triggering on tilts that weren't meant as a
  // dial-change gesture.
  const float kGestureThreshold = 0.5f;
  const unsigned long kDialTransitionMS = 500;
  const float kResetTiltThreshold = 0.85f;
  const unsigned long kResetHoldMS = 1000;

  // tap-to-set-minute: a light tilt right or left (below kTiltThreshold, so it
  // doesn't also engage speed) plus a sharp tap jumps the time by a whole
  // minute forward or backward, and also resets the second hand to 12. The tap
  // itself is a spike in raw (unsmoothed) accel magnitude well above the
  // recent smoothed baseline. Margins loosened slightly across the board so
  // the whole gesture is easier to land.
  const float kTapTiltMinimum = 0.07f; // ~4 degrees; must be a deliberate tilt, not just resting noise
  const float kTapMagnitudeRatio = 1.4f;
  const float kTapMinimumMagnitude = 8000.0f; // absolute floor so a near-zero baseline can't false-trigger
  const unsigned long kTapCooldownMS = 250;

  CRGBPalette16 handPalette = EarthToneHands_gp;

  double simulatedMillis = kStartSeconds * 1000.0;
  vector32 smoothAcc;

  int targetDial = kEarthDial;
  int previousDial = -1;
  unsigned long dialTransitionStart = 0;

  int tiltZone = 0; // -1 forward, 0 neutral, 1 back

  unsigned long resetHoldStart = 0; // 0 = not currently holding the reset gesture
  bool resetFired = false;

  bool tapWasActive = false;
  unsigned long lastTapMs = 0;

  // ground truth for standard time keeping: whenever speed is exactly 1x,
  // simulatedMillis is re-derived from real elapsed millis() since this anchor
  // rather than trusted as an accumulated running sum -- see update()'s comment
  unsigned long realTimeAnchorMs = millis();
  double simulatedMillisAtAnchor = kStartSeconds * 1000.0;

  PixelStorage<LED_COUNT> dialBufferA;
  PixelStorage<LED_COUNT> dialBufferB;

  // outer ring for the digital dials' seconds progress bar, same technique as
  // ChargingPattern's charge ring
  HexaShells hexaShells;
  int ringStartIdx = 0; // index into hexaShells.shells.back() closest to true 12 o'clock

  AnalogClock() {
    updateWhileHidden = true;

    float theta12 = M_PI/2 + kClockRotationOffset;
    auto &outerShell = hexaShells.shells.back();
    float bestDot = -1e9f;
    for (int i = 0; i < (int)outerShell.size(); ++i) {
      if (!outerShell[i].has_value()) continue;
      vectorf pos = axial.rectFromPixelIndex(outerShell[i].value());
      float mag = sqrtf(pos.x*pos.x + pos.y*pos.y);
      if (mag < 0.01f) continue;
      float dot = (pos.x*cosf(theta12) + pos.y*sinf(theta12)) / mag;
      if (dot > bestDot) {
        bestDot = dot;
        ringStartIdx = i;
      }
    }
  }

  static float fmodPositive(double x, double m) {
    double r = fmod(x, m);
    return (float)(r < 0 ? r + m : r);
  }

  void drawHandInto(PixelStorage<LED_COUNT> &buf, float lengthFrac, float length, CRGB color) {
    float theta = M_PI/2 - lengthFrac * 2*M_PI + kClockRotationOffset;
    vectorT<float> tip(length * cosf(theta), length * sinf(theta));
    fAxial tipAxial = axial.rectToHex(tip, 1.0);
    hexline(buf, fAxial(0,0), tipAxial, color);
  }

  // Draws a hand by directly overwriting each sampled pixel instead of blend-
  // brightening like hexline/drawHandInto does. blendBrighten can only ever make
  // a pixel brighter, so it can't draw a hand darker than what's under it (e.g. a
  // black hand), and it can't make one hand visually sit "above" another where
  // they cross -- both are just a per-channel max, order doesn't matter. This
  // does a plain overwrite instead, so it's used for every second hand (drawn
  // last, after hour/minute, so it always wins any overlap) and for any hand
  // that needs to be darker than its background.
  void drawOverwriteLineInto(PixelStorage<LED_COUNT> &buf, float lengthFrac, float length, CRGB color) {
    float theta = M_PI/2 - lengthFrac * 2*M_PI + kClockRotationOffset;
    int steps = (int)(length * 2.0f) + 1;
    for (int i = 0; i <= steps; ++i) {
      float t = length * i / steps;
      vectorT<float> pos(t * cosf(theta), t * sinf(theta));
      Axial ipos = axial.rectToHex(pos, 1.0).cubeRound();
      auto pxOpt = axial.indexAtAxial(ipos);
      if (pxOpt.has_value()) {
        buf.leds[pxOpt.value()] = color;
      }
    }
  }

  // seconds as a progress ring around the outside, same technique (and same
  // graduated brightness along the lit arc) as ChargingPattern's charge ring
  void drawSecondsRingInto(PixelStorage<LED_COUNT> &buf, float secondFrac, CRGB color) {
    auto &outerShell = hexaShells.shells.back();
    int shellSize = outerShell.size();
    int displayLength = (int)(secondFrac * shellSize);
    for (int i = 0; i < displayLength; ++i) {
      auto pxOpt = outerShell[(i + ringStartIdx) % shellSize];
      if (pxOpt.has_value()) {
        CRGB c = color;
        c = c.scale8(0x50 + 0x9F * i / max(1, displayLength));
        buf.leds[pxOpt.value()] = c;
      }
    }
  }

  // Comet hand: fades from dimColor at the pivot to brightColor at the tip (using
  // hexline's per-pixel color callback), plus a short dim counterweight tail
  // extending past the pivot in the opposite direction, like a weighted hand.
  void drawCometHandInto(PixelStorage<LED_COUNT> &buf, float lengthFrac, float length, CRGB dimColor, CRGB brightColor) {
    float theta = M_PI/2 - lengthFrac * 2*M_PI + kClockRotationOffset;
    vectorT<float> tip(length * cosf(theta), length * sinf(theta));
    fAxial tipAxial = axial.rectToHex(tip, 1.0);
    hexline(buf, fAxial(0,0), tipAxial, [dimColor, brightColor](uint8_t progress) {
      return blend(dimColor, brightColor, progress);
    });
    float tailLength = length * 0.18f;
    vectorT<float> tail(tailLength * cosf(theta + M_PI), tailLength * sinf(theta + M_PI));
    fAxial tailAxial = axial.rectToHex(tail, 1.0);
    hexline(buf, fAxial(0,0), tailAxial, dimColor);
  }

  // Blade hand: two edge lines splayed slightly apart from the pivot plus a
  // bright spine line straight down the middle, reading as a wide double-edged
  // blade rather than a thin single line.
  void drawBladeHandInto(PixelStorage<LED_COUNT> &buf, float lengthFrac, float length, CRGB edgeColor, CRGB spineColor, float halfWidthTurns) {
    drawHandInto(buf, lengthFrac - halfWidthTurns, length, edgeColor);
    drawHandInto(buf, lengthFrac + halfWidthTurns, length, edgeColor);
    drawHandInto(buf, lengthFrac, length, spineColor);
  }

  // classic continuous dot-matrix digit font (not segment-derived): every digit
  // is one unbroken stroke top to bottom, which reads more cleanly at this size
  // than the previous 7-segment-style font (e.g. its "1" and "7" had a blank
  // gap in the middle row where the missing middle segment would have been).
  const uint8_t kDigitFont[10][7] = {
    {0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110}, // 0
    {0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110}, // 1
    {0b01110,0b10001,0b00001,0b00010,0b00100,0b01000,0b11111}, // 2
    {0b11111,0b00010,0b00100,0b00010,0b00001,0b10001,0b01110}, // 3
    {0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010}, // 4
    {0b11111,0b10000,0b11110,0b00001,0b00001,0b10001,0b01110}, // 5
    {0b00110,0b01000,0b10000,0b11110,0b10001,0b10001,0b01110}, // 6
    {0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000}, // 7
    {0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110}, // 8
    {0b01110,0b10001,0b10001,0b01111,0b00001,0b00010,0b01100}, // 9
  };

  // places a single pixel at a "local" (X,Y) offset in the face's own rotated
  // frame (+Y toward 12, +X toward 3) -- rotation-aware generalization of
  // drawHandInto/drawMarkerInto's math, used to lay out the digit dials so they
  // stay correctly oriented under kClockRotationOffset like everything else
  void drawLocalPixelInto(PixelStorage<LED_COUNT> &buf, float localX, float localY, CRGB color) {
    float theta12 = M_PI/2 + kClockRotationOffset;
    float theta3 = theta12 - M_PI/2;
    vectorT<float> rectPos(localX * cosf(theta3) + localY * cosf(theta12), localX * sinf(theta3) + localY * sinf(theta12));
    Axial ipos = axial.rectToHex(rectPos, 1.0).cubeRound();
    auto pxOpt = axial.indexAtAxial(ipos);
    if (pxOpt.has_value()) {
      buf.leds[pxOpt.value()] = color;
    }
  }

  // draws one digit with its top-left corner at local (leftX, topY), growing
  // right and down from there
  void drawDigitGlyphInto(PixelStorage<LED_COUNT> &buf, int digit, float leftX, float topY, CRGB color) {
    if (digit < 0 || digit > 9) return;
    for (int gr = 0; gr < 7; ++gr) {
      uint8_t rowBits = kDigitFont[digit][gr];
      for (int gc = 0; gc < 5; ++gc) {
        if (!((rowBits >> (4 - gc)) & 1)) continue;
        drawLocalPixelInto(buf, leftX + gc, topY - gr, color);
      }
    }
  }

  // two digits (tens, ones) side by side, top edge at local Y=topY
  void drawTwoDigitInto(PixelStorage<LED_COUNT> &buf, int value, float topY, CRGB color) {
    drawDigitGlyphInto(buf, (value / 10) % 10, -5, topY, color);
    drawDigitGlyphInto(buf, value % 10,          1, topY, color);
  }

  // marker at the given clock fraction (0=12, 0.25=3, 0.5=6, 0.75=9): a solid
  // 7-pixel dot (center + its 6 immediate hex neighbors) in `color`; if `withHalo`,
  // a further ring of 12 pixels lights up dimmer in `haloColor` for a bigger,
  // softer glow on top of the solid core
  void drawMarkerInto(PixelStorage<LED_COUNT> &buf, float lengthFrac, CRGB color, bool withHalo = false, CRGB haloColor = CRGB::Black) {
    float theta = M_PI/2 - lengthFrac * 2*M_PI + kClockRotationOffset;
    vectorT<float> pos(kMarkerRadius * cosf(theta), kMarkerRadius * sinf(theta));
    Axial ipos = axial.rectToHex(pos, 1.0).cubeRound();

    const int dirQ[6] = {1, 1, 0, -1, -1, 0};
    const int dirR[6] = {0, -1, -1, 0, 1, 1};

    auto pxOpt = axial.indexAtAxial(ipos);
    if (pxOpt.has_value()) {
      buf.leds[pxOpt.value()] = color;
    }
    for (int i = 0; i < 6; ++i) {
      auto nOpt = axial.indexAtAxial(ipos.q() + dirQ[i], ipos.r() + dirR[i]);
      if (nOpt.has_value()) {
        buf.leds[nOpt.value()] = color;
      }
    }
    if (withHalo) {
      for (int side = 0; side < 6; ++side) {
        int startQ = ipos.q() + 2*dirQ[side];
        int startR = ipos.r() + 2*dirR[side];
        for (int step = 0; step < 2; ++step) {
          int q2 = startQ + step*dirQ[(side+2) % 6];
          int r2 = startR + step*dirR[(side+2) % 6];
          auto oOpt = axial.indexAtAxial(q2, r2);
          if (oOpt.has_value()) {
            buf.leds[oOpt.value()] = haloColor;
          }
        }
      }
    }
  }

  // single-hue radial gradient: only brightness varies with distance from center
  // and a slow time-based phase, so it reads as one dynamic color, not a rainbow
  void renderSingleHueBackground(PixelStorage<LED_COUNT> &buf, CRGB hue, uint8_t bgPhase, uint8_t minBright, uint8_t maxBright) {
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      Axial hexPos = axial.axialFromPixelIndex(px);
      int dist = max(max(abs(hexPos.q()), abs(hexPos.r())), abs(hexPos.s()));
      uint8_t ringPhase = (uint8_t)(dist * kRadialSpread + bgPhase);
      uint8_t brightness = minBright + scale8(sin8(ringPhase), maxBright - minBright);
      CRGB color = hue;
      color.nscale8(brightness);
      buf.leds[px] = color;
    }
  }

  // like renderSingleHueBackground but blends three overlapping sine waves at
  // different angles/frequencies instead of pure concentric rings, for a more
  // irregular, organic-feeling texture in the same single hue
  void renderOrganicBackground(PixelStorage<LED_COUNT> &buf, CRGB hue, uint8_t phase, uint8_t minBright, uint8_t maxBright) {
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      vectorf pos = axial.rectFromPixelIndex(px);
      uint8_t w1 = sin8((uint8_t)(int)(pos.x*11 + pos.y*6  + phase));
      uint8_t w2 = sin8((uint8_t)(int)(pos.x*4  - pos.y*9  + phase*2/3 + 85));
      uint8_t w3 = sin8((uint8_t)(int)(-pos.x*7 + pos.y*13 + phase/2 + 170));
      uint8_t blended = (uint8_t)(((uint16_t)w1 + w2 + w3) / 3);
      uint8_t brightness = minBright + scale8(blended, maxBright - minBright);
      CRGB color = hue;
      color.nscale8(brightness);
      buf.leds[px] = color;
    }
  }

  // single pixel at the given clock fraction, no halo/expansion -- a lighter
  // mark than drawMarkerInto's 7-pixel solid dot, for secondary hour ticks
  void drawTickMarkInto(PixelStorage<LED_COUNT> &buf, float lengthFrac, CRGB color) {
    float theta = M_PI/2 - lengthFrac * 2*M_PI + kClockRotationOffset;
    vectorT<float> pos(kMarkerRadius * cosf(theta), kMarkerRadius * sinf(theta));
    Axial ipos = axial.rectToHex(pos, 1.0).cubeRound();
    auto pxOpt = axial.indexAtAxial(ipos);
    if (pxOpt.has_value()) {
      buf.leds[pxOpt.value()] = color;
    }
  }

  // 1.0 exactly at hourFrac=0 (the 12 o'clock hour), fading to 0.0 by
  // hourFrac=0.5 (6 o'clock) -- used to fade the 12 o'clock marker into the
  // hour hand's color specifically while the hour hand is near it
  static float hourProximityTo12(float hourFrac) {
    float centered = fabsf(fmodPositive((double)hourFrac + 0.5, 1.0) - 0.5f) * 2.0f;
    return constrain(1.0f - centered, 0.0f, 1.0f);
  }

  void renderDial(int dial, PixelStorage<LED_COUNT> &buf, float hourFrac, float minuteFrac, float secondFrac, float tickingSecondFrac, uint8_t colorPhase, uint8_t bgPhase, double simulatedMillis) {
    switch (dial) {
      case kGreyDial: {
        // own slower background cycle, phase-offset from the shared bgPhase so
        // it doesn't breathe in sync with other dials
        uint8_t greyBgPhase = (uint8_t)(fmodPositive(simulatedMillis, (double)kSlowBackgroundCycleMS) * 256.0 / kSlowBackgroundCycleMS);
        greyBgPhase = (uint8_t)(greyBgPhase + 64);
        renderSingleHueBackground(buf, CRGB(150, 150, 150), greyBgPhase, 40, 130);
        drawMarkerInto(buf, 0.0f, CRGB::White);
        for (int h = 1; h <= 11; h += 2) drawTickMarkInto(buf, h / 12.0f, CRGB::White); // single-pixel odd-hour ticks
        drawHandInto(buf, hourFrac,   kHourHandLength,   CRGB::White);
        drawHandInto(buf, minuteFrac, kMinuteHandLength, CRGB::White);
        drawOverwriteLineInto(buf, tickingSecondFrac, kSecondHandLength, CRGB::Red); // ticks, drawn on top
        buf.leds[kHexaCenterIndex] = CRGB::White;
        break;
      }
      case kBlackDial:
        buf.leds.fill_solid(CRGB::Black);
        drawMarkerInto(buf, 0.0f,  CRGB::White); // 12
        drawMarkerInto(buf, 0.25f, CRGB::White); // 3
        drawMarkerInto(buf, 0.5f,  CRGB::White); // 6
        drawMarkerInto(buf, 0.75f, CRGB::White); // 9
        drawHandInto(buf, hourFrac,   kHourHandLength,   CRGB(255, 110, 0));   // orange
        drawHandInto(buf, minuteFrac, kMinuteHandLength, CRGB(170, 40, 230));  // purple
        drawOverwriteLineInto(buf, secondFrac, kSecondHandLength, CRGB(40, 220, 90)); // green, sweeps, on top
        buf.leds[kHexaCenterIndex] = CRGB::White;
        break;
      case kLagoonDial: {
        // deep single-hue teal, same proven radial technique as Earth/Grey, with
        // maximally distinct/high-contrast hands so it's easy to read at a glance
        renderSingleHueBackground(buf, CRGB(15, 95, 85), bgPhase, kBgMinBrightness, kBgMaxBrightness);
        drawMarkerInto(buf, 0.0f, CRGB::White, true, CRGB(20, 100, 90));
        drawHandInto(buf, hourFrac,   kHourHandLength,   CRGB(245, 250, 255)); // near-white
        drawHandInto(buf, minuteFrac, kMinuteHandLength, CRGB(255, 190, 40));  // golden amber
        drawOverwriteLineInto(buf, tickingSecondFrac, kSecondHandLength, CRGB(255, 80, 50)); // coral, ticks, on top
        buf.leds[kHexaCenterIndex] = CRGB::White;
        break;
      }
      case kSunsetDial: {
        float theta12 = M_PI/2 + kClockRotationOffset;
        float dir12x = cosf(theta12), dir12y = sinf(theta12);
        uint8_t pulse = 220 + scale8(sin8(bgPhase), 35); // gentle breathing, stays mostly bright
        // slow +-1.5 unit drift on where the gradient transitions, so the
        // horizon line subtly rises and falls over time instead of sitting still
        float drift = (sin8(bgPhase) / 255.0f - 0.5f) * 3.0f;
        for (PixelIndex px = 0; px < LED_COUNT; ++px) {
          vectorf rectPos = axial.rectFromPixelIndex(px);
          float posAlong12 = rectPos.x * dir12x + rectPos.y * dir12y + drift; // ~-9 (toward 6) .. +9 (toward 12)
          uint8_t t = (uint8_t)(constrain((posAlong12 + 9.0f) / 18.0f, 0.0f, 1.0f) * 255);
          CRGB color = blend(CRGB(255, 90, 30), CRGB(20, 15, 50), t); // warm horizon (6) to deep sky (12)
          color.nscale8(pulse);
          buf.leds[px] = color;
        }
        // the 12 o'clock marker fades into the hour hand's (black) color
        // specifically while the hour hand is near 12, otherwise it's the pale
        // sun color
        CRGB sunsetMarkerColor = blend(CRGB(255, 250, 200), CRGB::Black, (uint8_t)(hourProximityTo12(hourFrac) * 255));
        drawMarkerInto(buf, 0.0f, sunsetMarkerColor);
        // black can't be drawn with blendBrighten (max-with-existing can never
        // go darker), so the hour hand needs the overwrite technique too
        drawOverwriteLineInto(buf, hourFrac, kHourHandLength, CRGB::Black);
        drawHandInto(buf, minuteFrac, kMinuteHandLength, CRGB(255, 170, 40));  // golden orange
        drawOverwriteLineInto(buf, secondFrac, kSecondHandLength, CRGB(255, 60, 140)); // hot pink, on top
        buf.leds[kHexaCenterIndex] = CRGB(255, 200, 120);
        break;
      }
      case kNeonDial: {
        uint8_t pulse = sin8(bgPhase);
        for (PixelIndex px = 0; px < LED_COUNT; ++px) {
          Axial hexPos = axial.axialFromPixelIndex(px);
          int dist = max(max(abs(hexPos.q()), abs(hexPos.r())), abs(hexPos.s()));
          float falloff = max(0.0f, 1.0f - dist / 9.0f);
          uint8_t glow = (uint8_t)(falloff * falloff * scale8(pulse, 90));
          CRGB color(70, 10, 110); // deep violet core glow
          color.nscale8(glow);
          buf.leds[px] = color;
        }
        drawMarkerInto(buf, 0.0f, CRGB::White, true, CRGB(90, 20, 140));
        drawHandInto(buf, hourFrac,   kHourHandLength,   CRGB(0, 220, 255));   // electric cyan
        drawHandInto(buf, minuteFrac, kMinuteHandLength, CRGB(255, 20, 180));  // hot magenta
        drawOverwriteLineInto(buf, secondFrac, kSecondHandLength, CRGB(170, 255, 30)); // chartreuse, on top
        buf.leds[kHexaCenterIndex] = CRGB::White;
        break;
      }
      case kCometDial: {
        renderSingleHueBackground(buf, CRGB(8, 10, 30), bgPhase, 15, 45); // deep space navy
        // the marker fades from white into the hour hand's bright ember-orange
        // specifically while the hour hand is near 12
        CRGB cometMarkerColor = blend(CRGB::White, CRGB(255, 140, 20), (uint8_t)(hourProximityTo12(hourFrac) * 255));
        drawMarkerInto(buf, 0.0f, cometMarkerColor, true, CRGB(40, 45, 90));
        drawCometHandInto(buf, hourFrac,   kHourHandLength,   CRGB(60, 15, 5),  CRGB(255, 140, 20)); // ember orange
        drawCometHandInto(buf, minuteFrac, kMinuteHandLength, CRGB(5, 15, 50),  CRGB(60, 200, 255)); // electric blue
        drawOverwriteLineInto(buf, tickingSecondFrac, kSecondHandLength, CRGB::White); // ticks, on top
        buf.leds[kHexaCenterIndex] = CRGB(200, 210, 255);
        break;
      }
      case kBladeDial: {
        // dual-tone steel gradient (center to edge) plus a rotating laser sweep
        // with a fading trail layered on top, tied to simulated time so it
        // speeds up/reverses right along with the hands
        for (PixelIndex px = 0; px < LED_COUNT; ++px) {
          Axial hexPos = axial.axialFromPixelIndex(px);
          int dist = max(max(abs(hexPos.q()), abs(hexPos.r())), abs(hexPos.s()));
          uint8_t distT = (uint8_t)(constrain(dist / 9.0f, 0.0f, 1.0f) * 255);
          CRGB color = blend(CRGB(28, 34, 42), CRGB(14, 16, 20), distT); // steel-blue center to darker edge
          uint8_t ringPhase = (uint8_t)(dist * kRadialSpread + bgPhase);
          color.nscale8(30 + scale8(sin8(ringPhase), 40));
          buf.leds[px] = color;
        }
        const float kLaserPeriodSeconds = 6.0f; // one sweep every 6 simulated seconds
        float laserAngle = fmodPositive(simulatedMillis / 1000.0, (double)kLaserPeriodSeconds) / kLaserPeriodSeconds * 2*M_PI;
        for (PixelIndex px = 0; px < LED_COUNT; ++px) {
          vectorf pos = axial.rectFromPixelIndex(px);
          float pixelAngle = atan2f(pos.y, pos.x);
          float angleDiff = fmodPositive((double)(laserAngle - pixelAngle), 2*M_PI);
          float trail = max(0.0f, 1.0f - (float)(angleDiff / (M_PI/2.5)));
          if (trail <= 0) continue;
          uint8_t laserBrightness = (uint8_t)(trail * trail * 200);
          CRGB lit(80, 200, 255);
          lit.nscale8(laserBrightness);
          CRGB c = buf.leds[px];
          buf.leds[px] = CRGB(max(c.r, lit.r), max(c.g, lit.g), max(c.b, lit.b));
        }
        drawMarkerInto(buf, 0.0f, CRGB(220, 225, 235));
        CRGB edge(220, 225, 235); // shared silver edges, colored spine per hand
        drawBladeHandInto(buf, hourFrac,   kHourHandLength,   edge, CRGB(200, 30, 30),  0.010f); // crimson spine
        drawBladeHandInto(buf, minuteFrac, kMinuteHandLength, edge, CRGB(30, 180, 200), 0.008f); // cyan spine
        drawOverwriteLineInto(buf, secondFrac, kSecondHandLength, CRGB(255, 200, 40)); // gold, on top; blade splay reserved for hour/minute
        buf.leds[kHexaCenterIndex] = edge;
        break;
      }
      case kCircuitDial: {
        renderSingleHueBackground(buf, CRGB(10, 25, 15), bgPhase, 15, 45); // dark circuit-board green
        int hourVal = ((int)(hourFrac * 12)) % 12; if (hourVal == 0) hourVal = 12;
        int minuteVal = ((int)(minuteFrac * 60)) % 60;
        CRGB green(50, 230, 90);
        drawTwoDigitInto(buf, hourVal, 7, green);
        drawTwoDigitInto(buf, minuteVal, -1, green);
        drawLocalPixelInto(buf, 0, 0, CRGB(20, 80, 35));  // dim green separator accent
        drawSecondsRingInto(buf, secondFrac, CRGB::White);
        drawMarkerInto(buf, 0.0f, green);
        buf.leds[kHexaCenterIndex] = green;
        break;
      }
      case kAmberDial: {
        renderSingleHueBackground(buf, CRGB(30, 18, 10), bgPhase, 10, 35); // near-black warm brown
        int hourVal = ((int)(hourFrac * 12)) % 12; if (hourVal == 0) hourVal = 12;
        int minuteVal = ((int)(minuteFrac * 60)) % 60;
        CRGB amber(255, 140, 20);
        drawTwoDigitInto(buf, hourVal, 7, amber);
        drawTwoDigitInto(buf, minuteVal, -1, amber);
        drawLocalPixelInto(buf, 0, 0, CRGB(180, 90, 10));
        drawSecondsRingInto(buf, tickingSecondFrac, CRGB(255, 170, 40));
        drawMarkerInto(buf, 0.0f, amber);
        buf.leds[kHexaCenterIndex] = amber;
        break;
      }
      default: { // kEarthDial
        // own slower background cycle, organic (irregular) texture instead of
        // perfectly concentric rings
        uint8_t earthBgPhase = (uint8_t)(fmodPositive(simulatedMillis, (double)kSlowBackgroundCycleMS) * 256.0 / kSlowBackgroundCycleMS);
        renderOrganicBackground(buf, CRGB(168, 99, 46), earthBgPhase, 18, 65); // terracotta, darkened so hands stand out
        drawMarkerInto(buf, 0.0f, CRGB(230, 210, 160));
        drawHandInto(buf, hourFrac,   kHourHandLength,   ColorFromPalette(handPalette, (uint8_t)(colorPhase + 0)));
        drawHandInto(buf, minuteFrac, kMinuteHandLength, ColorFromPalette(handPalette, (uint8_t)(colorPhase + 85)));
        drawOverwriteLineInto(buf, secondFrac, kSecondHandLength, ColorFromPalette(handPalette, (uint8_t)(colorPhase + 170))); // sweeps, on top
        buf.leds[kHexaCenterIndex] = CRGB(180, 160, 120);
        break;
      }
    }
  }

  void changeDial(int delta, unsigned long nowMs) {
    previousDial = targetDial;
    targetDial = (targetDial + delta + kDialCount) % kDialCount;
    dialTransitionStart = nowMs;
  }

  void update() {
    ICM_20948_AGMT_t agmt = MotionManager::motionFrame.agmt;
    vector32 acc(agmt.acc.axes.x, agmt.acc.axes.y, agmt.acc.axes.z);
    smoothAcc = (10 * smoothAcc + acc) / 11; // heavier than before, to reject noise while resting

    float ax = smoothAcc.x, ay = smoothAcc.y, az = smoothAcc.z;
    float mag = sqrt(ax*ax + ay*ay + az*az);
    // standing at rest (6 o'clock down), gravity sits almost entirely along the
    // face's own 12-6 axis, leaving "3" (right roll) and the face-normal "z" axis
    // (front/back tip) free as independent controls. The roll axis is
    // deliberately its own kTiltAxisOffset rather than being derived from
    // kClockRotationOffset: it used to be ("3" = "12 rotated a quarter turn"),
    // but that meant every visual-rotation calibration also silently changed
    // which physical direction sped time up, which is its own separate thing
    // to get right.
    float theta12 = M_PI/2 + kClockRotationOffset;
    float theta3 = kTiltAxisOffset;
    float tiltRight = (mag > 1) ? constrain((ax*cosf(theta3) + ay*sinf(theta3)) / mag, -1.0f, 1.0f) : 0;
    float tiltFrontBack = (mag > 1) ? constrain(az / mag, -1.0f, 1.0f) : 0;

    // Two modes, deliberately not one continuous blended curve: below
    // kTiltThreshold this is standard time keeping, full stop -- exactly 1x,
    // tilt has zero influence at all, so it's accurate whenever the hexa is
    // just sitting there. Only past that threshold does tilt do anything, and
    // then it's a cubic ease-in (not linear -- linear felt too fast even at a
    // light tilt): tilt right speeds up, tilt left reverses, reaching
    // kTopSpeedMultiplier only very close to full tilt (for quickly setting
    // the time without the ramp feeling twitchy at moderate tilts).
    float absTiltRight = fabs(tiltRight);
    float speedMultiplier;
    if (absTiltRight <= kTiltThreshold) {
      speedMultiplier = 1.0f;
    } else {
      float t = (absTiltRight - kTiltThreshold) / (1.0f - kTiltThreshold); // 0..1 beyond the threshold
      float eased = t * t * t;
      float speedMag = 1.0f + eased * ((float)kTopSpeedMultiplier - 1.0f);
      speedMultiplier = (tiltRight < 0) ? -speedMag : speedMag;
    }
    if (fabs(tiltFrontBack) > kSpeedSuppressFrontBackThreshold) {
      // mid dial-select gesture: hold time exactly steady regardless of tiltRight
      speedMultiplier = 1.0f;
    }

    unsigned long nowMs = millis();

    // Ground truth check: whenever speed is exactly 1x (standard time keeping),
    // don't trust the accumulated per-frame sum -- re-derive simulatedMillis
    // directly from real elapsed millis() since the last anchor point instead.
    // millis() on RP2040 is backed by the chip's always-on hardware timer
    // (clocked independently of the CPU's own clock, precisely so it stays
    // correct even through clock scaling), so it's a reliable ground truth to
    // check against. This makes the clock self-correct any drift a late or
    // skipped update() call could otherwise silently accumulate, rather than
    // just adding frameTime() and hoping every frame was accounted for.
    // Whenever speed isn't 1x (actually tilted) there's no ground truth to
    // check -- integrate frame time as before, and re-anchor so the next
    // return to standard time keeping picks up from exactly here.
    if (speedMultiplier == 1.0f) {
      simulatedMillis = simulatedMillisAtAnchor + (double)(nowMs - realTimeAnchorMs);
    } else {
      simulatedMillis += frameTime() * speedMultiplier;
      realTimeAnchorMs = nowMs;
      simulatedMillisAtAnchor = simulatedMillis;
    }

    // tap-to-set-minute: a light tilt right or left (past kTapTiltMinimum but
    // below kTiltThreshold, so it can't also engage speed) plus a sharp tap
    // jumps the time by a whole minute forward (right) or backward (left). A
    // tap is a spike in raw (unsmoothed) accel magnitude well above the recent
    // smoothed baseline; edge triggered with a cooldown so one physical tap
    // can't double-fire.
    float rawMag = sqrtf((float)agmt.acc.axes.x*agmt.acc.axes.x + (float)agmt.acc.axes.y*agmt.acc.axes.y + (float)agmt.acc.axes.z*agmt.acc.axes.z);
    bool tapNow = rawMag > kTapMinimumMagnitude && rawMag > mag * kTapMagnitudeRatio;
    if (tapNow && !tapWasActive && nowMs - lastTapMs > kTapCooldownMS) {
      if (absTiltRight > kTapTiltMinimum && absTiltRight <= kTiltThreshold) {
        simulatedMillis += (tiltRight > 0 ? 60000.0 : -60000.0);
        double secsIntoMinute = fmodPositive(simulatedMillis / 1000.0, 60.0); // also snap seconds to 12
        simulatedMillis -= secsIntoMinute * 1000.0;
        realTimeAnchorMs = nowMs;
        simulatedMillisAtAnchor = simulatedMillis;
        lastTapMs = nowMs;
      }
    }
    tapWasActive = tapNow;

    // reset gesture: tilted essentially flat onto its back and held there for a
    // full second snaps the second hand to 12, without touching hour/minute.
    // Fires once per hold (not repeatedly), and resets once you tip back out of
    // the extreme zone, ready to fire again next time.
    if (tiltFrontBack > kResetTiltThreshold) {
      if (resetHoldStart == 0) {
        resetHoldStart = nowMs;
      } else if (!resetFired && nowMs - resetHoldStart >= kResetHoldMS) {
        double secsIntoMinute = fmodPositive(simulatedMillis / 1000.0, 60.0);
        simulatedMillis -= secsIntoMinute * 1000.0;
        realTimeAnchorMs = nowMs;
        simulatedMillisAtAnchor = simulatedMillis;
        resetFired = true;
      }
    } else {
      resetHoldStart = 0;
      resetFired = false;
    }

    double totalSeconds = simulatedMillis / 1000.0;
    float hourFrac   = fmodPositive(totalSeconds, 12*3600.0) / (12*3600.0);
    float minuteFrac = fmodPositive(totalSeconds, 3600.0) / 3600.0;
    float secondFrac = fmodPositive(totalSeconds, 60.0) / 60.0;
    float tickingSecondFrac = fmodPositive(floor(totalSeconds), 60.0) / 60.0; // whole-second jumps

    // color and background drift are keyed off simulated time rather than the
    // wall clock, so speeding up or reversing time visibly speeds up or
    // reverses the color/gradient motion too
    uint8_t colorPhase = (uint8_t)(fmodPositive(simulatedMillis, (double)kColorCycleMS) * 256.0 / kColorCycleMS);
    uint8_t bgPhase = (uint8_t)(fmodPositive(simulatedMillis, (double)kBackgroundCycleMS) * 256.0 / kBackgroundCycleMS);

    // dial selection: tip back -> previous dial, tip forward -> next dial, one
    // step per crossing (edge triggered off the zone, so holding a tip doesn't
    // repeatedly fire).
    int newZone = (tiltFrontBack > kGestureThreshold) ? 1 : (tiltFrontBack < -kGestureThreshold ? -1 : 0);
    if (newZone != tiltZone) {
      if (newZone == 1) changeDial(-1, nowMs);      // tip back -> previous
      else if (newZone == -1) changeDial(1, nowMs); // tip forward -> next
      tiltZone = newZone;
    }

    // everything above (motion, timekeeping, gestures) is cheap and always
    // needs to run so the clock stays accurate while hidden; the rendering
    // below is the expensive part (multiple full 271-pixel passes, hexline
    // calls), so skip it when nothing is actually looking at this pattern
    if (alpha == 0) return;

    renderDial(targetDial, dialBufferA, hourFrac, minuteFrac, secondFrac, tickingSecondFrac, colorPhase, bgPhase, simulatedMillis);

    unsigned long transitionElapsed = nowMs - dialTransitionStart;
    if (previousDial >= 0 && transitionElapsed < kDialTransitionMS) {
      renderDial(previousDial, dialBufferB, hourFrac, minuteFrac, secondFrac, tickingSecondFrac, colorPhase, bgPhase, simulatedMillis);
      uint8_t alpha = ease8InOutQuad((uint8_t)(0xFF * transitionElapsed / kDialTransitionMS));
      for (PixelIndex px = 0; px < LED_COUNT; ++px) {
        ctx.leds[px] = blend(dialBufferB.leds[px], dialBufferA.leds[px], alpha);
      }
    } else {
      for (PixelIndex px = 0; px < LED_COUNT; ++px) {
        ctx.leds[px] = dialBufferA.leds[px];
      }
    }
  }

  const char *description() {
    return "AnalogClock";
  }
};

/* ------------------------------------------------------------------------------- */

// Five orientation-driven scenes. Lying face up: the sun sits at the center
// and eight planets orbit it on rings 1-8, one ring per planet, with orbital
// periods following Kepler's third law (period proportional to ring
// radius^1.5) so the *ratios* between planets are physically correct even
// though the absolute speed is picked to be satisfying to watch rather than
// astronomically accurate; fast planets grow a fading tracer, and dim
// single-pixel dots mark which way to tilt for speed (yellow, at 3/9 o'clock)
// and which direction reaches each other scene (green=horizon at 4 o'clock,
// white=moon at 6 o'clock, purple=astrology at 8 o'clock) -- genuine
// alternating hex edges, 60 degrees apart. Lying face down: a star/sky
// viewer that pans a window into a much larger virtual scene as you tilt,
// using the same "shift the render origin by smoothed tilt" technique
// PulseHexaSmooth uses for its pulse center, revealing a fixed moon and a
// fixed sun somewhere out in that larger sky as you tilt toward them, fading
// between a starry night and a pale blue, cloud-flecked day over a slow
// cycle. Horizon: a sun and moon arcing over a small, slightly curved strip
// of a green-and-blue planet, its lighting a real gradient that tracks the
// sun's actual angle even while it's below the horizon (not just a flat
// on/off), fading dim rather than black at night; a black sky behind it with
// 5 twinkling stars that sweep fully on and off screen as they orbit, and an
// occasional random meteor; the moon's sky position is tied to its actual
// phase (near the sun at new moon, opposite it at full moon), not a fixed
// offset. Moon: a phase display using a proper continuous 3D-sphere
// illumination model (not a naive ellipse cut, which had a real
// discontinuity right at full and new moon), projected onto the same
// calibrated left/right axis the clock pattern uses so the terminator sweeps
// left-to-right the way the hexa is actually meant to be viewed, timed purely
// for looks with a deliberately longer dark stretch around new moon.
// Astrology: a slowly rotating, sparkling purple zodiac wheel. The four
// non-solar scenes run on their own fixed pleasant pace, independent of tilt
// -- tilt is spent on scene selection (or, face down, panning the sky) once
// you're off the flat-facing-up orientation, so it isn't also asked to
// control speed there.
class SolarSystem : public Pattern {
public:
  enum Scene { kSolarScene = 0, kMoonScene, kSunriseScene, kAstrologyScene, kSkyScene };

  // orientation: flatness is |z-tilt| / total tilt magnitude, ~1 when lying
  // flat, ~0 standing upright on an edge. Dual thresholds (enter/exit) give
  // hysteresis so it doesn't flicker right at the boundary; exit is set quite
  // low so the standing scenes only kick in close to fully upright, not at a
  // moderate tilt.
  const float kFlatEnterThreshold = 0.75f;
  const float kFlatExitThreshold = 0.25f;

  // which standing scene: matches AnalogClock's own hardware-calibrated
  // kClockRotationOffset and clock-hand-angle convention, so "4", "6", and
  // "8 o'clock" here line up with the same real positions on the physical
  // hexagon that the clock pattern already validated on the actual device.
  // These are 60 degrees apart, i.e. genuine alternating hex edges (stable
  // resting positions), not arbitrary in-between directions.
  const float kClockRotationOffset = M_PI;
  const float kHorizonEdgeAngle    = M_PI/2 - 4*(M_PI/6) + kClockRotationOffset; // "4 o'clock" -> sunrise scene
  const float kMoonEdgeAngle       = M_PI/2 - 6*(M_PI/6) + kClockRotationOffset; // "6 o'clock" -> moon scene
  const float kAstrologyEdgeAngle  = M_PI/2 - 8*(M_PI/6) + kClockRotationOffset; // "8 o'clock" -> astrology scene
  const float kEdgeMarginDeadzone = 0.10f; // how much closer to one direction than the runner-up before committing

  const unsigned long kSceneTransitionMS = 600;
  const unsigned long kMoonFadeInMS = 1400; // moon's own center-outward reveal, longer and subtler than the generic crossfade

  // solar system speed: same dead-zone-then-eased-ramp shape as AnalogClock's
  // tilt speed, tuned more responsive (higher top speed, lower dead zone,
  // squared rather than cubed ease) so a given tilt buys more speed. Reference
  // angle is arbitrary (there's no "12 o'clock" on this scene to anchor to);
  // flip its sign if right/left come out backwards.
  const float kSolarTiltAxisOffset = 0.0f;
  const float kSpeedDeadzone = 0.12f;
  const float kTopSpeedMultiplier = 10.0f;

  // innermost ring's (Mercury's) orbital period at 1x speed; every other
  // planet's period is this times ringRadius^1.5 (Kepler's third law), so
  // Neptune on ring 8 takes 8^1.5 =~ 22.6x as long as Mercury on ring 1.
  // Halved from 3000 for the 2x speedup.
  const double kBaseOrbitPeriodMS = 1500.0;

  const double kMoonCycleMS = 24000.0; // one full new-to-new cycle, purely for looks
  const unsigned long kDayCycleMS = 10667; // full day+night cycle in the sunrise scene (16000 sped up 50%); sun visible for exactly half, moon's position is tied to its actual phase rather than a fixed offset (see renderSunriseScene)
  const unsigned long kStarRotationPeriodMS = 45000; // slow sidereal-style rotation for the sunrise scene's sky stars

  struct Planet { CRGB color; };
  const Planet kPlanets[8] = {
    {CRGB(180, 180, 190)}, // Mercury: grey
    {CRGB(230, 200, 140)}, // Venus: pale gold
    {CRGB(70, 140, 220)},  // Earth: blue
    {CRGB(210, 90, 60)},   // Mars: rust
    {CRGB(220, 170, 110)}, // Jupiter: tan
    {CRGB(225, 195, 150)}, // Saturn: pale gold, slightly lighter than Venus
    {CRGB(150, 220, 220)}, // Uranus: pale cyan
    {CRGB(70, 90, 220)},   // Neptune: deep blue
  };

  double solarSimulatedMillis = 0;
  vector32 smoothAcc;

  int currentScene = kSolarScene;
  int previousScene = -1;
  unsigned long sceneTransitionStart = 0;
  unsigned long moonEnteredAt = 0;
  unsigned long skyEnteredAt = 0; // so the sky scene always starts fresh at night, not wherever global uptime happens to fall

  // random meteor in the sunrise scene: rolled roughly every kMeteorCheckIntervalMs
  // while none is active, so on average one appears every several such checks
  const unsigned long kMeteorCheckIntervalMs = 900;
  const uint8_t kMeteorChancePercent = 10;
  const unsigned long kMeteorDurationMs = 650;
  const float kMeteorTravelDistance = 11.0f;
  bool meteorActive = false;
  unsigned long meteorStartMs = 0;
  unsigned long lastMeteorCheckMs = 0;
  float meteorOriginX = 0, meteorOriginY = 0;
  float meteorDirX = 0, meteorDirY = 0;

  PixelStorage<LED_COUNT> sceneBufferA;
  PixelStorage<LED_COUNT> sceneBufferB;

  static float fmodPositive(double x, double m) {
    double r = fmod(x, m);
    return (float)(r < 0 ? r + m : r);
  }

  void renderSolarScene(PixelStorage<LED_COUNT> &buf, double simMs, float speedMultiplier) {
    buf.leds.fill_solid(CRGB::Black);

    // pulsing sun: center pixel plus its 6 immediate neighbors, dimmer
    uint8_t sunPulse = 200 + scale8(sin8((uint8_t)((long)simMs / 20)), 55);
    CRGB sunColor(255, 200, 60);
    sunColor.nscale8(sunPulse);
    buf.leds[kHexaCenterIndex] = sunColor;
    Axial centerAx = axial.axialFromPixelIndex(kHexaCenterIndex);
    const int dq[6] = {1, 1, 0, -1, -1, 0}, dr[6] = {0, -1, -1, 0, 1, 1};
    for (int i = 0; i < 6; ++i) {
      auto pxOpt = axial.indexAtAxial(centerAx.q() + dq[i], centerAx.r() + dr[i]);
      if (pxOpt.has_value()) {
        CRGB glow = sunColor;
        glow.nscale8(140);
        buf.leds[pxOpt.value()] = glow;
      }
    }

    // 0 at/below normal speed, 1 at the fastest tilt -- drives the tracer trail
    float speedFactor = constrain((fabs(speedMultiplier) - 1.0f) / (kTopSpeedMultiplier - 1.0f), 0.0f, 1.0f);

    for (int p = 0; p < 8; ++p) {
      float ringRadius = p + 1;
      double period = kBaseOrbitPeriodMS * pow((double)ringRadius, 1.5);
      float angle = fmodPositive(simMs, period) / (float)period * 2*M_PI;
      vectorT<float> pos(ringRadius * cosf(angle), ringRadius * sinf(angle));
      Axial ipos = axial.rectToHex(pos, 1.0).cubeRound();
      auto pxOpt = axial.indexAtAxial(ipos);
      if (pxOpt.has_value()) {
        buf.leds[pxOpt.value()] = kPlanets[p].color;
      }

      if (speedFactor > 0.02f) {
        // fading tracer trailing behind the direction of motion
        int trailDir = (speedMultiplier >= 0) ? -1 : 1;
        int trailCount = 1 + (int)(speedFactor * 3.99f);
        for (int k = 1; k <= trailCount; ++k) {
          float trailAngle = angle + trailDir * k * 0.12f;
          vectorT<float> tp(ringRadius * cosf(trailAngle), ringRadius * sinf(trailAngle));
          Axial tipos = axial.rectToHex(tp, 1.0).cubeRound();
          auto tpxOpt = axial.indexAtAxial(tipos);
          if (tpxOpt.has_value()) {
            CRGB trailColor = kPlanets[p].color;
            trailColor.nscale8((uint8_t)(speedFactor * 180.0f * (1.0f - (float)k / (trailCount+1))));
            buf.leds[tpxOpt.value()] = trailColor;
          }
        }
      }

      if (p == 5) { // a little ring flourish just for Saturn, on the same orbit
        float sideAngles[2] = {angle + 0.35f, angle - 0.35f};
        for (int s = 0; s < 2; ++s) {
          vectorT<float> rp(ringRadius * cosf(sideAngles[s]), ringRadius * sinf(sideAngles[s]));
          Axial ripos = axial.rectToHex(rp, 1.0).cubeRound();
          auto rpxOpt = axial.indexAtAxial(ripos);
          if (rpxOpt.has_value()) {
            CRGB ringColor = kPlanets[p].color;
            ringColor.nscale8(120);
            buf.leds[rpxOpt.value()] = ringColor;
          }
        }
      }
    }

    // speed indicator, drawn last so passing planets never cover it: two
    // yellow dots toward the tilt-right ("speed up") direction, one toward
    // tilt-left ("slow down") -- kept dim so it reads as a subtle hint, not a
    // bright fixture
    CRGB yellow(255, 210, 40);
    yellow.nscale8(70);
    float upX = cosf(kSolarTiltAxisOffset), upY = sinf(kSolarTiltAxisOffset);
    float upRadii[2] = {7.3f, 8.6f};
    for (int i = 0; i < 2; ++i) {
      vectorT<float> dp(upX * upRadii[i], upY * upRadii[i]);
      Axial dipos = axial.rectToHex(dp, 1.0).cubeRound();
      auto dpxOpt = axial.indexAtAxial(dipos);
      if (dpxOpt.has_value()) buf.leds[dpxOpt.value()] = yellow;
    }
    vectorT<float> downDp(-upX * 8.0f, -upY * 8.0f);
    Axial downIpos = axial.rectToHex(downDp, 1.0).cubeRound();
    auto downPxOpt = axial.indexAtAxial(downIpos);
    if (downPxOpt.has_value()) buf.leds[downPxOpt.value()] = yellow;

    // mode indicators, dim single pixels toward each scene's tilt direction:
    // green for horizon (4 o'clock), white for moon (6 o'clock), purple for
    // astrology (8 o'clock). Radius 7.6, not 8.4 -- these three directions are
    // now genuine hex edge-midpoints (60 degrees apart), where the disc's
    // usable radius tops out around 9*cos(30)=~7.79, not the ~9 that vertex
    // directions (like the yellow dots' 0-degree axis) allow. At 8.4 all
    // three were landing outside the valid pixel grid every frame -- that was
    // the "disappeared" bug.
    CRGB white(255, 255, 255); white.nscale8(45);
    CRGB green(60, 220, 90); green.nscale8(70);
    CRGB purple(190, 90, 255); purple.nscale8(70);

    vectorT<float> moonDot(7.6f * cosf(kMoonEdgeAngle), 7.6f * sinf(kMoonEdgeAngle));
    Axial moonDotI = axial.rectToHex(moonDot, 1.0).cubeRound();
    auto moonDotPx = axial.indexAtAxial(moonDotI);
    if (moonDotPx.has_value()) buf.leds[moonDotPx.value()] = white;

    vectorT<float> horizonDot(7.6f * cosf(kHorizonEdgeAngle), 7.6f * sinf(kHorizonEdgeAngle));
    Axial horizonDotI = axial.rectToHex(horizonDot, 1.0).cubeRound();
    auto horizonDotPx = axial.indexAtAxial(horizonDotI);
    if (horizonDotPx.has_value()) buf.leds[horizonDotPx.value()] = green;

    vectorT<float> astroDot(7.6f * cosf(kAstrologyEdgeAngle), 7.6f * sinf(kAstrologyEdgeAngle));
    Axial astroDotI = axial.rectToHex(astroDot, 1.0).cubeRound();
    auto astroDotPx = axial.indexAtAxial(astroDotI);
    if (astroDotPx.has_value()) buf.leds[astroDotPx.value()] = purple;
  }

  // true 3D-sphere illumination test: models the moon as a unit sphere with
  // its near hemisphere facing the viewer (z = sqrt(1-x^2-y^2) toward us) and
  // the sun direction S = (sin(sunAngle), 0, cos(sunAngle)) sweeping smoothly
  // as sunAngle goes 0..2pi over the phase cycle. A surface point is lit when
  // its outward normal -- which for a unit sphere is just the point itself --
  // has positive dot product with S. This is a single continuous formula with
  // no phase-boundary branching, unlike an earlier version of this that cut
  // the disc with a fixed ellipse and picked which side was lit based on
  // which half of the cycle it was in: that produced a real, sometimes
  // multi-pixel discontinuity right at full and new moon (the two halves of
  // the piecewise formula don't actually agree off the boundary pixels), and
  // a hard jump in which edge the sliver crescent appeared on at the new-moon
  // wraparound. Sun angle offset by pi from the raw phase so phase=0 (the
  // cycle's start/wrap point) reads as new moon and phase=0.5 as full.
  static float moonLitAmount(float nx, float ny, float sunAngle) {
    float z = sqrtf(max(0.0f, 1.0f - nx*nx - ny*ny));
    float litRaw = nx*sinf(sunAngle) + z*cosf(sunAngle);
    return constrain(litRaw * 5.0f + 0.5f, 0.0f, 1.0f); // soft few-pixel terminator edge
  }

  void renderMoonScene(PixelStorage<LED_COUNT> &buf, unsigned long nowMs) {
    const float kMoonDisplayRadius = 7.0f; // smaller than the full ~9 disc radius, so the moon reads as a circle floating with a dark margin around it, not filling the whole screen
    const float kMoonDarkBias = 0.09f; // slows the sweep near new moon for a longer dark stretch

    // phase is anchored to when the scene was entered (not raw uptime), and
    // shifted so entry always lands exactly on full moon (0.5) rather than
    // wherever the cycle happens to be; the dark-bias slowdown is centered on
    // u=0.5 too, since that's where new moon now falls
    unsigned long elapsed = nowMs - moonEnteredAt;
    float u = fmodPositive((double)elapsed, kMoonCycleMS) / kMoonCycleMS; // 0 at entry
    float warpedU = u - kMoonDarkBias * sinf(2*M_PI*(u - 0.5f));
    float phase = fmodPositive((double)warpedU + 0.5, 1.0); // 0.5 at entry (full), 0/1 = new
    float sunAngle = phase * 2*M_PI + M_PI;

    // fades in from the center outward over kMoonFadeInMS, on top of (and
    // longer than) the generic scene crossfade, for a subtler edge reveal
    float revealT = constrain((nowMs - moonEnteredAt) / (float)kMoonFadeInMS, 0.0f, 1.0f);

    // project onto the same calibrated right/up axes AnalogClock uses (not
    // raw rect x/y) so the terminator sweeps left-to-right the way the hexa
    // is actually meant to be viewed, not relative to the arbitrary raw pixel grid
    float theta12 = M_PI/2 + kClockRotationOffset;
    float theta3 = theta12 - M_PI/2;
    float rightX = cosf(theta3), rightY = sinf(theta3);
    float upX = cosf(theta12), upY = sinf(theta12);

    // one large, prominent crater (like a real one such as Tycho) whose
    // shadow relief -- dark floor, brighter raised rim -- is strongest near
    // the terminator and fades out in full light or full shadow, the way
    // real craters show dramatic shadow relief mainly at low sun angles
    const float kBigCraterX = 0.32f, kBigCraterY = -0.28f, kBigCraterR = 0.22f;

    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      vectorf rp = axial.rectFromPixelIndex(px);
      float nx = (rp.x*rightX + rp.y*rightY) / kMoonDisplayRadius, ny = (rp.x*upX + rp.y*upY) / kMoonDisplayRadius;
      float r2 = nx*nx + ny*ny;
      if (r2 > 1.0f) {
        buf.leds[px] = CRGB::Black;
        continue;
      }
      float litAmount = moonLitAmount(nx, ny, sunAngle);

      // realistic-ish surface: a few broad dark maria (like the real dark
      // lunar plains) plus scattered small crater dots, both fixed in place
      CRGB lit(200, 200, 205);
      const float mariaX[3] = {-0.35f, 0.25f, -0.1f};
      const float mariaY[3] = {0.3f, 0.15f, -0.45f};
      const float mariaR[3] = {0.35f, 0.28f, 0.22f};
      for (int m = 0; m < 3; ++m) {
        float dx = nx - mariaX[m], dy = ny - mariaY[m];
        float d = sqrtf(dx*dx + dy*dy);
        if (d < mariaR[m]) {
          lit.nscale8((uint8_t)(255.0f - (1.0f - d/mariaR[m]) * 70.0f));
        }
      }
      uint32_t h = (uint32_t)px * 2654435761u;
      if (((h >> 24) % 11) == 0) lit.nscale8(135);

      float bcx = nx - kBigCraterX, bcy = ny - kBigCraterY;
      float bcd = sqrtf(bcx*bcx + bcy*bcy);
      if (bcd < kBigCraterR) {
        float relief = constrain(1.0f - fabsf(litAmount - 0.5f) * 1.6f, 0.0f, 1.0f);
        if (bcd > kBigCraterR * 0.82f) {
          lit = blend(lit, CRGB(235, 232, 225), (uint8_t)(relief * 200)); // bright rim
        } else {
          lit.nscale8((uint8_t)(255.0f - relief * 110.0f)); // dark floor
        }
      }

      CRGB color = blend(CRGB::Black, lit, (uint8_t)(litAmount * 255));

      float radiusFrac = sqrtf(r2);
      float edgeDelay = radiusFrac * 0.6f;
      float localReveal = constrain((revealT - edgeDelay) / max(0.05f, 1.0f - edgeDelay), 0.0f, 1.0f);
      color.nscale8((uint8_t)(localReveal * 255));

      buf.leds[px] = color;
    }
  }

  // small filled disc: a center pixel plus its 6 immediate hex neighbors, core
  // and glow colors separate -- used for the bigger sun in the sunrise scene
  void drawSmallDisc(PixelStorage<LED_COUNT> &buf, Axial center, CRGB coreColor, CRGB glowColor) {
    auto cpx = axial.indexAtAxial(center);
    if (cpx.has_value()) buf.leds[cpx.value()] = coreColor;
    const int dq[6] = {1, 1, 0, -1, -1, 0}, dr[6] = {0, -1, -1, 0, 1, 1};
    for (int i = 0; i < 6; ++i) {
      auto npx = axial.indexAtAxial(center.q() + dq[i], center.r() + dr[i]);
      if (npx.has_value()) buf.leds[npx.value()] = glowColor;
    }
  }

  // same small 7-pixel disc, but each pixel's brightness comes from
  // moonLitAmount() using that pixel's actual direction from the disc's
  // center (matching hexToRect's own q/r -> x/y layout), so the little moon
  // in the sunrise scene shows a rough version of its real current phase
  // instead of a plain dot
  void drawSmallMoonDisc(PixelStorage<LED_COUNT> &buf, Axial center, float sunAngle) {
    CRGB dark(15, 16, 24);
    auto cpx = axial.indexAtAxial(center);
    if (cpx.has_value()) {
      float lit = moonLitAmount(0.0f, 0.0f, sunAngle);
      buf.leds[cpx.value()] = blend(dark, CRGB(220, 220, 225), (uint8_t)(lit * 255));
    }
    const int dq[6] = {1, 1, 0, -1, -1, 0}, dr[6] = {0, -1, -1, 0, 1, 1};
    const float ndx[6] = {1.0f, 0.5f, -0.5f, -1.0f, -0.5f, 0.5f};
    const float ndy[6] = {0.0f, 0.87f, 0.87f, 0.0f, -0.87f, -0.87f};
    for (int i = 0; i < 6; ++i) {
      auto npx = axial.indexAtAxial(center.q() + dq[i], center.r() + dr[i]);
      if (npx.has_value()) {
        float lit = moonLitAmount(ndx[i], ndy[i], sunAngle);
        buf.leds[npx.value()] = blend(dark, CRGB(200, 200, 210), (uint8_t)(lit * 255));
      }
    }
  }

  void renderSunriseScene(PixelStorage<LED_COUNT> &buf, unsigned long nowMs) {
    // ground direction is the same "4 o'clock" edge used for scene selection,
    // so the ground/sky split and the arc both stay self-consistent
    const float kHorizonHeight = 3.0f; // pushes the ground down, but shows more of it than before
    const float kHorizonCurvature = 0.03f; // subtle limb curve at the strip's edges
    const float kArcRadius = 7.0f;

    float groundDirX = cosf(kHorizonEdgeAngle), groundDirY = sinf(kHorizonEdgeAngle);
    float perpX = cosf(kHorizonEdgeAngle + M_PI/2), perpY = sinf(kHorizonEdgeAngle + M_PI/2);

    // the sun sweeps a FULL circle over kDayCycleMS -- only the half above
    // ground is ever drawn, but its angle keeps advancing even while below
    // the horizon ("off screen"), so the ground lighting below can still
    // track where it actually is
    float sunT = fmodPositive((double)nowMs, (double)kDayCycleMS) / kDayCycleMS;
    bool sunVisible = sunT < 0.5f;
    float sunAngle = kHorizonEdgeAngle + M_PI/2 + sunT * 2*M_PI;

    // the moon's sky position is tied to its actual phase rather than a fixed
    // half-cycle offset from the sun -- same relationship the real moon has:
    // near the sun (and so barely visible, near the horizon together) at new
    // moon, opposite the sun (rising as it sets) at full moon, roughly a
    // quarter-circle off at the quarters. Ties the dedicated moon scene's
    // phase cycle to this scene's sky position for a single consistent story.
    float moonPhaseU = fmodPositive((double)nowMs, kMoonCycleMS) / kMoonCycleMS;
    float moonAngle = sunAngle + moonPhaseU * 2*M_PI;
    float moonX = kArcRadius * cosf(moonAngle), moonY = kArcRadius * sinf(moonAngle);
    bool moonVisible = (moonX*groundDirX + moonY*groundDirY) < 0.0f; // true horizon plane, not the foreground terrain threshold

    CRGB nightLand(14, 34, 18), dayLand(60, 190, 90);
    CRGB nightOcean(10, 22, 46), dayOcean(35, 110, 195);

    buf.leds.fill_solid(CRGB::Black); // black sky

    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      vectorf rp = axial.rectFromPixelIndex(px);
      float alongGround = rp.x * groundDirX + rp.y * groundDirY;
      float lateral = rp.x * perpX + rp.y * perpY;
      float curvedThreshold = kHorizonHeight + lateral*lateral * kHorizonCurvature;
      if (alongGround > curvedThreshold) {
        // dynamic gradient tracking the sun's actual angle (even while it's
        // below the horizon/off screen): brightest on the side of the earth
        // nearest the sun's current direction, dimmer (not black) on the far
        // side -- a real day/night terminator sweeping across the strip
        // rather than a single flat brightness for the whole thing
        float pixelAngle = atan2f(rp.y, rp.x);
        uint8_t closeness = (uint8_t)(((cosf(sunAngle - pixelAngle) + 1.0f) / 2.0f) * 255);
        CRGB land = blend(nightLand, dayLand, closeness);
        CRGB ocean = blend(nightOcean, dayOcean, closeness);
        // fixed continent shape -- does not scroll/rotate; only its lighting does
        uint8_t terrainPhase = (uint8_t)(int)(rp.x*14 + rp.y*5);
        bool isLand = sin8(terrainPhase) > 140;
        buf.leds[px] = isLand ? land : ocean;
      }
    }

    // 5 twinkling stars orbiting a fixed pole point partway toward zenith, all
    // rotating together in a slow "sidereal" sweep. The pole offset and orbit
    // radii are large enough that each star's distance from the true center
    // ranges from well inside the disc to well past its radius-9 edge, so
    // they genuinely sweep off screen and back on as they go around, rather
    // than staying confined to a small always-visible patch near the middle.
    const int kStarCount = 5;
    float poleX = -groundDirX * kArcRadius*0.6f, poleY = -groundDirY * kArcRadius*0.6f;
    float starSpin = fmodPositive((double)nowMs, (double)kStarRotationPeriodMS) / kStarRotationPeriodMS * 2*M_PI;
    for (int s = 0; s < kStarCount; ++s) {
      float starRadius = 5.5f + (s % 3) * 1.5f;
      float starAngle = starSpin + s * (2*M_PI/kStarCount);
      float sx = poleX + starRadius*cosf(starAngle), sy = poleY + starRadius*sinf(starAngle);
      if (sx*groundDirX + sy*groundDirY < kHorizonHeight) {
        Axial si = axial.rectToHex(vectorT<float>(sx, sy), 1.0).cubeRound();
        auto spx = axial.indexAtAxial(si);
        if (spx.has_value()) {
          uint8_t twinklePhase = (uint8_t)(s * 53 + 17);
          uint8_t twinkle = sin8((uint8_t)(nowMs/35 + twinklePhase));
          CRGB starColor(200, 210, 255);
          starColor.nscale8(70 + scale8(twinkle, 160));
          buf.leds[spx.value()] = starColor;
        }
      }
    }

    // random meteor: a short bright streak with a fading tail, crossing the
    // sky when meteorActive (rolled/timed in update())
    if (meteorActive) {
      float progress = (nowMs - meteorStartMs) / (float)kMeteorDurationMs;
      const int kTailSteps = 5;
      for (int t = 0; t < kTailSteps; ++t) {
        float tp = progress - t * 0.035f;
        if (tp < 0.0f || tp > 1.0f) continue;
        float mx = meteorOriginX + meteorDirX * kMeteorTravelDistance * tp;
        float my = meteorOriginY + meteorDirY * kMeteorTravelDistance * tp;
        if (mx*groundDirX + my*groundDirY < kHorizonHeight) {
          Axial mi = axial.rectToHex(vectorT<float>(mx, my), 1.0).cubeRound();
          auto mpx = axial.indexAtAxial(mi);
          if (mpx.has_value()) {
            CRGB c(255, 255, 240);
            c.nscale8((uint8_t)(255.0f * (1.0f - (float)t / kTailSteps)));
            buf.leds[mpx.value()] = c;
          }
        }
      }
    }

    if (sunVisible) {
      Axial sunI = axial.rectToHex(vectorT<float>(kArcRadius*cosf(sunAngle), kArcRadius*sinf(sunAngle)), 1.0).cubeRound();
      drawSmallDisc(buf, sunI, CRGB(255, 220, 100), CRGB(255, 170, 40));
    }
    if (moonVisible) {
      float moonSunAngle = moonPhaseU * 2*M_PI + M_PI; // same convention as the dedicated moon scene
      Axial moonI = axial.rectToHex(vectorT<float>(moonX, moonY), 1.0).cubeRound();
      drawSmallMoonDisc(buf, moonI, moonSunAngle);
    }
  }

  // reached by flipping the hexa onto its other flat face. A ring of 12
  // sparkling markers (one per zodiac sign) slowly rotates as a whole over a
  // pulsing purple glow; one marker is brighter than the rest at a time,
  // itself slowly cycling around the ring like a calendar moving through the
  // signs, and a fast bright white glint sweeps around separately from the
  // markers for a reflective, shiny feel.
  void renderAstrologyScene(PixelStorage<LED_COUNT> &buf, unsigned long nowMs) {
    const int kSignCount = 12;
    const float kWheelRadius = 7.5f;
    const unsigned long kWheelRotationPeriodMS = 60000;
    const unsigned long kHighlightCycleMS = 36000;
    const unsigned long kShinePeriodMS = 5000;

    uint8_t pulse = sin8((uint8_t)(nowMs / 25));
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      Axial hexPos = axial.axialFromPixelIndex(px);
      int dist = max(max(abs(hexPos.q()), abs(hexPos.r())), abs(hexPos.s()));
      float falloff = max(0.0f, 1.0f - dist / 9.0f);
      uint8_t glow = (uint8_t)(falloff * falloff * scale8(pulse, 60));
      CRGB color(60, 15, 90);
      color.nscale8(20 + glow);
      buf.leds[px] = color;
    }

    float wheelRotation = fmodPositive((double)nowMs, (double)kWheelRotationPeriodMS) / kWheelRotationPeriodMS * 2*M_PI;
    int highlightIndex = (int)(fmodPositive((double)nowMs, (double)kHighlightCycleMS) / kHighlightCycleMS * kSignCount);

    for (int i = 0; i < kSignCount; ++i) {
      float angle = wheelRotation + i * (2*M_PI / kSignCount);
      vectorT<float> pos(kWheelRadius * cosf(angle), kWheelRadius * sinf(angle));
      Axial ipos = axial.rectToHex(pos, 1.0).cubeRound();
      auto pxOpt = axial.indexAtAxial(ipos);
      if (pxOpt.has_value()) {
        uint8_t sparklePhase = (uint8_t)(i * 23 + 7);
        uint8_t sparkle = sin8((uint8_t)(nowMs / 20 + sparklePhase));
        bool isHighlighted = (i == highlightIndex);
        CRGB signColor = isHighlighted ? CRGB(230, 160, 255) : CRGB(170, 80, 220);
        signColor.nscale8(isHighlighted ? (200 + scale8(sparkle, 55)) : (110 + scale8(sparkle, 120)));
        buf.leds[pxOpt.value()] = signColor;
      }
    }

    float shineAngle = fmodPositive((double)nowMs, (double)kShinePeriodMS) / kShinePeriodMS * 2*M_PI;
    vectorT<float> shinePos(kWheelRadius * cosf(shineAngle), kWheelRadius * sinf(shineAngle));
    Axial shineI = axial.rectToHex(shinePos, 1.0).cubeRound();
    auto shinePx = axial.indexAtAxial(shineI);
    if (shinePx.has_value()) buf.leds[shinePx.value()] = CRGB::White;

    // fading colorful slideshow at the center, standing in for the current
    // sign: a small filled disc (center + 6 neighbors) that crossfades from
    // one vivid color to the next every few seconds, cycling through all 12
    const CRGB kSignColors[12] = {
      CRGB(255, 80, 80), CRGB(210, 150, 70), CRGB(255, 220, 80), CRGB(150, 200, 255),
      CRGB(255, 140, 40), CRGB(140, 200, 140), CRGB(255, 150, 200), CRGB(210, 40, 70),
      CRGB(160, 80, 220), CRGB(120, 130, 170), CRGB(80, 220, 220), CRGB(120, 150, 255),
    };
    const unsigned long kSlideshowPeriodMS = 4000;
    unsigned long slideElapsed = nowMs % kSlideshowPeriodMS;
    int signIndex = (int)((nowMs / kSlideshowPeriodMS) % 12);
    int nextSignIndex = (signIndex + 1) % 12;
    uint8_t slideFade = ease8InOutQuad((uint8_t)(0xFF * slideElapsed / kSlideshowPeriodMS));
    CRGB slideColor = blend(kSignColors[signIndex], kSignColors[nextSignIndex], slideFade);

    buf.leds[kHexaCenterIndex] = slideColor;
    Axial centerAx = axial.axialFromPixelIndex(kHexaCenterIndex);
    const int dq[6] = {1, 1, 0, -1, -1, 0}, dr[6] = {0, -1, -1, 0, 1, 1};
    for (int i = 0; i < 6; ++i) {
      auto npx = axial.indexAtAxial(centerAx.q() + dq[i], centerAx.r() + dr[i]);
      if (npx.has_value()) {
        CRGB glow = slideColor;
        glow.nscale8(150);
        buf.leds[npx.value()] = glow;
      }
    }
  }

  // Reached by resting the hexa face down. Tilt pans a window into a much
  // larger virtual sky -- the same "offset the rendering origin by smoothed
  // tilt" technique PulseHexaSmooth uses to shift its pulse's center around,
  // just applied to sample position in a big procedural scene instead of
  // shifting a single point. The moon here shares the exact same phase clock
  // (kMoonCycleMS) and illumination model as the dedicated moon scene and the
  // sunrise scene's little moon, so all three agree on what phase it's in.
  void renderSkyScene(PixelStorage<LED_COUNT> &buf, unsigned long nowMs) {
    const unsigned long kDayNightCycleMS = 32000; // 40000 sped up 20% further from the original 50000
    const float kPanScale = 0.006f; // how far the view pans per unit of smoothed tilt
    const float kMoonVX = 22.0f, kMoonVY = 14.0f; // moon's fixed spot in the larger virtual sky
    const float kSunVX = -18.0f, kSunVY = 20.0f;  // sun's fixed spot in the larger virtual sky
    const float kMoonRadius = 2.3f;
    const float kSunRadius = 2.2f;
    // a fixed waxing-crescent look (not tied to the live moon-phase cycle) --
    // this scene is decorative, so a consistently recognizable crescent reads
    // better than occasionally landing on a boring, near-invisible new moon
    const float kSkyMoonSunAngle = 4.084f;

    // relative to when the scene was entered, not raw uptime, so it always
    // starts fresh at night rather than wherever global millis() happens to land
    unsigned long elapsed = nowMs - skyEnteredAt;
    uint8_t dayAmount = sin8((uint8_t)(fmodPositive((double)elapsed, (double)kDayNightCycleMS) / kDayNightCycleMS * 255.0));

    float viewX = smoothAcc.x * kPanScale, viewY = smoothAcc.y * kPanScale;

    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      vectorf rp = axial.rectFromPixelIndex(px);
      float vx = rp.x + viewX, vy = rp.y + viewY; // this pixel's position in the larger virtual sky

      // --- night: gradient background plus the moon (stars are drawn in a
      // separate pass below as stable points, not sampled per-pixel here)
      CRGB nightPixel = blend(CRGB(3, 3, 12), CRGB(9, 9, 24), (uint8_t)constrain((vy + 20.0f) * 4.0f, 0.0f, 255.0f));
      float mdx = vx - kMoonVX, mdy = vy - kMoonVY;
      if (mdx*mdx + mdy*mdy <= kMoonRadius*kMoonRadius) {
        float lit = moonLitAmount(mdx / kMoonRadius, mdy / kMoonRadius, kSkyMoonSunAngle);
        nightPixel = blend(CRGB(15, 16, 24), CRGB(215, 215, 225), (uint8_t)(lit * 255));
      }

      // --- day: gradient pale blue sky, sparser wispy (soft-edged, not flat
      // blob) clouds, a bigger sun with a gradient core and a simple face
      CRGB skyBase = blend(CRGB(105, 165, 220), CRGB(175, 215, 245), (uint8_t)constrain((vy + 20.0f) * 4.0f, 0.0f, 255.0f));
      uint8_t w1 = sin8((uint8_t)(int)(vx*9 + vy*4));
      uint8_t w2 = sin8((uint8_t)(int)(-vx*5 + vy*11 + 50));
      uint8_t cloudNoise = (uint8_t)(((uint16_t)w1 + w2) / 2);
      uint8_t cloudDensity = (cloudNoise > 200) ? (uint8_t)min(255, (cloudNoise - 200) * 5) : 0; // sparser + soft wispy edge
      CRGB dayPixel = blend(skyBase, CRGB(250, 250, 255), cloudDensity);

      float sdx = vx - kSunVX, sdy = vy - kSunVY;
      float sunDist2 = sdx*sdx + sdy*sdy;
      if (sunDist2 <= kSunRadius*kSunRadius) {
        float snx = sdx / kSunRadius, sny = sdy / kSunRadius; // -1..1 within the sun's own disc
        bool isEye = fabsf(fabsf(snx) - 0.4f) < 0.18f && sny > 0.15f && sny < 0.55f;
        bool isMouth = sny < -0.15f && sny > -0.55f && fabsf(snx) < 0.5f && fabsf(sny - (-0.45f + 0.5f*snx*snx)) < 0.15f;
        if (isEye || isMouth) {
          dayPixel = CRGB(190, 90, 30); // warm, darker feature color for the face
        } else {
          float distFrac = sqrtf(sunDist2) / kSunRadius;
          dayPixel = blend(CRGB(255, 250, 200), CRGB(255, 170, 60), (uint8_t)(distFrac * 255)); // bright core to warm edge
        }
      }

      buf.leds[px] = blend(nightPixel, dayPixel, dayAmount);
    }

    // stars: a fixed list of points in the virtual sky (not re-sampled per
    // pixel from a hash grid), so panning moves them smoothly instead of
    // flickering at quantization boundaries. Each one twinkles gently --
    // fading in and out slightly, not a hard on/off -- on its own independent
    // phase, and every 7th is drawn a little bigger.
    if (dayAmount < 220) {
      const int kStarCount = 40;
      uint8_t nightWeight = 255 - dayAmount;
      const int dq[6] = {1, 1, 0, -1, -1, 0}, dr[6] = {0, -1, -1, 0, 1, 1};
      for (int i = 0; i < kStarCount; ++i) {
        uint32_t sh = (uint32_t)i * 2654435761u;
        sh = (sh ^ (sh >> 15)) * 0x85ebca6bu;
        sh ^= sh >> 13;
        float starVX = (float)((int)(sh % 8000) - 4000) / 100.0f; // spread across -40..40
        float starVY = (float)((int)((sh / 8000) % 8000) - 4000) / 100.0f;

        float screenX = starVX - viewX, screenY = starVY - viewY;
        Axial si = axial.rectToHex(vectorT<float>(screenX, screenY), 1.0).cubeRound();
        auto spx = axial.indexAtAxial(si);
        if (!spx.has_value()) continue;

        uint8_t twinklePhase = (uint8_t)sh;
        uint8_t twinkle = sin8((uint8_t)(nowMs/60 + twinklePhase)); // slow, gentle
        uint8_t brightness = scale8(140 + scale8(twinkle, 90), nightWeight); // fades in/out slightly, not a hard swing
        CRGB starColor(200, 210, 255);
        starColor.nscale8(brightness);
        buf.leds[spx.value()] = starColor;

        if (i % 7 == 0) { // some stars big
          auto npx = axial.indexAtAxial(si.q() + dq[0], si.r() + dr[0]);
          if (npx.has_value()) {
            CRGB dim = starColor;
            dim.nscale8(140);
            buf.leds[npx.value()] = dim;
          }
        }
      }
    }
  }

  void renderScene(int scene, PixelStorage<LED_COUNT> &buf, double simMs, unsigned long nowMs, float speedMultiplier) {
    switch (scene) {
      case kMoonScene:      renderMoonScene(buf, nowMs); break;
      case kSunriseScene:   renderSunriseScene(buf, nowMs); break;
      case kAstrologyScene: renderAstrologyScene(buf, nowMs); break;
      case kSkyScene:       renderSkyScene(buf, nowMs); break;
      default:               renderSolarScene(buf, simMs, speedMultiplier); break;
    }
  }

  void update() {
    ICM_20948_AGMT_t agmt = MotionManager::motionFrame.agmt;
    vector32 acc(agmt.acc.axes.x, agmt.acc.axes.y, agmt.acc.axes.z);
    smoothAcc = (10 * smoothAcc + acc) / 11;

    float ax = smoothAcc.x, ay = smoothAcc.y, az = smoothAcc.z;
    float mag = sqrt(ax*ax + ay*ay + az*az);
    float flatness = (mag > 1) ? fabs(az) / mag : 1.0f;

    // lying flat happens on either of the hexa's two big faces: az>0 (face up)
    // is the solar system, az<0 (face down) is the star/sky viewing scene.
    // The other three scenes are reached by tilting off-flat toward one of
    // three reference directions (4/6/8 o'clock, genuine alternating hex
    // edges) and picking whichever is closest.
    bool wasFlat = (currentScene == kSolarScene || currentScene == kSkyScene);
    bool nowFlat = wasFlat ? (flatness >= kFlatExitThreshold) : (flatness >= kFlatEnterThreshold);

    unsigned long nowMs = millis();
    int newScene;
    if (nowFlat) {
      newScene = (az >= 0) ? kSolarScene : kSkyScene;
    } else {
      float dotHorizon = (mag > 1) ? (ax*cosf(kHorizonEdgeAngle) + ay*sinf(kHorizonEdgeAngle)) / mag : 0;
      float dotMoon = (mag > 1) ? (ax*cosf(kMoonEdgeAngle) + ay*sinf(kMoonEdgeAngle)) / mag : 0;
      float dotAstrology = (mag > 1) ? (ax*cosf(kAstrologyEdgeAngle) + ay*sinf(kAstrologyEdgeAngle)) / mag : 0;

      float vals[3] = {dotHorizon, dotMoon, dotAstrology};
      int scenes[3] = {kSunriseScene, kMoonScene, kAstrologyScene};
      int bestIdx = 0;
      for (int i = 1; i < 3; ++i) if (vals[i] > vals[bestIdx]) bestIdx = i;
      float secondVal = -1e9f;
      for (int i = 0; i < 3; ++i) if (i != bestIdx && vals[i] > secondVal) secondVal = vals[i];

      if (vals[bestIdx] - secondVal < kEdgeMarginDeadzone) {
        newScene = wasFlat ? kMoonScene : currentScene; // ambiguous: keep whichever standing scene we're already in
      } else {
        newScene = scenes[bestIdx];
      }
    }
    if (newScene != currentScene) {
      previousScene = currentScene;
      currentScene = newScene;
      sceneTransitionStart = nowMs;
      if (newScene == kMoonScene) moonEnteredAt = nowMs;
      if (newScene == kSkyScene) skyEnteredAt = nowMs;
    }

    // random meteor for the sunrise scene: rolled once per check interval
    // while none is active
    if (!meteorActive) {
      if (nowMs - lastMeteorCheckMs > kMeteorCheckIntervalMs) {
        lastMeteorCheckMs = nowMs;
        if (random8(100) < kMeteorChancePercent) {
          meteorActive = true;
          meteorStartMs = nowMs;
          float startAngle = (random8() / 255.0f) * 2*M_PI;
          meteorOriginX = 9.0f * cosf(startAngle);
          meteorOriginY = 9.0f * sinf(startAngle);
          float travelAngle = startAngle + M_PI + ((int)random8() - 128) / 128.0f * 0.8f;
          meteorDirX = cosf(travelAngle);
          meteorDirY = sinf(travelAngle);
        }
      }
    } else if (nowMs - meteorStartMs > kMeteorDurationMs) {
      meteorActive = false;
    }

    // solar system speed: dead zone then a squared ease toward the top
    // multiplier, same shape as AnalogClock's tilt speed but more responsive
    float tiltRight = (mag > 1) ? constrain((ax*cosf(kSolarTiltAxisOffset) + ay*sinf(kSolarTiltAxisOffset)) / mag, -1.0f, 1.0f) : 0;
    float absTiltRight = fabs(tiltRight);
    float speedMultiplier;
    if (absTiltRight <= kSpeedDeadzone) {
      speedMultiplier = 1.0f;
    } else {
      float t = (absTiltRight - kSpeedDeadzone) / (1.0f - kSpeedDeadzone);
      float eased = t*t;
      float speedMag = 1.0f + eased * (kTopSpeedMultiplier - 1.0f);
      speedMultiplier = (tiltRight < 0) ? -speedMag : speedMag;
    }
    solarSimulatedMillis += frameTime() * speedMultiplier;

    renderScene(currentScene, sceneBufferA, solarSimulatedMillis, nowMs, speedMultiplier);

    unsigned long transitionElapsed = nowMs - sceneTransitionStart;
    if (previousScene >= 0 && transitionElapsed < kSceneTransitionMS) {
      renderScene(previousScene, sceneBufferB, solarSimulatedMillis, nowMs, speedMultiplier);
      uint8_t alpha = ease8InOutQuad((uint8_t)(0xFF * transitionElapsed / kSceneTransitionMS));
      for (PixelIndex px = 0; px < LED_COUNT; ++px) {
        ctx.leds[px] = blend(sceneBufferB.leds[px], sceneBufferA.leds[px], alpha);
      }
    } else {
      for (PixelIndex px = 0; px < LED_COUNT; ++px) {
        ctx.leds[px] = sceneBufferA.leds[px];
      }
    }
  }

  const char *description() {
    return "SolarSystem";
  }
};

/* ------------------------------------------------------------------------------- */

// Radar sweep
class LineSweep : public Pattern, PaletteRotation<CRGBPalette256> {
public:
  HexaShells hexaShells;
  int maxShellSize = 0;
  LineSweep() {
    maxColorJump = 7;
    secondsPerPalette = 7;
    minBrightness = 10;
    for (auto shell : hexaShells.shells) {
      if (shell.size() > maxShellSize) {
        maxShellSize = shell.size();
      }
    }
  }

  void update() {
    ctx.leds.fadeToBlackBy(18);
    for (int s = 0 ; s < hexaShells.shells.size(); ++s) {
      uint8_t shellSize = hexaShells.shells[s].size();
      
      for (int l = 0; l < 2; ++l) {
        unsigned long index = millis()/30;
        int si = ((shellSize * (index + l)) / maxShellSize)%shellSize;
        CRGB c = getMirroredPaletteColor(millis()/20, (l == 0 ? 0xFF : 0x7F));
        ctx.leds[hexaShells.shells[s][si].value()] = c;
      }
    }
  }

  const char *description() {
    return "LineSweep";
  }
};

/* ------------------------------------------------------------------------------- */

// broken version of LineSweep that Sequoia thought was neat
class LineSweepOops : public Pattern, PaletteRotation<CRGBPalette256> {
public:
  HexaShells hexaShells;
  LineSweepOops() {
    maxColorJump = 7;
    secondsPerPalette = 15;
  }

  void update() {
    ctx.leds.fadeToBlackBy(5);
    int shellCount = hexaShells.shells.size();
    for (int s = 0 ; s < hexaShells.shells.size(); ++s) {
      uint8_t shellSize = hexaShells.shells[s].size();
      
      for (int l = 0; l < 3; ++l) {
        unsigned long index = millis()/100;
        int si = (shellSize * index / shellSize)%shellSize;
        if (s == 0) {
          logf("si = %i", si);
        }
        CRGB c = CRGB::Red;
        ctx.leds[hexaShells.shells[s][si].value()] = c;
      }
    }
  }
  const char *description() {
    return "LineSweepOops";
  }
};

/* ------------------------------------------------------------------------------- */

class BouncyPixels : public Pattern, PaletteRotation<CRGBPalette256> {
public:
  const PixelIndex pixelCount;
  PixelPhysics<LED_COUNT> physics;
  int fadeDown = 0xFF;
  BouncyPixels(PixelIndex pixelCount, uint8_t accelScaling, uint8_t elasticity, uint8_t elasticityMultiplier=1) : physics(hexGrid, pixelCount, accelScaling, elasticity, elasticityMultiplier), pixelCount(pixelCount) {
    minBrightness = 15;
  }

  virtual void update() {
    ctx.leds.fadeToBlackBy(fadeDown);
    physics.update([](PixelIndex index) {
      return accelerationAtPixelIndex(index, MotionManager::motionFrame.agmt);
    });
    int i = 0;
    for (PixelPhysics<LED_COUNT>::Particle *p : physics.particles) {
      CRGB color = getShiftingPaletteColor(0xFF * i++ / physics.particles.size());
      ctx.leds[p->index] = color;
    }
  }

  virtual const char *description() {
    return "BouncyPixels";
  }
};

class TriBounce : public BouncyPixels {
public:
  TriBounce() : BouncyPixels(3, 70, 0xFF, 2) {
  }
  void update() {
    BouncyPixels::update();
    int i = 0;
    for (PixelPhysics<LED_COUNT>::Particle *p : physics.particles) {
      CRGB color = CHSV(i++ * 0xFF/pixelCount, 0xFF, 0xFF);
      ctx.leds[p->index] = color;
    }
  }
  const char *description() {
    return "TriBounce";
  }
};

class PixelDust : public BouncyPixels {
public:
  PixelDust() : BouncyPixels(60, 70, 0xF4) {
  }
  const char *description() {
    return "PixelDust";
  }
};

class PixelSand : public BouncyPixels {
public:
  PixelSand() : BouncyPixels(60, 70, 0xC0) {
  }
  const char *description() {
    return "PixelSand";
  }
};

class RandomDust : public BouncyPixels {
public:
  RandomDust() : BouncyPixels(random8(100)+1, random8(20), random8(255)) {
    logf("RandomDust chose pixelCount=%i, accelScaling=%i, elasticity=%i", physics.particles.size(), physics.accelScaling, physics.elasticity);
  }
  const char *description() {
    return "RandomDust";
  }
};

// special case the single ball physics since we can do nice floating point math for a single particle
class LargeBouncyBall : public Pattern {
  struct Ball {
    vectorf pos;
    vectorf velocity;
    Ball() : pos(0,0), velocity(0,0) {};
    Ball(vectorf pos, vectorf velocity) : pos(pos), velocity(velocity) {};
  };
public:
  Ball p;
  unsigned long boomStart = 0;

  void stellate(float radius, float bright) {
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      Axial ax = axial.axialFromPixelIndex(px);
      int aq = abs(ax.q()), ar = abs(ax.r()), as = abs(ax.s());
      float stellatedDist = (max(max(aq, ar), as) + min(min(aq, ar), as) * 2) / 2;
      if (stellatedDist <= radius) {
        uint8_t b = bright * (1.0f - stellatedDist / max(radius, 0.01f)) * 255;
        ctx.leds[px] = CRGB(b, b, b);
      }
    }
  }

  void sideHit(Ball &p, int w, uint8_t hue, unsigned long elapsed) {
    assert(hexaSide(w).size() == 10,"hexa side size");
    uint8_t hitSpeed = constrain(2000 * p.velocity.length()*elapsed - 100, 0, 0xFF);
    for (PixelIndex px : hexaSide(w)) {
      ctx.leds[px] = CHSV(hue+0xFF/2, 0xFF, hitSpeed);
    }
  }

  uint8_t sideCollision(Ball &p, unsigned long elapsed) {
    static const float sideR = kMeridian/2.f - 2;
    const linef urLine(kSqrtThree,   1, -sideR*kSqrtThree);
    const linef uLine (0,            1, -sideR*kSqrtThree/2);
    const linef ulLine(-kSqrtThree,  1, -sideR*kSqrtThree);
    const linef dlLine(-kSqrtThree, -1, -sideR*kSqrtThree);
    const linef dLine (0,           -1, -sideR*kSqrtThree/2);
    const linef drLine(kSqrtThree,  -1, -sideR*kSqrtThree);

    // u,ur,dr,d,dl,ul order, matches clockwise from px 0 hexaSide order
    const linef lines[] = {uLine, urLine, drLine, dLine, dlLine, ulLine};
    const float elasticity = 0.95f + constrain(p.velocity.length()*elapsed/6 - 0.019, 0, 0.09f);

    uint8_t sidesHit = 0;
    // Iterate to handle corner collision
    for (int it = 0; it < 3; ++it) {
      bool collided = false;
      for (int w = 0; w < 6; ++w) {
        const auto &wallLine = lines[w];
        float dist = wallLine.A * p.pos.x + wallLine.B * p.pos.y + wallLine.C;
        if (dist > 0) {
          float nLenSq = wallLine.A * wallLine.A + wallLine.B * wallLine.B;

          // Reflect position back inside (mirror across wall)
          p.pos.x -= 2 * wallLine.A * dist / nLenSq;
          p.pos.y -= 2 * wallLine.B * dist / nLenSq;

          // Reflect velocity only if moving outward
          float vDotN = p.velocity.x * wallLine.A + p.velocity.y * wallLine.B;
          if (vDotN > 0) {
            p.velocity.x -= 2 * wallLine.A * vDotN / nLenSq * elasticity;
            p.velocity.y -= 2 * wallLine.B * vDotN / nLenSq * elasticity;
          }
          collided = true;
          sidesHit |= 1 << w;
          break; // re-check all
        }
      }
      if (!collided) {
        break;
      }
    }
    return sidesHit;
  }

  unsigned long lastUpdate = 0;
  virtual void update() {
    ctx.leds.fadeToBlackBy(12);

    int32_t elapsed = (lastUpdate > 0 ? millis() - lastUpdate : 1);
    lastUpdate = millis();

    if (boomStart != 0) {
      unsigned long boomRuntime = millis() - boomStart;
      auto p = Phaser()
        .anim(250, [this](Phase ph) {
          ctx.leds.fill_solid(CRGB::Black);
        })
        .anim(300, [this](Phase ph) {
          float p = ph.progress();
          float expand = (1.0f - p) * (1.0f - p);
          stellate(24.0f * expand, 1.0f);
        })
        .complete([this](Phase) {
          boomStart = 0;
        });
      p.run(min(boomRuntime, p.duration()));

      if (boomRuntime < p.duration()) {
        return;
      };
    }

    // update ball position from velocity and elapsed time
    std::optional<PixelIndex> pxopt = axial.indexAtRect(p.pos);
    if (!pxopt.has_value()) {
      logf("You win! Ball at pos (%f, %f) is out of bounds!", p.pos.x, p.pos.y);
      ctx.leds.fill_solid(CRGB::Black);
      boomStart = millis();

      p.pos.x = 0;
      p.pos.y = 0;
      p.velocity.x = 0;
      p.velocity.y = 0;
      return;
    }
    
    PixelIndex px = pxopt.value();
    auto agmt = MotionManager::motionFrame.agmt;
    vectorf accelVector = accelerationAtPixelIndex(px, agmt);

    const float accelPreScale = 3600; // tuned
    vectorf scaledAccel = accelVector * elapsed / accelPreScale / MotionManager::accelToGScale;
    p.velocity.x += scaledAccel.x;
    p.velocity.y += scaledAccel.y;

    // Coriolis: deflects ball path during rotation
    const float coriolisScale = 1.0f;
    const float coriolisK = coriolisScale / (MotionManager::gyrToRadScale * 1000.0f);
    float coriolisF = agmt.gyr.axes.z * elapsed * coriolisK;
    p.velocity.x +=  coriolisF * p.velocity.y;
    p.velocity.y += -coriolisF * p.velocity.x;

    // Friction: linear approximation of exp(-k*dt)
    const float frictionCoeff = 0.9f;
    const float frictionK = frictionCoeff / 1000.0f;
    float dampFactor = max(0.0f, 1.0f - elapsed * frictionK);
    p.velocity.x *= dampFactor;
    p.velocity.y *= dampFactor;

    p.pos += p.velocity * elapsed;
    uint8_t sidesHit = sideCollision(p, elapsed);

    uint8_t hue = constrain(3000 * p.velocity.length() - 30, 0, 224);
    
    // 16-mult integer optimizations
    int16_t bx16 = (int16_t)(p.pos.x * 16);
    int16_t by16 = (int16_t)(p.pos.y * 16);
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      vectorf r = axial.rectFromPixelIndex(px);
      // Hex-shaped integer distance, pulling out the sqrt(3) ~= 111/64
      uint16_t adx = abs(bx16 - (int16_t)(r.x * 16));
      uint16_t ady = abs(by16 - (int16_t)(r.y * 16));
      uint32_t hexD = max(ady * 2, (adx * 111 >> 6) + ady);
      uint16_t size = 50; // diameter 2*sqrt(3)*16 ~= 55
      if (hexD >= size) { 
        continue;
      }
      uint8_t brightness = 255 - (uint8_t)(hexD * 255/size);
      int hueShift = hexD * size >> 8;

      CRGB c = CHSV(max(0, hue - hueShift), 0xFF, 0xFF);
      c = c.scale8(brightness);
      ctx.point(px, c, blendBrighten);
    }
    for (int i = 0; i < 6; ++i) {
      if (sidesHit & (1 << i)) {
        sideHit(p, i, hue, elapsed);
      }
    }
  }

  virtual const char *description() {
    return "LargeBouncyBall";
  }
};

/* ------------------------------------------------------------------------------- */

class TriangleSpin : public Pattern, PaletteRotation<CRGBPalette256> {
public:
  TriangleSpin() {
    secondsPerPalette = 20;
  };

  // Rotate vector v by quaternion q: v' = v + 2w*(q×v) + 2*(q×(q×v))
  vectorf quatRotate(const Quaternion &q, vectorf v) {
    // t = 2 * cross(q.xyz, v)
    float tx = 2.0f * (q.y * v.z - q.z * v.y);
    float ty = 2.0f * (q.z * v.x - q.x * v.z);
    float tz = 2.0f * (q.x * v.y - q.y * v.x);
    // v' = v + w*t + cross(q.xyz, t)
    return vectorf(
      v.x + q.w * tx + (q.y * tz - q.z * ty),
      v.y + q.w * ty + (q.z * tx - q.x * tz),
      v.z + q.w * tz + (q.x * ty - q.y * tx)
    );
  }

  void update() {
    ctx.leds.fill_solid(CRGB::Black);

    // we're actually counterrotating a tetrahedron mmkay
    Quaternion q = MotionManager::motionFrame.quat;
    q.z = -q.z;

    float r = kMeridian/2-1;
    unsigned timeOffset = millis() / 50;

    // Regular tetrahedron with vertex pointing 'up' when flat
    constexpr float sq2_3 = 0.9428090f;  // 2*sqrt(2)/3
    constexpr float sq6_3 = 0.8164966f;  // sqrt(6)/3
    constexpr float third = 1.0f / 3.0f;
    const vectorf baseVerts[4] = {
      {0,          0,      -1},      // v0: apex
      {sq2_3,      0,       third},  // v1: base front
      {-sq2_3/2,   sq6_3,   third},  // v2: base left
      {-sq2_3/2,  -sq6_3,   third},  // v3: base right
    };

    // Rotate and scale vertices
    vectorf verts[4];
    for (int i = 0; i < 4; i++) {
      verts[i] = quatRotate(q, baseVerts[i]) * r;
    }

    constexpr uint8_t edges[6][2] = {
      {0,1}, {0,2}, {0,3}, {1,2}, {1,3}, {2,3}
    };

    uint16_t yawBytes = max(0, min(0x1FF, (int)((fabsf(q.w) + fabsf(q.x) + fabsf(q.y) + fabsf(q.z)) * 0x1FF/4)));

    for (int e = 0; e < 6; e++) {
      vectorf &a = verts[edges[e][0]];
      vectorf &b = verts[edges[e][1]];

      // tweak brightness based on average z of endpoints
      float avgZ = (a.z + b.z) / (2.0f * r);  // normalized to [-1, 1]
      uint8_t brightness = 100 + (uint8_t)(155 * (avgZ + 1.0f) / 2.0f); // 100..255

      vectorf pa(a.x, a.y, 0);
      vectorf pb(b.x, b.y, 0);
      fAxial ax1 = axial.rectToHex(pa, 1.0);
      fAxial ax2 = axial.rectToHex(pb, 1.0);

      uint16_t edgeOffset = e * 0x1FF / 6;
      hexline(ctx, ax1, ax2, [this, yawBytes, timeOffset, edgeOffset, brightness] (uint8_t progress) {
        CRGB c = getMirroredPaletteColor(timeOffset + yawBytes + edgeOffset + progress);
        c.nscale8(brightness);
        return c;
      });
    }
  }

  const char *description() {
    return "TriangleSpin";
  }
};

/* ------------------------------------------------------------------------------- */

class PridefulSpinnyThing : public Pattern {
public:
  CRGBPalette256 palettes[6] = {
    Trans_Flag_gp,
    Pride_Flag_gp,
    Genderqueer_Flag_gp,
    Bi_Flag_gp,
    Ace_Flag_gp,
    Lesbian_Flag_gp
  };
  PridefulSpinnyThing() {
    dSpin *= random8(2)?-1:1;
  }
  float avgZ=0;
  int lastSeenAtHighAngle = 0;
  float spinTheta = 0;
  float dSpin = 1/500.;
  void update() {
    
    ICM_20948_AGMT_t agmt = MotionManager::motionFrame.agmt;

    float theta = M_PI+atan2(agmt.acc.axes.y, agmt.acc.axes.x);
    int flag = 6*(theta+M_PI/12) / (2*M_PI);
    flag = mod_wrap(flag,6);
    
    const int maxHexRadius = (kMeridian/2-2);
    const int minHexRadius = -3;
    const float maxZ = 9000.;
    avgZ = min(maxZ, (10*avgZ+agmt.acc.axes.z)/11.f);
    const float maxLineRadius = kMeridian/2+2;
    float lineRadius = maxLineRadius - (maxLineRadius+2) * abs(avgZ) / maxZ;
    float hexRadius = minHexRadius + (maxHexRadius-minHexRadius) * abs(avgZ) / maxZ;

    if (lineRadius > kMeridian/2) {
      lastSeenAtHighAngle = flag;
    }

    ctx.leds.fadeToBlackBy(5 + (hexRadius>0?hexRadius:0));

    float scaledGyr = (agmt.gyr.axes.z / 6666) / 66666.f;
    spinTheta += frameTime() * dSpin;
  
    if (lineRadius > 0) {
      vectorT<float> pt1 = {lineRadius*cosf(spinTheta), lineRadius*sinf(spinTheta)};
      vectorT<float> pt2 = {lineRadius*-cosf(spinTheta), lineRadius*-sinf(spinTheta)};
      fAxial ax1 = axial.rectToHex(pt1, 1.0);
      fAxial ax2 = axial.rectToHex(pt2, 1.0);

      hexline(ctx, ax1, ax2, [this, flag] (uint8_t progress) {
        return ColorFromPalette(palettes[flag], progress);
      });
    }

    if (hexRadius > 0) {
      dSpin += frameTime()*(scaledGyr * (hexRadius/maxHexRadius));
      dSpin = constrain(dSpin, -0.1, 0.1);
      for (int i = 0; i < 6; ++i) {
        float ptTheta = i * 2*+M_PI/6;
        float ptTheta2 = (i+1) * 2*+M_PI/6;
        
        // When we rotate the drawn hexagon at correct angles, there is an aliasing effect where all 6 lines move to the next pixel at the same time
        // causing a visible flicker. shifting each vertex slightly spreads out the next-pixel jumps across different frames and reduces the flicker.
        float vertexBump = 0.01*i;

        vectorT<float> pt1 = {hexRadius*cosf(ptTheta+spinTheta+vertexBump), hexRadius*sinf(ptTheta+spinTheta+vertexBump)};
        vectorT<float> pt2 = {hexRadius*cosf(ptTheta2+spinTheta+vertexBump), hexRadius*sinf(ptTheta2+spinTheta+vertexBump)};
        fAxial ax1 = axial.rectToHex(pt1, 1.0);
        fAxial ax2 = axial.rectToHex(pt2, 1.0);
        
        hexline(ctx, ax1, ax2, [this, i] (uint8_t progress) {
          return PaletteRotation<CRGBPalette256>::getMirroredPaletteColor(palettes[lastSeenAtHighAngle], progress/3 + 0xFF*i/3);
        });
      }
    }
  }
  const char *description() {
    return "PridefulSpinnyThing";
  }
};

/* ------------------------------------------------------------------------------- */

// Displays a single digit (0-9), chosen by spinning the hexa around its face like a
// dial. Each digit gets its own red/green confusion pair in the spirit of an
// Ishihara colorblindness test plate: figure and field are luminance-matched and
// differ mainly by hue, so the digit reads clearly to typical color vision but
// washes out for red-green colorblind viewers. Tilting the hexa up onto an edge
// gently nudges each digit's pair along its own hue family, echoing the way
// PixelSand/LargeBouncyBall let motion drive their color.
class IshiharaDigits : public Pattern {
public:
  struct DigitPalette {
    CRGB field0, figure0; // colors while resting flat
    CRGB field1, figure1; // colors while tilted up onto an edge
  };

  // Ten luminance-matched field/figure pairs, one per digit. Each pair varies only
  // in how far the field leans orange and the figure leans green -- blue and overall
  // brightness are held constant -- which is the axis a real red-green confusion
  // plate uses. Exact plate inks vary by printing; these are a from-scratch
  // approximation, easy to retune here if you have exact values you'd rather match.
  const DigitPalette kDigitPalettes[10] = {
    {CRGB(200,144,64), CRGB(110,171,64), CRGB(215,139,64), CRGB(95,175,64)},  // 0
    {CRGB(190,147,64), CRGB(120,168,64), CRGB(205,142,64), CRGB(105,172,64)}, // 1
    {CRGB(210,141,64), CRGB(100,174,64), CRGB(225,136,64), CRGB(85,178,64)},  // 2
    {CRGB(180,150,64), CRGB(130,165,64), CRGB(195,145,64), CRGB(115,169,64)}, // 3
    {CRGB(205,142,64), CRGB(90,177,64),  CRGB(220,138,64), CRGB(75,181,64)},  // 4
    {CRGB(195,145,64), CRGB(115,169,64), CRGB(210,141,64), CRGB(100,174,64)}, // 5
    {CRGB(185,148,64), CRGB(125,166,64), CRGB(170,153,64), CRGB(140,162,64)}, // 6
    {CRGB(215,139,64), CRGB(95,175,64),  CRGB(230,135,64), CRGB(80,180,64)},  // 7
    {CRGB(175,151,64), CRGB(135,163,64), CRGB(160,156,64), CRGB(150,159,64)}, // 8
    {CRGB(208,141,64), CRGB(105,172,64), CRGB(193,146,64), CRGB(118,168,64)}, // 9
  };

  const int kFontCols = 5;
  const int kFontRows = 7;
  const int kGlyphScale = 2;  // each font pixel becomes a kGlyphScale x kGlyphScale block of LEDs
  const int kTopRow = -6;     // hex row (r) of the glyph's top edge
  const int kLeftCol = -5;    // visual column of the glyph's left edge
  const unsigned long kFadeOutMS = 150;
  const unsigned long kFadeInMS = 200;

  // classic 5x7 dot-matrix digit font, one bit per column (MSB = leftmost column)
  const uint8_t kDigitFont[10][7] = {
    {0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110}, // 0
    {0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110}, // 1
    {0b01110,0b10001,0b00001,0b00010,0b00100,0b01000,0b11111}, // 2
    {0b11111,0b00010,0b00100,0b00010,0b00001,0b10001,0b01110}, // 3
    {0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010}, // 4
    {0b11111,0b10000,0b11110,0b00001,0b00001,0b10001,0b01110}, // 5
    {0b00110,0b01000,0b10000,0b11110,0b10001,0b10001,0b01110}, // 6
    {0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000}, // 7
    {0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110}, // 8
    {0b01110,0b10001,0b10001,0b01111,0b00001,0b00010,0b01100}, // 9
  };

  vector32 smoothAcc;
  int currentDigit = -1;
  int previousDigit = -1;
  unsigned long transitionStart = 0;
  bool figureMask[LED_COUNT];

  // maps the digit's font bits onto hex pixel indices, compensating for the hex
  // grid's per-row shear so glyph columns stay visually aligned top to bottom
  void addDigitMask(int digit) {
    memset(figureMask, 0, sizeof(figureMask));
    if (digit < 0) return;
    for (int gr = 0; gr < kFontRows; ++gr) {
      uint8_t rowBits = kDigitFont[digit][gr];
      for (int gc = 0; gc < kFontCols; ++gc) {
        if (!((rowBits >> (kFontCols - 1 - gc)) & 1)) continue;
        for (int dr = 0; dr < kGlyphScale; ++dr) {
          int r = kTopRow + gr * kGlyphScale + dr;
          int parity = ((r % 2) + 2) % 2; // handles negative r correctly
          for (int dc = 0; dc < kGlyphScale; ++dc) {
            int visualCol = kLeftCol + gc * kGlyphScale + dc;
            int q = visualCol - (r - parity) / 2;
            auto pxOpt = axial.indexAtAxial(q, r);
            if (pxOpt.has_value()) {
              figureMask[pxOpt.value()] = true;
            }
          }
        }
      }
    }
  }

  void update() {
    ICM_20948_AGMT_t agmt = MotionManager::motionFrame.agmt;
    vector32 acc(agmt.acc.axes.x, agmt.acc.axes.y, agmt.acc.axes.z);
    smoothAcc = (9 * smoothAcc + acc) / 10;

    // which digit: angle of rotation about the hexa's face, split into 10 wedges
    float theta = atan2((float)smoothAcc.y, (float)smoothAcc.x);
    int wedge = (int)floor(10 * (theta + M_PI) / (2 * M_PI));
    wedge = ((wedge % 10) + 10) % 10;
    if (wedge != currentDigit) {
      previousDigit = currentDigit;
      currentDigit = wedge;
      transitionStart = millis();
    }

    // how tilted up onto an edge the hexa is: 0 = resting flat, 0xFF = on edge
    float ax = smoothAcc.x, ay = smoothAcc.y, az = smoothAcc.z;
    float mag = sqrt(ax*ax + ay*ay + az*az);
    uint8_t tiltAmount = 0;
    if (mag > 1) {
      float t = constrain(1.0f - fabs(az) / mag, 0.0f, 1.0f);
      tiltAmount = (uint8_t)(t * 0xFF);
    }

    // which digit to actually draw this frame, and at what brightness -- fades
    // through black when the displayed digit changes
    int drawDigit = currentDigit;
    uint8_t brightness = 0xFF;
    unsigned long elapsed = millis() - transitionStart;
    if (previousDigit >= 0 && elapsed < kFadeOutMS) {
      drawDigit = previousDigit;
      brightness = 0xFF - ease8InOutQuad(0xFF * elapsed / kFadeOutMS);
    } else if (elapsed < kFadeOutMS + kFadeInMS) {
      unsigned long fadeInElapsed = (previousDigit >= 0) ? elapsed - kFadeOutMS : elapsed;
      brightness = ease8InOutQuad(0xFF * min(fadeInElapsed, kFadeInMS) / kFadeInMS);
    }

    const DigitPalette &pal = kDigitPalettes[drawDigit];
    CRGB figure = blend(pal.figure0, pal.figure1, tiltAmount);
    CRGB field = blend(pal.field0, pal.field1, tiltAmount);
    figure.nscale8(brightness);
    field = blend(CRGB::Black, field, brightness);

    addDigitMask(drawDigit);
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      ctx.leds[px] = figureMask[px] ? figure : field;
    }
  }

  const char *description() {
    return "IshiharaDigits";
  }
};

/* ------------------------------------------------------------------------------- */

class SoundPattern : public Pattern, public FFTReceiver {
public:
  unsigned long lastLevelThreshChange{0};
  int minFFTLevelThreshold{3};
  int fftLevelThreshold{minFFTLevelThreshold};
  int autoGainAdjustmentInterval{600};

  SoundPattern() : FFTReceiver(fftProcessing) {
    // stop main loop from lowering framerate when we have nothing to draw, since that results in visibly-delayed response to sounds
    fc.takeFPSAssertion(); 
  }
  ~SoundPattern() {
    fc.releaseFPSAssertion();
  }
  void autoGainUpdate() {
    FFTFrame frame = fftProcessing.getDataFrame();
    unsigned long mils = millis();

    int maxFrameValue = 0;
    int32_t sumFrameValue = 0;
    for (int i = 0 ; i < frame.size; ++i) {
      if (frame.smoothSpectrum[i] > maxFrameValue) {
        maxFrameValue = frame.smoothSpectrum[i];
      }
      sumFrameValue += frame.smoothSpectrum[i];
    }
    int avgFrameValue = sumFrameValue/frame.size;

    int litCount{0};
    for (int i = 0 ; i < LED_COUNT; ++i) {
      litCount += ctx.leds[i] ? 1 : 0;
    }
    /* latch-ditch auto gain:
     * slowly adjust threshold for drawing if to approach the average levels
     * quickly move threshold for drawing if we're over- or under-drawing
     * temporarily adjust thresholds at a fast interval at the start of pattern running to find a baseline
    */
   bool overDrawing = litCount > 95*LED_COUNT/100;
   bool underDrawing = litCount < 2*LED_COUNT/10;
   int adjustmentInterval = (runTime() > 3000 ? autoGainAdjustmentInterval : autoGainAdjustmentInterval/6);
   if ((overDrawing || fftLevelThreshold < avgFrameValue) && mils - lastLevelThreshChange > adjustmentInterval) {
      fftLevelThreshold++;
      if (overDrawing) {
        fftLevelThreshold += max(0, (maxFrameValue - fftLevelThreshold) / 20);
      }
      // logf("SoundPattern litCount = %i, frame value avg=%i,max=%i, fftLevelThreshold up to %i", litCount, avgFrameValue, maxFrameValue, fftLevelThreshold);
      lastLevelThreshChange = mils;
    } else if (fftLevelThreshold > minFFTLevelThreshold && (underDrawing || fftLevelThreshold > avgFrameValue) && mils - lastLevelThreshChange > adjustmentInterval) {
      fftLevelThreshold--;
      if (underDrawing) {
        fftLevelThreshold = max(minFFTLevelThreshold, fftLevelThreshold + min(0, (maxFrameValue - fftLevelThreshold) / 10));
      }
      // logf("SoundPattern litCount = %i, frame value avg=%i,max=%i, fftLevelThreshold down to %i", litCount, avgFrameValue, maxFrameValue, fftLevelThreshold);
      lastLevelThreshChange = mils;
    }
  }
};

class SoundDroplets : public SoundPattern, public PaletteRotation<CRGBPalette256> {
  HexaShells shells;
  CRGB cs[LED_COUNT] = {0}; // scratch space
  unsigned long lastFlow = 0;
  unsigned long lastLevelThreshChange;
public:
  int dropletSize;

  SoundDroplets(int size) : dropletSize(size) {
    minBrightness = 20;
  }

  void flowDroplets(int i, int i2) {
    // This sub-pixel flow algorithm leaves a lot of r,g,&b residue pixels during fadedown
    const int kFlow = 5;//%
    const int kEff = 80;//%
    const int minLoss = 1;

    // calculate flows from og leds, set in scratch
    CRGB led1 = ctx.leds[i];
    CRGB led2 = ctx.leds[i2];
    for (uint8_t sp = 0; sp < 3; ++sp) { // each subpixel
      uint8_t *refSp = NULL;
      uint8_t *srcSp = NULL;
      uint8_t *dstSp = NULL;
      if (led1[sp] < led2[sp]) {
        refSp = &led2[sp];
        srcSp = &cs[i2][sp];
        dstSp = &cs[i][sp];
      } else if (led1[sp] > led2[sp] ) {
        refSp = &led1[sp];
        srcSp = &cs[i][sp];
        dstSp = &cs[i2][sp];
      }
      if (srcSp && dstSp) {
        uint8_t flow = min(*srcSp, min(*refSp * kFlow/100, 0xFF - *dstSp));
        *dstSp += flow * kEff/100;
        *srcSp = max(0, *srcSp - max(minLoss, flow));
      }
    }
  }

  void makeDroplet(PixelIndex px, int size, uint8_t phase, uint8_t brightness, uint8_t gradientDropoff=0x7F) {
    CRGB color = getPaletteColor(phase, brightness);
    if (size > 0) {
      ctx.leds[px] = color;
    }
    if (size > 1) {
      HexaShells droplet(px, size);
      for (int s = 1; s < size; ++s) {
        brightness = scale8(brightness, gradientDropoff);
        color = getPaletteColor(phase + 20*s, brightness);
        for (int si = 0; si < droplet.shells[s].size(); ++si) {
          auto d = droplet.shells[s][si];
          if (d.has_value()) {
            ctx.leds[d.value()] = color;
          }
        }
      }
    }
  }

  void update() {
    unsigned long mils = millis();
    FFTFrame frame = spectrumFrame();

    for (int s = 0 ; s < min(frame.size, shells.shells.size()); ++s) {
      int32_t level = frame.spectrum[s] - fftLevelThreshold;
      if (level > 0) {
        int shellNum = (s + millis()/1000 + random8()%2) % shells.shells.size();
        int indexInShell = random16()%shells.shells[shellNum].size();
        
        paletteRotate(MotionManager::motionFrame.agmt.gyr.axes.z/1000);

        auto pxOpt = shells.shells[shellNum][indexInShell];
        if (!pxOpt.has_value()) continue;
        PixelIndex px = pxOpt.value();
        uint8_t phase = s*15+millis()/100;
        uint8_t brightness = min(0xFF, level*20);
        makeDroplet(px, dropletSize,phase, brightness);
      }
    }

    int pixelsLit = 0;
    const unsigned int flowInterval = 30;
    if (mils - lastFlow > flowInterval) {
      for (int i = 0; i < LED_COUNT; ++i) {
        cs[i] = ctx.leds[i];
      }
      for (int i = 0; i < LED_COUNT; ++i) {
        Axial ax = axial.axialFromPixelIndex(i);
        std::optional<PixelIndex> other;
        other = axial.indexAtAxial(ax + Axial(1,0));
        if (other.has_value()) flowDroplets(i, other.value());
        other = axial.indexAtAxial(ax + Axial(-1,1));
        if (other.has_value()) flowDroplets(i, other.value());
        other = axial.indexAtAxial(ax + Axial(0,1));
        if (other.has_value()) flowDroplets(i, other.value());
      }
      for (int i = 0; i < LED_COUNT; ++i) {
        ctx.leds[i] = cs[i];
      }
      lastFlow  = mils;
    }
    autoGainUpdate();
  }
  const char *description() {
    return "SoundDroplets";
  }
};

class SparkleDroplets : public SoundDroplets {
public:
  SparkleDroplets() : SoundDroplets(1) { }
  const char *description() {
    return "SparkleDroplets";
  }
};

class BlobDroplets : public SoundDroplets {
public:
  BlobDroplets() : SoundDroplets(2) { }
  const char *description() {
    return "BlobDroplets";
  }
};

class SoundBits : public SoundPattern, public PaletteRotation<CRGBPalette256> {
  HexaShells shells;
public:
  ParticleSim<LED_COUNT> particles;

  int bitLoudZoom = 70;

  SoundBits() : particles(ledgraph, ctx, 0, 0, 1200, {clockwise, counterclockwise}) {
    minBrightness = 20;
    particles.setFadeUpDistance(1);
    particles.handleUpdateParticle = [this](Particle &bit, uint8_t index) {
      if (bit.age() > bit.lifespan/2) {
        bit.brightness = min(0xFF, max(0, (int)(0xFF - 0xAF * (bit.age()-bit.lifespan/2) / (bit.lifespan-bit.lifespan/2))));
      }
      if (bit.speed > bitLoudZoom - bitLoudZoom * bit.age() / bit.lifespan) {
        bit.speed-=2;
      }
    };
  }

  vector32 gyrAccum32;
  
  void update() {
    unsigned long mils = millis();
    
    auto agmt = MotionManager::motionFrame.agmt;
    gyrAccum32 += vector16(agmt.gyr.axes.x/100, agmt.gyr.axes.y/100, agmt.gyr.axes.z/100); // drop low order noisy data
    
    paletteRotate(MotionManager::motionFrame.agmt.gyr.axes.z/1000);

    FFTFrame frame = spectrumFrame();
    for (int s = 0 ; s < min(frame.size, shells.shells.size()); ++s) {
      int32_t level = frame.spectrum[s] - fftLevelThreshold;
      if (level > 0 && particles.particles.size() < 255) {
        int shellNum = (s + millis()/1000 + random8()%2 + gyrAccum32.x/200) % shells.shells.size();
        int indexInShell = random16()%shells.shells[shellNum].size();
        
        unsigned maxlifespan = 300;
        Particle &p = particles.addParticle();
        p.px = shells.shells[shellNum][indexInShell].value();
        p.lifespan = max(1, min(maxlifespan, maxlifespan * level/30));
        uint8_t phase = s*15+millis()/100;
        uint8_t brightness = min(0xFF, level*10);
        p.color = getPaletteColor(phase, brightness);
        p.speed = min(bitLoudZoom, 3*level);
      }
      autoGainUpdate();
    }
    particles.update();
  }

  const char *description() {
    return "SoundBits";
  }
};

/* ------------------------------------------------------------------------------- */

class ChargingPattern : public Pattern {
public:
  int lastStateOfCharge = 0;
  int animateFromSOC = 0;
  unsigned long lastValueChange;

  ChargingPattern() : lastValueChange(millis()) {
    btlogf("[t=%lu] ChargingPattern start: batteryInitialized=%i soc=%u%% flags=%X detected=%i",
           millis(), powerState.batteryInitialized, batteryData.stateOfCharge, batteryData.flags,
           batteryData.batteryDetected());
  }
  void update() {
    ctx.leds.fill_solid(CRGB::Black);
    HexaShells shells;
    auto outerShell = shells.shells.back();

    const int ringAnimateTime = 1000;
    const int minHue = 0;
    const int maxHue = 0x66;
    const PixelIndex firstIdx = 14; // start near usb port
    int SOC = min(100, 100 * batteryData.stateOfCharge / kFullCharge);

    // animate any jumps in reported battery value
    if (SOC != lastStateOfCharge) {
      lastValueChange = millis();
      animateFromSOC = lastStateOfCharge;
      lastStateOfCharge = SOC;
    }
    
    long animationMillis = millis() - lastValueChange;
    int displaySOC = (animationMillis > ringAnimateTime)
                      ? SOC
                      : (animateFromSOC + (SOC - animateFromSOC) * ease8InOutQuad(0xFF*animationMillis/ringAnimateTime) / 0xFF);
    int displayLength = displaySOC * outerShell.size() / 100;
    int maxLength = SOC * outerShell.size() / 100;
    
    uint8_t hue = maxHue * SOC / 100 - minHue;
    CRGB color = CHSV(hue, 0xFF, 0xAF);

    for (int i = 0; i < displayLength; ++i) {
      ctx.leds[outerShell[(i + firstIdx) % outerShell.size()].value()] = color.scale8(0x50 + 0x9F*i / displayLength);
    }
    if (animationMillis > ringAnimateTime) {
      if (displayLength < outerShell.size()) {
        ctx.leds[outerShell[(displayLength + firstIdx) % outerShell.size()].value()] = color.scale8(beatsin8(30));
      }
    }
  }
  const char *description() {
    return "ChargingPattern";
  }
};

class PowerOnOffAnimation : public Pattern {
  const int maxPosition = kMeridian/2;
  float position; // distance from origin 
public:
  bool animatingPowerOn = true;
  PowerOnOffAnimation(bool poweringOn) : animatingPowerOn(poweringOn), position(poweringOn?0:maxPosition) {
    setPoweringOn(poweringOn);
  }

  void setPoweringOn(bool poweringOn) {
    animatingPowerOn = poweringOn;
  }

  float progress() {
    return (animatingPowerOn ? position / maxPosition : (maxPosition - position) / maxPosition);
  }

  void update() {
    uint8_t centerPixelRed = ctx.leds[LED_COUNT/2].red;
    ctx.leds.fill_solid(CRGB::Black);
    
    const int duration = 1000;

    position += (animatingPowerOn ? 1 : -1) * (int)frameTime() * maxPosition / (float)duration;
    if (position < 0) {
      const int powerOffDonePos = -5;
      if (position < powerOffDonePos) {
        stop();
      } else {
        // final dot
        ctx.leds[LED_COUNT/2] = CHSV(0, 0xFF, 0xFF - 0xFF*(position/powerOffDonePos));
      }
    } else if (position > maxPosition) {
      stop();
    } else {
      const int waveSize = 5;
      const float expand = 1.8; // factor to expand the animation from the logical position
      float animationPosition = position * expand - maxPosition*(expand-1)/2;
      for (int q = 0; q <= maxPosition; ++q) {
        float distance = fabs(q - animationPosition);
        Axial ax(q,0);
        for (int i = 0; i < 6; ++i) {
          auto pxOpt = axial.indexAtAxial(ax);
          if (pxOpt) {
            PixelIndex px = pxOpt.value();
            CRGB c = CHSV(0, 0xFF, 0xFF - 0xFF * distance/waveSize);
            if (px == LED_COUNT/2 && (!animatingPowerOn || progress() < 0.2)) {
              // hack to keep the final dot at a consistent brightness at the end, as well as after resuming a canceled power-on animation
              c.red = max(c.red, centerPixelRed); 
            }
            ctx.leds[px] = c;
          }
          // rotate to next spoke
          ax = Axial(-ax.r(), -ax.s());
        }
      }
    }
  }
  const char *description() {
    return (animatingPowerOn ? "PowerOn" : "PowerOff");
  }
};

class BlinkIdentifyPattern : public Pattern {
  const int blinkTime = 900;
  HexaShells hexaShells;
  void update() {
    unsigned long rt = runTime();
    ctx.leds.fadeToBlackBy(20);
    int shell = hexaShells.shells.size() * triwave8(0xFF * rt / (blinkTime/3)) / 0xFF;
    shell = min(hexaShells.shells.size(), shell);
    for (int i = 0; i < hexaShells.shells[shell].size(); ++i) {
      ctx.leds[hexaShells.shells[shell][i].value()] = CRGB::Blue;
    }
    if (rt >= blinkTime) {
      stop();
    }
  }
  const char *description() {
    return "BlinkIdentifyPattern";
  }
};

/* ------------------------------------------------------------------------------- */

#include "rickroll_data.h"

// A prank pattern. This is genuine sampled pixel data, not hand-painted: once
// rik.gif existed as a real file (it didn't at first -- it was only ever
// visible inline in chat, so there was nothing to decode), scripts/convert_gif.py
// (Pillow) replicated ledgraph.h's exact AxialAccess pixel-index<->(q,r)
// mapping and hexToRect formula, mapped each of the 271 hex pixel positions
// into the gif's frame coordinates, and averaged a small patch of source
// pixels around each mapped point per frame (271 pixels can't resolve
// 498x427 1:1, so a patch average reads far cleaner than nearest-neighbor).
// All 32 frames are baked into rickroll_data.h at the gif's own 50ms/frame
// pace. Re-run scripts/convert_gif.py after replacing rik.gif to regenerate it.
class RickRoll : public Pattern {
  const float kUprightThreshold = 0.6f;   // in-plane gravity fraction past which we trust an edge is down
  const float kSpeedDeadzone = 0.15f;
  const float kTopSpeedMultiplier = 4.0f;    // tilt forward: sped up
  const float kBottomSpeedMultiplier = 0.2f; // tilt back: slowed down (never fully frozen)

  vector32 smoothAcc;
  int rotationSteps = 0;         // 0..5, current 60deg-quantized orientation lock
  int cachedRotationSteps = -1;  // forces rotatedSource to build on first update()
  PixelIndex rotatedSource[LED_COUNT];
  double playbackMs = 0;

  // rotatedSource[physical pixel] = which baked-frame pixel supplies its color, so
  // that whichever of the hexa's 6 edges is currently resting down, the image
  // still reads right-side-up to the viewer. Quantized to exact 60deg steps
  // (rather than a continuous resample) because the hex grid only has true 6-fold
  // rotational symmetry -- anything finer would have to blur between pixels.
  void buildRotationMap(int steps) {
    float theta = steps * (float)(M_PI / 3.0);
    float c = cosf(theta), s = sinf(theta);
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      Axial a = axial.axialFromPixelIndex(px);
      vectorf rect = axial.hexToRect(fAxial(a), 1);
      // rotate this physical pixel's position by -theta to find the source
      // (canonically upright) pixel that should be displayed here
      float rx =  rect.x * c + rect.y * s;
      float ry = -rect.x * s + rect.y * c;
      Axial ipos = axial.rectToHex(vectorf(rx, ry), 1.0).cubeRound();
      auto srcOpt = axial.indexAtAxial(ipos);
      rotatedSource[px] = srcOpt.has_value() ? srcOpt.value() : px;
    }
    cachedRotationSteps = steps;
  }

public:
  void update() {
    ICM_20948_AGMT_t agmt = MotionManager::motionFrame.agmt;
    vector32 acc(agmt.acc.axes.x, agmt.acc.axes.y, agmt.acc.axes.z);
    smoothAcc = (10 * smoothAcc + acc) / 11;
    float ax = smoothAcc.x, ay = smoothAcc.y, az = smoothAcc.z;
    float mag = sqrtf(ax*ax + ay*ay + az*az);
    float inPlaneMag = sqrtf(ax*ax + ay*ay);

    // which of the 6 side edges is down, from in-plane gravity direction --
    // same (ax,ay) == pixel-rect-space convention AnalogClock/SolarSystem rely
    // on. Only trusted (and only updates the lock) once tilt is clearly past
    // flat, so lying on either face keeps whatever orientation was last locked
    // in rather than snapping to noise.
    if (mag > 1 && inPlaneMag / mag > kUprightThreshold) {
      float gravityAngle = atan2f(ay, ax);
      float delta = gravityAngle + (float)M_PI/2.0f; // rotation so image "up" ends up opposite gravity
      int steps = ((int)roundf(delta / (float)(M_PI/3.0))) % 6;
      if (steps < 0) steps += 6;
      rotationSteps = steps;
    }
    if (rotationSteps != cachedRotationSteps) {
      buildRotationMap(rotationSteps);
    }

    // rocking the standing hexa forward/back (tipping it toward or away from
    // the viewer) speeds up or slows down playback; dead zone then an eased
    // ramp, same shape as the other patterns' tilt speed controls.
    float tiltFrontBack = (mag > 1) ? constrain(az / mag, -1.0f, 1.0f) : 0;
    float absTilt = fabsf(tiltFrontBack);
    float speedMultiplier;
    if (absTilt <= kSpeedDeadzone) {
      speedMultiplier = 1.0f;
    } else {
      float t = (absTilt - kSpeedDeadzone) / (1.0f - kSpeedDeadzone);
      float eased = t * t;
      speedMultiplier = (tiltFrontBack > 0)
        ? 1.0f + eased * (kTopSpeedMultiplier - 1.0f)
        : 1.0f - eased * (1.0f - kBottomSpeedMultiplier);
    }

    playbackMs += frameTime() * speedMultiplier;
    int frame = ((int)(playbackMs / RICKROLL_FRAME_MS)) % RICKROLL_FRAME_COUNT;

    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      PixelIndex src = rotatedSource[px];
      ctx.leds[px] = CRGB(kRickRollFrames[frame][src][0], kRickRollFrames[frame][src][1], kRickRollFrames[frame][src][2]);
    }
  }

  const char *description() {
    return "RickRoll";
  }
};

class Billiards : public Pattern {
  static const int kBallCount = 7; // 0 = cue, 1..6 = colored
  struct Ball {
    vectorf pos;
    vectorf velocity;
    bool inPlay;
    CRGB color;
  };

  // ball's effective size for ball-ball separation -- comfortably bigger than
  // the 7-pixel flower's true visual radius of 1 unit, so two balls' flowers
  // clearly separate before physics stops correcting them
  const float kBallRadius = 1.2f;
  const int kCollisionPasses = 2; // extra passes so a multi-ball pileup un-overlaps within one frame
  const float kWallElasticity = 0.85f;
  const float kBallRestitution = 0.98f;
  const float kFrictionK = 0.0006f;   // velocity decay per ms; tuned by feel, easy to retune
  const float kRestEpsilon = 0.0003f; // units/ms below which a ball is considered stopped
  const int kPocketOverlapToPot = 4;  // of the ball's 7 rendered pixels, how many must land in a pocket's 9 to sink it

  const float kPowerDeadzone = 0.24f;      // in-plane tilt fraction below which we're not aiming at all -- wide margin so picking up/repositioning the hexa can't fire a false shot
  const float kMinShotPower = 0.3f;        // charged power required, past the deadzone, to allow firing
  const float kAimAngleTolerance = 0.3f;   // radians (~17deg) the pull-back direction may drift and still count as "held"
  const float kAimPowerTolerance = 0.12f;  // fraction of power range allowed to drift and still count as "held"
  const unsigned long kHoldStableMs = 1100; // how long the pull-back has to stay steady before it fires
  const float kMaxShotSpeed = 0.03f;       // units/ms at full power (50% up from the first pass, so shots can actually reach a pocket)
  const float kIndicatorMaxLength = 12.0f;
  const float kCueTipLength = 0.5f;        // green contact tip, nearest the cue ball
  const float kCueFerruleLength = 0.5f;    // white ferrule, just behind the tip
  const CRGB kCueTipColor = CRGB(20, 160, 60);
  const CRGB kCueFerruleColor = CRGB(235, 235, 225);
  const CRGB kCueWoodColor = CRGB(150, 105, 65); // the rest of the shaft, back toward the butt
  const unsigned long kWinPauseMs = 2500;
  const unsigned long kSadPauseMs = 1300;
  const int kConfettiPerFrame = 14;

  static const int kPocketCount = 4;
  static const int kPocketPixelCount = 9;
  Ball balls[kBallCount];
  vectorf pocketPos[kPocketCount];
  PixelIndex pocketPixels[kPocketCount][kPocketPixelCount];
  vectorf cueStartPos;

  vector32 smoothAcc;
  bool charging = false;
  float chargeDirAngle = 0;
  float chargePower = 0;
  unsigned long stableStartMs = 0;
  unsigned long lastUpdateMs = 0;
  unsigned long wonSinceMs = 0;
  unsigned long scratchedSinceMs = 0;

  // pockets only at 4 of the hexagon's 6 corners (the two along the cue/rack
  // axis plus the two opposite side corners, skipping the remaining opposite
  // pair to keep the table symmetric). Each pocket is a 9-pixel diamond: a 3x3
  // parallelogram anchored right at the true corner, built from that corner's
  // two boundary-preserving neighbor directions (every hex corner has exactly
  // two -- the two edges meeting there -- plus one that steps inward, which we
  // don't use here), so it's guaranteed to stay on the board and to sit
  // exactly at the point of the table, not pulled inward.
  void computePockets() {
    static const int16_t cq[kPocketCount] = {9, 0, -9, 0};
    static const int16_t cr[kPocketCount] = {0, -9, 0, 9};
    static const int8_t ndq[6] = {1, 1, 0, -1, -1, 0};
    static const int8_t ndr[6] = {0, -1, -1, 0, 1, 1};
    const int n = kMeridian / 2;
    for (int p = 0; p < kPocketCount; ++p) {
      Axial corner(cq[p], cr[p]);
      pocketPos[p] = axial.hexToRect(fAxial(corner), 1);

      int8_t edgeDQ[2], edgeDR[2];
      int found = 0;
      for (int d = 0; d < 6 && found < 2; ++d) {
        int nq = corner.q() + ndq[d], nr = corner.r() + ndr[d], ns = -nq - nr;
        if (max(max(abs(nq), abs(nr)), abs(ns)) == n) {
          edgeDQ[found] = ndq[d];
          edgeDR[found] = ndr[d];
          found++;
        }
      }
      int idx = 0;
      for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
          int q = corner.q() + i * edgeDQ[0] + j * edgeDQ[1];
          int r = corner.r() + i * edgeDR[0] + j * edgeDR[1];
          auto pxOpt = axial.indexAtAxial(q, r);
          pocketPixels[p][idx++] = pxOpt.has_value() ? pxOpt.value() : axial.indexAtAxial(corner).value();
        }
      }
    }
  }

  void rack() {
    const float r = kBallRadius;
    const float rowSpacing = r * kSqrtThree; // touching balls in adjacent rows
    const float apexX = 3.0f;
    const vectorf rackPos[6] = {
      vectorf(apexX, 0),
      vectorf(apexX + rowSpacing, -r),          vectorf(apexX + rowSpacing, r),
      vectorf(apexX + 2 * rowSpacing, -2 * r),  vectorf(apexX + 2 * rowSpacing, 0), vectorf(apexX + 2 * rowSpacing, 2 * r),
    };
    for (int i = 0; i < 6; ++i) {
      balls[1 + i].pos = rackPos[i];
      balls[1 + i].velocity = vectorf(0, 0);
      balls[1 + i].inPlay = true;
      balls[1 + i].color = CHSV((uint8_t)(i * 0xFF / 6), 0xFF, 0xFF);
    }
    cueStartPos = vectorf(-4.0f, 0);
    balls[0].pos = cueStartPos;
    balls[0].velocity = vectorf(0, 0);
    balls[0].inPlay = true;
    balls[0].color = CRGB::White;
    wonSinceMs = 0;
  }

  // reflect a ball off the hexagonal cushion -- same wall-line construction
  // as LargeBouncyBall::sideCollision, proven on real hardware. Uses the true
  // apothem (no inset) so a ball's center can travel all the way to the true
  // corners, where the pocket diamonds live -- otherwise the cushion would
  // stop it short and a pocket could never actually be reached.
  void wallCollide(vectorf &pos, vectorf &vel) {
    static const float apothem = 9.0f * kSqrtThreeOverTwo;
    const linef urLine(kSqrtThree,   1, -apothem * kSqrtThree);
    const linef uLine ( 0,           1, -apothem * kSqrtThree / 2);
    const linef ulLine(-kSqrtThree,  1, -apothem * kSqrtThree);
    const linef dlLine(-kSqrtThree, -1, -apothem * kSqrtThree);
    const linef dLine ( 0,          -1, -apothem * kSqrtThree / 2);
    const linef drLine(kSqrtThree,  -1, -apothem * kSqrtThree);
    const linef lines[] = {uLine, urLine, drLine, dLine, dlLine, ulLine};

    for (int it = 0; it < 3; ++it) { // a few passes lets corners resolve cleanly
      bool collided = false;
      for (int w = 0; w < 6; ++w) {
        const linef &wall = lines[w];
        float dist = wall.A * pos.x + wall.B * pos.y + wall.C;
        if (dist > 0) {
          float nLenSq = wall.A * wall.A + wall.B * wall.B;
          pos.x -= 2 * wall.A * dist / nLenSq;
          pos.y -= 2 * wall.B * dist / nLenSq;
          float vDotN = vel.x * wall.A + vel.y * wall.B;
          if (vDotN > 0) {
            vel.x -= 2 * wall.A * vDotN / nLenSq * kWallElasticity;
            vel.y -= 2 * wall.B * vDotN / nLenSq * kWallElasticity;
          }
          collided = true;
          break;
        }
      }
      if (!collided) break;
    }
  }

  // equal-mass elastic collision: swap the velocity components along the
  // line connecting centers, leave the tangential components untouched.
  // Runs kCollisionPasses times so a multi-ball pileup (e.g. right after a
  // break shot) fully separates within a single frame instead of leaving a
  // residual overlap for resolveBallCollisions to slowly grind through frame
  // by frame.
  void resolveBallCollisions() {
    const float minDist = 2 * kBallRadius;
    for (int pass = 0; pass < kCollisionPasses; ++pass) {
      for (int i = 0; i < kBallCount; ++i) {
        if (!balls[i].inPlay) continue;
        for (int j = i + 1; j < kBallCount; ++j) {
          if (!balls[j].inPlay) continue;
          float dx = balls[j].pos.x - balls[i].pos.x;
          float dy = balls[j].pos.y - balls[i].pos.y;
          float dist = sqrtf(dx * dx + dy * dy);
          if (dist < minDist) {
            float nx, ny;
            if (dist > 0.0001f) {
              nx = dx / dist; ny = dy / dist;
            } else {
              // exact coincidence (e.g. a scratch respawning the cue ball
              // right on top of another) -- still needs to be pushed apart
              nx = 1; ny = 0;
              dist = 0;
            }
            float overlap = (minDist - dist) * 0.5f;
            balls[i].pos.x -= nx * overlap; balls[i].pos.y -= ny * overlap;
            balls[j].pos.x += nx * overlap; balls[j].pos.y += ny * overlap;

            float relVelAlongNormal = (balls[i].velocity.x - balls[j].velocity.x) * nx
                                     + (balls[i].velocity.y - balls[j].velocity.y) * ny;
            if (relVelAlongNormal > 0) {
              float impulse = relVelAlongNormal * kBallRestitution;
              balls[i].velocity.x -= impulse * nx; balls[i].velocity.y -= impulse * ny;
              balls[j].velocity.x += impulse * nx; balls[j].velocity.y += impulse * ny;
            }
          }
        }
      }
    }
  }

  // blend-brighten a straight line between two rect-space points -- brighten
  // (not overwrite) so the cue reads as a translucent highlight over
  // whatever's underneath, including another ball, rather than replacing it.
  // colorAt is given the distance already traveled from "from" (the ball end).
  void drawIndicatorLine(vectorf from, vectorf to, std::function<CRGB(float)> colorAt) {
    float dx = to.x - from.x, dy = to.y - from.y;
    float dist = sqrtf(dx * dx + dy * dy);
    int steps = (int)(dist * 2.0f) + 1;
    for (int i = 0; i <= steps; ++i) {
      float t = (float)i / steps;
      vectorf pos(from.x + dx * t, from.y + dy * t);
      Axial ipos = axial.rectToHex(pos, 1.0).cubeRound();
      auto pxOpt = axial.indexAtAxial(ipos);
      if (pxOpt.has_value()) {
        ctx.point(pxOpt.value(), colorAt(t * dist), blendBrighten);
      }
    }
  }

  // a ball's 7-pixel flower (its nearest grid point plus that point's 6
  // immediate neighbors), used both to render it and to test pocket overlap
  int ballFootprint(const vectorf &pos, PixelIndex *out) {
    static const int8_t ndq[6] = {1, 1, 0, -1, -1, 0};
    static const int8_t ndr[6] = {0, -1, -1, 0, 1, 1};
    int count = 0;
    Axial ipos = axial.rectToHex(pos, 1.0).cubeRound();
    auto pxOpt = axial.indexAtAxial(ipos);
    if (pxOpt.has_value()) out[count++] = pxOpt.value();
    for (int d = 0; d < 6; ++d) {
      auto nOpt = axial.indexAtAxial(ipos.q() + ndq[d], ipos.r() + ndr[d]);
      if (nOpt.has_value()) out[count++] = nOpt.value();
    }
    return count;
  }

  // how many of a ball's footprint pixels land among a pocket's pixels
  int pocketOverlap(const vectorf &ballPos, int pocketIdx) {
    PixelIndex footprint[7];
    int footprintCount = ballFootprint(ballPos, footprint);
    int overlap = 0;
    for (int f = 0; f < footprintCount; ++f) {
      for (int k = 0; k < kPocketPixelCount; ++k) {
        if (pocketPixels[pocketIdx][k] == footprint[f]) {
          overlap++;
          break;
        }
      }
    }
    return overlap;
  }

  void render() {
    const CRGB kFeltColor = CRGB(0, 60, 16);
    const CRGB kPocketColor = CRGB(90, 50, 20);
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      // subtle vignette, like a single lamp hanging over the center of the table
      vectorf rect = axial.rectFromPixelIndex(px);
      float distFrac = constrain(sqrtf(rect.x * rect.x + rect.y * rect.y) / 9.0f, 0.0f, 1.0f);
      ctx.leds[px] = kFeltColor.scale8(255 - (uint8_t)(distFrac * 90));
    }
    for (int p = 0; p < kPocketCount; ++p) {
      for (int k = 0; k < kPocketPixelCount; ++k) {
        ctx.leds[pocketPixels[p][k]] = kPocketColor;
      }
    }
    for (int b = 0; b < kBallCount; ++b) {
      if (!balls[b].inPlay) continue;
      PixelIndex footprint[7];
      int footprintCount = ballFootprint(balls[b].pos, footprint);
      for (int f = 0; f < footprintCount; ++f) {
        ctx.leds[footprint[f]] = balls[b].color;
      }
    }
  }

  void drawLocalPixel(float x, float y, CRGB color) {
    Axial ipos = axial.rectToHex(vectorf(x, y), 1.0).cubeRound();
    auto pxOpt = axial.indexAtAxial(ipos);
    if (pxOpt.has_value()) {
      ctx.leds[pxOpt.value()] = color;
    }
  }

  // small hand-drawn trophy: rim, handles, bowl, stem, base -- overlaid on
  // top of the (still-visible) table while the win pause runs
  void renderTrophy() {
    static const float pts[][2] = {
      {-2, 3}, {-1, 3}, {0, 3}, {1, 3}, {2, 3},
      {-3, 2}, {-2, 2}, {-1, 2}, {0, 2}, {1, 2}, {2, 2}, {3, 2},
      {-2, 1}, {-1, 1}, {0, 1}, {1, 1}, {2, 1},
      {-1, 0}, {0, 0}, {1, 0},
      {0, -1},
      {-1, -2}, {0, -2}, {1, -2},
      {-2, -3}, {-1, -3}, {0, -3}, {1, -3}, {2, -3},
    };
    const CRGB kTrophyColor = CRGB(0xFF, 0xD7, 0x00);
    for (auto &p : pts) {
      drawLocalPixel(p[0], p[1], kTrophyColor);
    }
  }

  // small hand-drawn sad face (frown + a tear) -- overlaid on top of the
  // table when the cue ball scratches, before it respawns at center
  void renderSadFace() {
    static const float pts[][2] = {
      {-1.5f, 1}, {1.5f, 1}, // eyes
      {-2, -1.5f}, {-1, -1.1f}, {0, -0.9f}, {1, -1.1f}, {2, -1.5f}, // frown
    };
    const CRGB kSadColor = CRGB(230, 230, 240);
    for (auto &p : pts) {
      drawLocalPixel(p[0], p[1], kSadColor);
    }
    drawLocalPixel(-1.5f, 0, CRGB(60, 140, 255)); // a little tear
  }

  // a handful of random bright pixels each frame; since render() redraws the
  // felt fresh every frame, anything not re-picked next frame just reverts,
  // giving a simple sparkling confetti effect with no extra state to track
  void renderConfetti() {
    for (int i = 0; i < kConfettiPerFrame; ++i) {
      ctx.leds[random16() % LED_COUNT] = CHSV(random8(), 0xFF, 0xFF);
    }
  }

public:
  Billiards() {
    computePockets();
    rack();
  }

  void update() {
    unsigned long nowMs = millis();
    unsigned long elapsed = (lastUpdateMs > 0) ? (nowMs - lastUpdateMs) : 16;
    elapsed = min(elapsed, 50UL); // clamp a stall (e.g. pattern switch) from launching a ball through a wall
    lastUpdateMs = nowMs;

    for (int i = 0; i < kBallCount; ++i) {
      if (!balls[i].inPlay) continue;
      Ball &b = balls[i];
      b.pos.x += b.velocity.x * elapsed;
      b.pos.y += b.velocity.y * elapsed;
      wallCollide(b.pos, b.velocity);
      float damp = max(0.0f, 1.0f - elapsed * kFrictionK);
      b.velocity.x *= damp;
      b.velocity.y *= damp;
      if (sqrtf(b.velocity.x * b.velocity.x + b.velocity.y * b.velocity.y) < kRestEpsilon) {
        b.velocity.x = 0;
        b.velocity.y = 0;
      }
    }
    resolveBallCollisions();

    for (int i = 0; i < kBallCount; ++i) {
      if (!balls[i].inPlay) continue;
      for (int p = 0; p < kPocketCount; ++p) {
        if (pocketOverlap(balls[i].pos, p) >= kPocketOverlapToPot) {
          if (i == 0) {
            balls[0].inPlay = false; // scratch: vanishes, sad face plays, then comes back out in the middle
            balls[0].velocity = vectorf(0, 0);
            scratchedSinceMs = nowMs;
          } else {
            balls[i].inPlay = false;
            balls[i].velocity = vectorf(0, 0);
          }
          break;
        }
      }
    }

    bool moving = false;
    for (int i = 0; i < kBallCount; ++i) {
      if (balls[i].inPlay && (balls[i].velocity.x != 0 || balls[i].velocity.y != 0)) {
        moving = true;
        break;
      }
    }

    bool allPotted = true;
    for (int i = 1; i < kBallCount; ++i) {
      if (balls[i].inPlay) { allPotted = false; break; }
    }
    if (allPotted) {
      if (wonSinceMs == 0) wonSinceMs = nowMs;
      else if (nowMs - wonSinceMs > kWinPauseMs) rack();
    } else {
      wonSinceMs = 0;
    }

    // Aiming: tilting the device pulls an (invisible) cue stick back in that
    // same direction, exactly like a real cue -- so the shot fires opposite
    // the tilt, not toward it. (ax,ay) here is the same pixel-rect-space
    // gravity-direction convention AnalogClock/SolarSystem/RickRoll use.
    // Holding the pull-back steady (both direction and power) for
    // kHoldStableMs is what releases the shot; drifting outside tolerance
    // just restarts the hold timer at the new reading rather than firing.
    charging = false;
    if (balls[0].inPlay && !moving && wonSinceMs == 0) {
      ICM_20948_AGMT_t agmt = MotionManager::motionFrame.agmt;
      vector32 acc(agmt.acc.axes.x, agmt.acc.axes.y, agmt.acc.axes.z);
      smoothAcc = (6 * smoothAcc + acc) / 7;
      float ax = smoothAcc.x, ay = smoothAcc.y, az = smoothAcc.z;
      float mag = sqrtf(ax * ax + ay * ay + az * az);
      float inPlaneMag = sqrtf(ax * ax + ay * ay);
      float rawPower = (mag > 1) ? constrain(inPlaneMag / mag, 0.0f, 1.0f) : 0;

      if (rawPower > kPowerDeadzone) {
        float pullBackAngle = atan2f(ay, ax);
        float power = constrain((rawPower - kPowerDeadzone) / (1.0f - kPowerDeadzone), 0.0f, 1.0f);
        charging = true;

        float angleDelta = fabsf(atan2f(sinf(pullBackAngle - chargeDirAngle), cosf(pullBackAngle - chargeDirAngle)));
        float powerDelta = fabsf(power - chargePower);
        if (stableStartMs == 0 || angleDelta > kAimAngleTolerance || powerDelta > kAimPowerTolerance) {
          chargeDirAngle = pullBackAngle;
          chargePower = power;
          stableStartMs = nowMs;
        } else {
          chargeDirAngle = pullBackAngle;
          chargePower = (chargePower * 3 + power) / 4; // absorb small hand-shake without resetting the hold timer
          if (chargePower >= kMinShotPower && nowMs - stableStartMs >= kHoldStableMs) {
            float shotAngle = chargeDirAngle + (float)M_PI; // fire opposite the pull-back
            float speed = chargePower * kMaxShotSpeed;
            balls[0].velocity = vectorf(cosf(shotAngle) * speed, sinf(shotAngle) * speed);
            charging = false;
            stableStartMs = 0;
          }
        }
      } else {
        stableStartMs = 0;
      }
    } else {
      stableStartMs = 0;
    }

    if (scratchedSinceMs != 0) {
      if (nowMs - scratchedSinceMs > kSadPauseMs) {
        balls[0].pos = vectorf(0, 0);
        balls[0].inPlay = true;
        scratchedSinceMs = 0;
      } else {
        render();
        renderSadFace();
        return;
      }
    }

    if (wonSinceMs != 0) {
      render();
      renderTrophy();
      renderConfetti();
      return;
    }

    render();
    if (charging) {
      // the cue sits behind the cue ball on the pull-back side -- the far side
      // from where it's about to travel -- like a real cue stick drawn back.
      // green tip, then a white ferrule, then a wood shaft back toward the butt.
      vectorf indicatorEnd(balls[0].pos.x + cosf(chargeDirAngle) * kIndicatorMaxLength * chargePower,
                            balls[0].pos.y + sinf(chargeDirAngle) * kIndicatorMaxLength * chargePower);
      drawIndicatorLine(balls[0].pos, indicatorEnd, [this](float distFromBall) {
        if (distFromBall < kCueTipLength) return kCueTipColor;
        if (distFromBall < kCueTipLength + kCueFerruleLength) return kCueFerruleColor;
        return kCueWoodColor;
      });
    }
  }

  const char *description() {
    return "Billiards";
  }
};

// A classic unicursal spiral labyrinth, like a nested-hexagon labyrinth
// diagram: concentric hex rings around the center alternate between corridor
// (every pixel at that ring distance is open) and wall (every pixel at that
// ring distance is a wall, except a single gap). Each wall ring's one gap
// connects the corridor ring just outside it to the one just inside it, and
// consecutive gaps are placed close to opposite sides of the hexagon, so
// getting from the outer entrance to the center means walking almost all the
// way around each ring in turn. No branches, no dead ends -- exactly the
// classic labyrinth shape, not a branching maze.
class HexMaze : public Pattern {
  static const int kMazeCount = 6;

  bool open_[LED_COUNT];
  bool visited_[LED_COUNT];
  CRGB trailColor_[LED_COUNT];

  int currentMaze = 0;
  PixelIndex tracerPos = 0;
  PixelIndex endPixel = 0;
  float rollingHue = 0;

  vector32 smoothAcc;
  unsigned long lastStepMs = 0;
  bool levelWon = false;
  unsigned long levelWonAtMs = 0;

  const float kMoveThreshold = 0.03f;          // in-plane tilt fraction below which the tracer holds still
  const unsigned long kMaxStepIntervalMs = 200; // step rate just past the threshold
  const unsigned long kMinStepIntervalMs = 45;  // step rate at full tilt
  const float kHueStepPerCell = 7.0f;
  const unsigned long kWinPauseMs = 2400;

  int ringDistance(PixelIndex px) {
    Axial a = axial.axialFromPixelIndex(px);
    return max(max(abs(a.q()), abs(a.r())), abs(a.s()));
  }

  // cells of ring k in connected walking order -- each consecutive pair,
  // including the wrap from the last cell back to the first, is a true hex
  // adjacency, so a contiguous run of this array is a contiguous wall segment
  int ringCellsInOrder(int k, PixelIndex *out) {
    if (k == 0) { out[0] = kHexaCenterIndex; return 1; }
    static const int8_t ndq[6] = {1, 1, 0, -1, -1, 0};
    static const int8_t ndr[6] = {0, -1, -1, 0, 1, 1};
    int q = ndq[4] * k, r = ndr[4] * k;
    int count = 0;
    for (int side = 0; side < 6; ++side) {
      for (int step = 0; step < k; ++step) {
        auto pxOpt = axial.indexAtAxial(q, r);
        if (pxOpt.has_value()) out[count++] = pxOpt.value();
        q += ndq[side]; r += ndr[side];
      }
    }
    return count;
  }

  void generateMaze() {
    for (int i = 0; i < LED_COUNT; ++i) { open_[i] = false; visited_[i] = false; }
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      if (ringDistance(px) % 2 == 0) open_[px] = true; // corridor rings: 0 (center), 2, 4, 6, 8
    }

    static const int8_t ndq[6] = {1, 1, 0, -1, -1, 0};
    static const int8_t ndr[6] = {0, -1, -1, 0, 1, 1};

    // protectedCells[cr] holds the cells on corridor ring cr that a gap
    // actually plugs into -- these can never be blocked below, or the spiral
    // route itself would break. Sized generously: a non-corner ring cell has
    // the usual 2 inward + 2 outward neighbors, but a *corner* cell of a ring
    // is lopsided (1 inward, 3 outward, since that's exactly where the next
    // ring out grows its extra cells) -- so a corridor ring can pick up as
    // many as 2 (inward from the wall outside it) + 3 (outward from the wall
    // inside it) = 5 protected cells if both neighboring gaps land on
    // corners. This was previously sized for 2, which silently overflowed
    // the array on the stack and was the actual cause of the crash.
    PixelIndex protectedCells[9][6];
    int protectedCount[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

    // wall rings 9,7,5,3,1, each carved open at exactly one gap pixel. Every
    // gap targets roughly the angle opposite the previous one (with a little
    // jitter for variety), forcing a near-full lap of that corridor ring
    // between consecutive gaps.
    float gapAngle = (random8() / 255.0f) * 2.0f * (float)M_PI;
    for (int ring = 9; ring >= 1; ring -= 2) {
      PixelIndex candidates[60];
      int candCount = 0;
      for (PixelIndex px = 0; px < LED_COUNT; ++px) {
        if (ringDistance(px) == ring) candidates[candCount++] = px;
      }
      float targetAngle = gapAngle + (float)M_PI + (((int)random8() - 128) / 128.0f) * 0.5f;
      PixelIndex best = candidates[0];
      float bestScore = -2;
      for (int c = 0; c < candCount; ++c) {
        vectorf rect = axial.rectFromPixelIndex(candidates[c]);
        float angle = atan2f(rect.y, rect.x);
        float score = cosf(angle - targetAngle);
        if (score > bestScore) { bestScore = score; best = candidates[c]; }
      }
      open_[best] = true;
      Axial bestAxial = axial.axialFromPixelIndex(best);
      for (int d = 0; d < 6; ++d) {
        auto nOpt = axial.indexAtAxial(bestAxial.q() + ndq[d], bestAxial.r() + ndr[d]);
        if (!nOpt.has_value()) continue;
        int nRing = ringDistance(nOpt.value());
        if ((nRing == ring - 1 || nRing == ring + 1) && protectedCount[nRing] < 6) {
          protectedCells[nRing][protectedCount[nRing]++] = nOpt.value();
        }
      }
      vectorf bestRect = axial.rectFromPixelIndex(best);
      gapAngle = atan2f(bestRect.y, bestRect.x);
      if (ring == 9) tracerPos = best; // the outermost gap is the entrance
    }
    endPixel = kHexaCenterIndex;

    // dead ends: wall off a short run of cells on each corridor ring,
    // anywhere except its 2 gap connectors, so wandering the "wrong way"
    // around a ring now genuinely dead-ends instead of always looping back
    for (int ring = 2; ring <= 8; ring += 2) {
      PixelIndex ringCells[54];
      int ringCount = ringCellsInOrder(ring, ringCells);
      for (int attempt = 0; attempt < 25; ++attempt) {
        int startIdx = random16() % ringCount;
        int runLen = 2 + random8(3); // 2..4 cells
        bool hitsProtected = false;
        for (int s = 0; s < runLen && !hitsProtected; ++s) {
          PixelIndex px = ringCells[(startIdx + s) % ringCount];
          for (int pc = 0; pc < protectedCount[ring]; ++pc) {
            if (protectedCells[ring][pc] == px) { hitsProtected = true; break; }
          }
        }
        if (hitsProtected) continue;
        for (int s = 0; s < runLen; ++s) {
          open_[ringCells[(startIdx + s) % ringCount]] = false;
        }
        break;
      }
    }
  }

  void startLevel(int levelIdx) {
    generateMaze();
    visited_[tracerPos] = true;
    trailColor_[tracerPos] = CHSV(0, 0xFF, 0xFF);
    rollingHue = 0;
    levelWon = false;
  }

  // snaps the current tilt angle to the nearest of the 6 hex directions and
  // steps the tracer one pixel that way, if (and only if) that pixel is open
  void tryStep(float angleRad) {
    Axial a = axial.axialFromPixelIndex(tracerPos);
    static const int8_t ndq[6] = {1, 1, 0, -1, -1, 0};
    static const int8_t ndr[6] = {0, -1, -1, 0, 1, 1};
    int bestDir = 0;
    float bestDot = -2;
    for (int d = 0; d < 6; ++d) {
      float dirAngle = d * (float)(M_PI / 3.0);
      float dot = cosf(angleRad - dirAngle);
      if (dot > bestDot) { bestDot = dot; bestDir = d; }
    }
    auto nOpt = axial.indexAtAxial(a.q() + ndq[bestDir], a.r() + ndr[bestDir]);
    if (nOpt.has_value() && open_[nOpt.value()]) {
      tracerPos = nOpt.value();
      if (!visited_[tracerPos]) {
        visited_[tracerPos] = true;
        trailColor_[tracerPos] = CHSV((uint8_t)rollingHue, 0xFF, 0xFF);
        rollingHue += kHueStepPerCell;
      }
    }
  }

  void drawLocalPixel(float x, float y, CRGB color) {
    Axial ipos = axial.rectToHex(vectorf(x, y), 1.0).cubeRound();
    auto pxOpt = axial.indexAtAxial(ipos);
    if (pxOpt.has_value()) {
      ctx.leds[pxOpt.value()] = color;
    }
  }

  void renderTrophy(unsigned long wonElapsedMs) {
    static const float pts[][2] = {
      {-2, 3}, {-1, 3}, {0, 3}, {1, 3}, {2, 3},
      {-3, 2}, {-2, 2}, {-1, 2}, {0, 2}, {1, 2}, {2, 2}, {3, 2},
      {-2, 1}, {-1, 1}, {0, 1}, {1, 1}, {2, 1},
      {-1, 0}, {0, 0}, {1, 0},
      {0, -1},
      {-1, -2}, {0, -2}, {1, -2},
      {-2, -3}, {-1, -3}, {0, -3}, {1, -3}, {2, -3},
    };
    const CRGB kTrophyColor = CRGB(0xFF, 0xD7, 0x00);
    const float kBouncePeriodMs = 500.0f;
    const float kBounceAmplitude = 1.6f;
    // touches down and springs back up on a beat, like a real bounce, rather
    // than a smooth symmetric float
    float bounceY = fabsf(sinf((wonElapsedMs / kBouncePeriodMs) * (float)M_PI)) * kBounceAmplitude;
    for (auto &p : pts) {
      drawLocalPixel(p[0], p[1] + bounceY, kTrophyColor);
    }
  }

  // a handful of random bright pixels each frame; since render() redraws the
  // maze fresh every frame, anything not re-picked next frame just reverts,
  // giving a simple sparkling confetti effect with no extra state to track
  void renderConfetti() {
    const int kConfettiPerFrame = 14;
    for (int i = 0; i < kConfettiPerFrame; ++i) {
      ctx.leds[random16() % LED_COUNT] = CHSV(random8(), 0xFF, 0xFF);
    }
  }

  void render() {
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      if (!open_[px]) {
        ctx.leds[px] = CRGB::White;
      } else if (visited_[px]) {
        ctx.leds[px] = trailColor_[px];
      } else {
        ctx.leds[px] = CRGB::Black;
      }
    }
    ctx.leds[endPixel] = CRGB(20, 200, 60); // goal, always visible
    // the runner is its own rainbow trail color, blinked off every other
    // beat so it still reads as "the current position" against the trail
    bool flashOn = ((millis() / 220) % 2) == 0;
    ctx.leds[tracerPos] = flashOn ? trailColor_[tracerPos] : CRGB::Black;
  }

public:
  HexMaze() {
    startLevel(0);
  }

  void update() {
    unsigned long nowMs = millis();

    if (!levelWon) {
      ICM_20948_AGMT_t agmt = MotionManager::motionFrame.agmt;
      vector32 acc(agmt.acc.axes.x, agmt.acc.axes.y, agmt.acc.axes.z);
      smoothAcc = (6 * smoothAcc + acc) / 7;
      float ax = smoothAcc.x, ay = smoothAcc.y, az = smoothAcc.z;
      float mag = sqrtf(ax * ax + ay * ay + az * az);
      float inPlaneMag = sqrtf(ax * ax + ay * ay);
      float tiltFrac = (mag > 1) ? constrain(inPlaneMag / mag, 0.0f, 1.0f) : 0;

      if (tiltFrac > kMoveThreshold) {
        float t = constrain((tiltFrac - kMoveThreshold) / (1.0f - kMoveThreshold), 0.0f, 1.0f);
        unsigned long interval = kMaxStepIntervalMs - (unsigned long)((kMaxStepIntervalMs - kMinStepIntervalMs) * t);
        if (nowMs - lastStepMs >= interval) {
          tryStep(atan2f(ay, ax));
          lastStepMs = nowMs;
        }
      }

      if (tracerPos == endPixel) {
        levelWon = true;
        levelWonAtMs = nowMs;
      }
    }

    render();
    if (levelWon) {
      ctx.leds.fadeToBlackBy(200); // dim the finished maze so the trophy pops
      renderTrophy(nowMs - levelWonAtMs);
      renderConfetti();
      if (nowMs - levelWonAtMs > kWinPauseMs) {
        currentMaze = (currentMaze + 1) % kMazeCount;
        startLevel(currentMaze);
      }
    }
  }

  const char *description() {
    return "HexMaze";
  }
};

// A workout countdown timer keyed to 3 of the hexa's edges: face 8 o'clock is
// 30s, face 6 o'clock is 60s, face 4 o'clock is 90s (same edges and same
// theta(hour) angle convention SolarSystem uses, reused verbatim). Every time
// the hexa is (re)placed on a face -- even the same one it just left -- that
// face's timer restarts fresh from its full duration; it does not pause and
// resume. Because the 3 faces are genuinely different physical rotations of
// the board (not just different tilt amounts), the number and the GOOD/JOB
// banner are drawn through a fixed per-face rotation so they always read
// upright to whoever set the hexa down, using the same rotate-canonical-
// content-by-delta math RickRoll uses to stay upright on any of the 6 edges
// (there it rotates the *sample*, backward; here it rotates freshly-authored
// content, forward -- inverse directions of the same problem).
class WorkoutTimer : public Pattern {
  static const int kFaceCount = 3; // 0 = face 8 (30s), 1 = face 6 (60s), 2 = face 4 (90s)
  const unsigned long kDurationMs[kFaceCount] = {30000, 60000, 90000};

  // hardware-calibrated rotation constant, reused from AnalogClock/SolarSystem
  const float kClockRotationOffset = M_PI;
  const float kFace8Angle = M_PI/2 - 8*(M_PI/6) + kClockRotationOffset;
  const float kFace6Angle = M_PI/2 - 6*(M_PI/6) + kClockRotationOffset;
  const float kFace4Angle = M_PI/2 - 4*(M_PI/6) + kClockRotationOffset;

  // one color per face, reused for that face's idle arrow AND its 30/60/90 label
  const CRGB kFaceColor8 = CRGB(255, 140, 40);  // 30s -- warm orange
  const CRGB kFaceColor6 = CRGB(80, 210, 255);  // 60s -- cyan
  const CRGB kFaceColor4 = CRGB(150, 255, 110); // 90s -- green

  const float kFlatnessThreshold = 0.7f;  // |az|/mag above this: lying flat, not on any face
  const float kFaceConfidence = 0.5f;     // how strongly gravity must align with a face direction
  const float kFaceMargin = 0.05f;        // required lead over the second-best face, to avoid flicker

  // same front/back "tiltFrontBack = az/mag" identifier and the same
  // threshold/hold constants as AnalogClock's own reset gesture
  const float kResetTiltThreshold = 0.85f;
  const unsigned long kResetHoldMs = 1000;

  const unsigned long kCelebrationPhraseMs = 5000; // one random affirmation, shown once
  const unsigned long kCelebrationResetMs = 1800;  // then "RESET" scrolls once
  const unsigned long kCelebrationArrowMs = 2200;  // then a fade to an up-arrow, held
  const unsigned long kCelebrationFadeMs = 600;     // crossfade length at each seam

  static const int kCelebrationPhraseCount = 5;
  const char *kCelebrationPhrases[kCelebrationPhraseCount] = { "GOOD JOB", "YOU DID IT", "ALL DONE", "FINISHED", "NICE WORK" };
  const int kCelebrationPhraseLens[kCelebrationPhraseCount] = { 8, 10, 8, 8, 9 };

  enum Phase { kIdle, kRunning, kCelebrating };
  int currentFace = -1; // which of the 3 faces is currently under the hexa, -1 if none
  int lastUsedFace = 1; // most recent real face (0/1/2); defaults to 60s (index 1) until one's actually used
  Phase phase = kIdle;
  float remainingMs = 0;
  unsigned long celebrationStepStartMs = 0;
  int celebrationPhraseIdx = 0;
  float faceRotationAngle[kFaceCount]; // radians, for the progress ring's rect-space "12 o'clock" math
  int faceRotationSteps[kFaceCount];   // same rotation as exact integer 60deg steps, for the hex-native font

  int lastDisplaySeconds = -1;        // whichever whole second is currently shown
  unsigned long secondFlashStartMs = 0; // when it last changed -- anchors the per-second flash/dim

  vector32 smoothAcc;
  unsigned long lastUpdateMs = 0;
  unsigned long resetHoldStart = 0;
  bool resetFired = false;

  // how far to rotate freshly-authored local content so it reads upright when
  // resting on a face whose gravity direction (in pixel-rect-space) is
  // faceAngle -- same derivation RickRoll uses, just solved in the opposite
  // (forward, not backward) direction, and evaluated at a fixed known angle
  // instead of live sensor data since these 3 faces are already known exactly
  int computeFaceRotationSteps(float faceAngle) {
    float delta = faceAngle + (float)M_PI / 2.0f;
    int steps = (int)roundf(delta / (float)(M_PI / 3.0));
    return ((steps % 6) + 6) % 6;
  }

  // Medium-font countdown: a single Medium digit already spans nearly the
  // whole board (radius 6 of a radius-9 hex), so there's no room to kern two
  // of them apart without overlap the way the old Small-font version did.
  // Verified against every one of the 100 tens/ones digit pairs: shifting
  // each digit 3 cells off-center (toward its own side) is the largest
  // symmetric offset that keeps BOTH digits' own pixels within the true
  // board -- anything past that clips real ink off the edge. The two digits'
  // footprints do overlap in the middle at this offset; that's expected, not
  // a bug -- it's the tradeoff for a font this big on a screen this size.
  // single digit (1-9): the whole board to itself, no kerning question at all
  void drawSingleLargeDigit(int value, int rotSteps, CRGB color) {
    drawHexBitmaskSteps(ctx, hexBitmaskDigitLG(value), kHexCellQR_LG, 271, 0, 0, rotSteps, color);
  }

  void drawTwoMediumDigits(int value, int rotSteps, CRGB color) {
    const int kOverlapOffset = 3;
    int tens = (value / 10) % 10, ones = value % 10;
    drawHexBitmaskSteps(ctx, hexBitmaskDigitMD(tens), kHexCellQR_MD, 127, -kOverlapOffset, 0, rotSteps, color);
    drawHexBitmaskSteps(ctx, hexBitmaskDigitMD(ones), kHexCellQR_MD, 127, kOverlapOffset, 0, rotSteps, color);
  }

  // two digits (10-99): same ink-bounds-based kerning, tuned to exactly 1
  // pixel of overlap between the two digits' outer edges -- verified against
  // all 100 tens/ones pairs (kKerningGap=-1.5 was actually landing on a
  // 0-pixel touch, not a real overlap; -2.0 is the value that actually
  // produces -1).
  void drawTwoSmallDigits(int value, int rotSteps, CRGB color) {
    const float kKerningGap = -2.0f;
    int tens = (value / 10) % 10, ones = value % 10;
    const uint8_t *tensMask = hexBitmaskDigitXS(tens);
    const uint8_t *onesMask = hexBitmaskDigitXS(ones);
    int tensMinQ, tensMaxQ, onesMinQ, onesMaxQ;
    hexBitmaskQBounds(tensMask, kHexCellQR_XS, 61, tensMinQ, tensMaxQ);
    hexBitmaskQBounds(onesMask, kHexCellQR_XS, 61, onesMinQ, onesMaxQ);
    float half = (kKerningGap + 1.0f) / 2.0f;
    int tensOrigin = (int)roundf(-half - tensMaxQ);
    int onesOrigin = (int)roundf(half - onesMinQ);
    drawHexBitmaskSteps(ctx, tensMask, kHexCellQR_XS, 61, tensOrigin, 0, rotSteps, color);
    drawHexBitmaskSteps(ctx, onesMask, kHexCellQR_XS, 61, onesOrigin, 0, rotSteps, color);
  }

  // "GO?" sits still (it's short enough to fit at once, unlike the
  // affirmation phrases), oriented so it reads upright with the 60-second
  // face at the bottom -- the same rotation that face's own countdown uses.
  // A slow brightness pulse plus an occasional single-cell rise give it a
  // little life without turning it into a full scroll/animation. Oriented
  // toward whichever face was actually used most recently (lastUsedFace),
  // not a fixed face -- defaults to the 60s face until one's been used.
  void renderIdleWord(unsigned long nowMs) {
    const char *word = "GO?";
    float width = bitmaskWordWidth(word, 3);
    uint8_t b = beatsin8(24, 130, 255);
    int bobR = (beatsin8(10) > 180) ? 1 : 0;
    drawBitmaskWordAt(ctx, word, 3, -width / 2.0f, bobR, faceRotationSteps[lastUsedFace], CRGB(b, b, b));
  }

  // a short radial mark from mid-radius to the true edge, pointing straight
  // at faceAngle -- each of the 3 valid faces gets its own color instead of
  // the old dot-count arcs, so which edge is which reads by color alone
  void drawFaceIndicator(float faceAngle, CRGB color) {
    for (float d = 5.5f; d <= 7.5f; d += 1.0f) {
      vectorf p(cosf(faceAngle) * d, sinf(faceAngle) * d);
      Axial ipos = axial.rectToHex(p, 1.0).cubeRound();
      auto pxOpt = axial.indexAtAxial(ipos);
      if (pxOpt.has_value()) ctx.leds[pxOpt.value()] = color;
    }
    vectorf tipRect(cosf(faceAngle) * 8.3f, sinf(faceAngle) * 8.3f);
    Axial tipAxial = axial.rectToHex(tipRect, 1.0).cubeRound();
    auto tipOpt = axial.indexAtAxial(tipAxial);
    if (tipOpt.has_value()) {
      ctx.leds[tipOpt.value()] = color;
    }
  }

  // dim red, always waiting for a face -- a small colored arrow at each of
  // the 3 valid edges is always on screen (a different color per face), and
  // the center cycles between "GO?" and each face's own duration (30/60/90,
  // in that face's color, oriented to that face) -- a full 2-digit number at
  // Small size uses most of the board's own width, so all three can't sit on
  // screen at once without clipping the true edge or piling on top of each
  // other; verified that both problems are real for every offset tried, so
  // this cycles them one at a time instead of forcing a layout that doesn't
  // fit.
  void renderIdle(unsigned long nowMs) {
    ctx.leds.fill_solid(CRGB(50, 8, 8));
    drawFaceIndicator(kFace8Angle, kFaceColor8);
    drawFaceIndicator(kFace6Angle, kFaceColor6);
    drawFaceIndicator(kFace4Angle, kFaceColor4);

    const unsigned long kIdleGoMs = 2500;
    const unsigned long kIdleNumberMs = 1400;
    const unsigned long kIdleCycleMs = kIdleGoMs + 3 * kIdleNumberMs;
    unsigned long cyclePos = nowMs % kIdleCycleMs;
    if (cyclePos < kIdleGoMs) {
      renderIdleWord(nowMs);
    } else {
      int slot = (int)((cyclePos - kIdleGoMs) / kIdleNumberMs); // 0=30s, 1=60s, 2=90s
      uint8_t b = beatsin8(30, 170, 255);
      CRGB col = (slot == 0) ? kFaceColor8 : (slot == 1) ? kFaceColor6 : kFaceColor4;
      col.nscale8(b);
      int value = (slot == 0) ? 30 : (slot == 1) ? 60 : 90;
      drawTwoSmallDigits(value, faceRotationSteps[slot], col);
    }
  }

  // per-face background: same red->yellow->green progress hue everywhere,
  // but each face's gradient shape is different so which timer is running is
  // identifiable at a glance even before reading the number, and the whole
  // background pulses gently in time with the number's per-second flash
  void renderTimerBackground(int face, uint8_t hue, uint8_t sat, float brightnessFrac) {
    const float kBgBaseV = 150.0f;
    float bgFlash = 0.7f + 0.3f * brightnessFrac;
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      vectorf rect = axial.rectFromPixelIndex(px);
      float dist = sqrtf(rect.x * rect.x + rect.y * rect.y) / 9.0f;
      float gradMul;
      if (face == 0) {
        gradMul = 1.15f - 0.45f * dist; // 30s: bright center, dimmer toward the edge
      } else if (face == 1) {
        gradMul = 0.85f + 0.30f * (rect.y / 9.0f); // 60s: brighter toward the top
      } else {
        gradMul = 0.55f + 0.55f * dist; // 90s: dim center, brighter ring toward the edge
      }
      float v = kBgBaseV * gradMul * bgFlash;
      ctx.leds[px] = CHSV(hue, sat, (uint8_t)constrain(v, 15.0f, 255.0f));
    }
  }

  // same outer-ring fill technique ChargingPattern uses for its battery
  // indicator (a fraction of the perimeter lit solid, plus a pulsing pixel at
  // the leading edge), extended with: a dim full-circumference track so the
  // ring's shape is always visible; a start point at true 12 o'clock for
  // whichever face's rotation is active, filling clockwise from there (the
  // shell's own natural order runs counterclockwise, so it's walked backward);
  // and a tick mark every 10 seconds of this face's duration.
  void drawProgressRing(float progress, float rotAngle, unsigned long durationMs, CRGB color) {
    HexaShells shells;
    auto &outerShell = shells.shells.back();
    int total = outerShell.size();

    float upAngle = (float)M_PI / 2.0f + rotAngle;
    int startIdx = 0;
    float bestScore = -2;
    for (int i = 0; i < total; ++i) {
      vectorf rect = axial.rectFromPixelIndex(outerShell[i].value());
      float score = cosf(atan2f(rect.y, rect.x) - upAngle);
      if (score > bestScore) { bestScore = score; startIdx = i; }
    }

    CRGB track = color.scale8(24);
    for (int i = 0; i < total; ++i) {
      ctx.leds[outerShell[i].value()] = track;
    }

    int displayLength = (int)(progress * total);
    for (int i = 0; i < displayLength; ++i) {
      int idx = ((startIdx - i) % total + total) % total;
      ctx.leds[outerShell[idx].value()] = color;
    }
    if (displayLength < total) {
      int idx = ((startIdx - displayLength) % total + total) % total;
      ctx.leds[outerShell[idx].value()] = color.scale8(beatsin8(30));
    }

    CRGB tickColor = CRGB(160, 160, 160);
    for (unsigned long t = 10000; t < durationMs; t += 10000) {
      int idx = ((startIdx - (int)((float)t / durationMs * total)) % total + total) % total;
      ctx.leds[outerShell[idx].value()] = tickColor;
    }
  }

  void render(unsigned long nowMs) {
    if (phase == kIdle || currentFace < 0) {
      renderIdle(nowMs);
      return;
    }

    float total = kDurationMs[currentFace] / 1000.0f;
    float remaining = remainingMs / 1000.0f;
    float progress = constrain(1.0f - remaining / total, 0.0f, 1.0f);
    bool isFinalCountdown = (phase == kRunning) && remaining <= 5.0f;

    uint8_t hue = (uint8_t)(85 * progress); // red -> yellow -> green as progress rises

    int displaySeconds = (int)ceilf(remaining);
    if (displaySeconds != lastDisplaySeconds) {
      lastDisplaySeconds = displaySeconds;
      secondFlashStartMs = nowMs;
    }
    // each new second flashes the number in at full brightness, then dims
    // across that second -- a gentle dim normally, more pronounced (though
    // not all the way to black) once inside the final 5 seconds
    float intoSecond = constrain((nowMs - secondFlashStartMs) / 1000.0f, 0.0f, 1.0f);
    float minBrightnessFrac = isFinalCountdown ? 0.65f : 0.85f;
    float fade = 1.0f - intoSecond;
    float brightnessFrac = minBrightnessFrac + (1.0f - minBrightnessFrac) * fade * fade;
    uint8_t sat = isFinalCountdown ? 0 : 0xFF; // white instead of the red->green hue for the last 5 seconds
    CRGB numberColor = CHSV(hue, sat, (uint8_t)(255 * brightnessFrac));
    CRGB ringColor = CHSV(hue, sat, (uint8_t)(220 * brightnessFrac)); // now flashes with the number

    // background: always on (every phase), a per-face gradient shape so the
    // 3 timers are visually distinct at a glance, and it pulses gently along
    // with the same per-second beat as the number
    renderTimerBackground(currentFace, hue, sat, brightnessFrac);

    float rotAngle = faceRotationAngle[currentFace];
    if (phase == kCelebrating) {
      // one random affirmation (kCelebrationPhraseMs), then "RESET" scrolls
      // once (kCelebrationResetMs), then a pulsing up-arrow (kElement_ARROW_UP,
      // kCelebrationArrowMs) -- each fades into the next. "RESET" alone is
      // already close to the board's full width at Small size (verified: a
      // 5-letter word needs nearly the whole 18-cell diameter), so it can't
      // share the screen with the arrow at the same time without clipping or
      // burying one under the other; this sequences them tightly instead.
      unsigned long celebElapsed = nowMs - celebrationStepStartMs;
      if (celebElapsed < kCelebrationPhraseMs) {
        uint8_t b = beatsin8(90, 150, 255);
        unsigned long remainingPhraseMs = kCelebrationPhraseMs - celebElapsed;
        uint8_t fadeOut = (remainingPhraseMs < kCelebrationFadeMs) ? (uint8_t)(255UL * remainingPhraseMs / kCelebrationFadeMs) : 255;
        CRGB col = CRGB(b, b, b);
        col.nscale8(fadeOut);
        drawScrollingBitmaskWord(ctx, kCelebrationPhrases[celebrationPhraseIdx], kCelebrationPhraseLens[celebrationPhraseIdx], (float)celebElapsed, (float)kCelebrationPhraseMs, faceRotationSteps[currentFace], col);
      } else if (celebElapsed < kCelebrationPhraseMs + kCelebrationResetMs) {
        unsigned long resetElapsed = celebElapsed - kCelebrationPhraseMs;
        unsigned long remainingResetMs = kCelebrationResetMs - resetElapsed;
        uint8_t fadeIn = (resetElapsed < kCelebrationFadeMs) ? (uint8_t)(255UL * resetElapsed / kCelebrationFadeMs) : 255;
        uint8_t fadeOut = (remainingResetMs < kCelebrationFadeMs) ? (uint8_t)(255UL * remainingResetMs / kCelebrationFadeMs) : 255;
        uint8_t b = beatsin8(90, 150, 255);
        CRGB col = CRGB(b, b, b);
        col.nscale8(min(fadeIn, fadeOut));
        drawScrollingBitmaskWord(ctx, "RESET", 5, (float)resetElapsed, (float)kCelebrationResetMs, faceRotationSteps[currentFace], col);
      } else {
        unsigned long arrowElapsed = celebElapsed - kCelebrationPhraseMs - kCelebrationResetMs;
        uint8_t fadeIn = (arrowElapsed < kCelebrationFadeMs) ? (uint8_t)(255UL * arrowElapsed / kCelebrationFadeMs) : 255;
        uint8_t pulse = beatsin8(20, 150, 255);
        CRGB col = CRGB(pulse, pulse, pulse);
        col.nscale8(fadeIn);
        drawHexBitmaskSteps(ctx, kElement_ARROW_UP, kHexCellQR_LG, 271, 0, 0, faceRotationSteps[currentFace], col);
      }
    } else if (displaySeconds >= 1 && displaySeconds <= 9) {
      drawSingleLargeDigit(displaySeconds, faceRotationSteps[currentFace], numberColor);
    } else {
      drawTwoSmallDigits(displaySeconds, faceRotationSteps[currentFace], numberColor);
    }
    drawProgressRing(progress, rotAngle, kDurationMs[currentFace], ringColor);
  }

public:
  WorkoutTimer() {
    faceRotationSteps[0] = computeFaceRotationSteps(kFace8Angle);
    faceRotationSteps[1] = computeFaceRotationSteps(kFace6Angle);
    faceRotationSteps[2] = computeFaceRotationSteps(kFace4Angle);
    for (int i = 0; i < kFaceCount; ++i) faceRotationAngle[i] = faceRotationSteps[i] * (float)(M_PI / 3.0);
  }

  void update() {
    unsigned long nowMs = millis();
    unsigned long elapsedMs = (lastUpdateMs > 0) ? (nowMs - lastUpdateMs) : 16;
    elapsedMs = min(elapsedMs, 200UL);
    lastUpdateMs = nowMs;

    ICM_20948_AGMT_t agmt = MotionManager::motionFrame.agmt;
    vector32 acc(agmt.acc.axes.x, agmt.acc.axes.y, agmt.acc.axes.z);
    smoothAcc = (10 * smoothAcc + acc) / 11;
    float ax = smoothAcc.x, ay = smoothAcc.y, az = smoothAcc.z;
    float mag = sqrtf(ax * ax + ay * ay + az * az);
    float flatness = (mag > 1) ? fabsf(az) / mag : 1.0f;

    int detectedFace = -1;
    if (mag > 1 && flatness < kFlatnessThreshold) {
      float dot8 = (ax * cosf(kFace8Angle) + ay * sinf(kFace8Angle)) / mag;
      float dot6 = (ax * cosf(kFace6Angle) + ay * sinf(kFace6Angle)) / mag;
      float dot4 = (ax * cosf(kFace4Angle) + ay * sinf(kFace4Angle)) / mag;
      float vals[3] = {dot8, dot6, dot4};
      int best = 0;
      for (int i = 1; i < 3; ++i) if (vals[i] > vals[best]) best = i;
      float second = -1e9f;
      for (int i = 0; i < 3; ++i) if (i != best && vals[i] > second) second = vals[i];
      if (vals[best] > kFaceConfidence && vals[best] - second > kFaceMargin) {
        detectedFace = best;
      }
    }

    // every (re)placement on a face -- even back onto the same one -- is
    // treated as a fresh start, not a resume
    if (detectedFace != currentFace) {
      currentFace = detectedFace;
      if (currentFace >= 0) {
        remainingMs = kDurationMs[currentFace];
        phase = kRunning;
        lastDisplaySeconds = -1;
        lastUsedFace = currentFace; // idle screen orients toward whichever was used most recently
      } else {
        phase = kIdle;
      }
      resetHoldStart = 0;
      resetFired = false;
    }

    // tilting the standing hexa forward or backward, held briefly -- the same
    // az/mag "front/back tilt" identifier and threshold/hold as AnalogClock's
    // own reset gesture. That axis is the board's face-normal, so its meaning
    // ("tipping toward/away from the viewer") is the same regardless of which
    // of the 3 faces is down -- only the in-plane rotation (what
    // faceRotationAngle corrects) actually differs per face.
    if (currentFace >= 0) {
      float tiltFrontBack = (mag > 1) ? az / mag : 0;
      if (fabsf(tiltFrontBack) > kResetTiltThreshold) {
        if (resetHoldStart == 0) {
          resetHoldStart = nowMs;
        } else if (!resetFired && nowMs - resetHoldStart >= kResetHoldMs) {
          remainingMs = kDurationMs[currentFace];
          phase = kRunning;
          lastDisplaySeconds = -1;
          resetFired = true;
        }
      } else {
        resetHoldStart = 0;
        resetFired = false;
      }
    }

    if (phase == kRunning) {
      remainingMs -= elapsedMs;
      if (remainingMs <= 0) {
        remainingMs = 0;
        phase = kCelebrating;
        celebrationStepStartMs = nowMs;
        celebrationPhraseIdx = random8(kCelebrationPhraseCount);
      }
    } else if (phase == kCelebrating) {
      if (nowMs - celebrationStepStartMs >= kCelebrationPhraseMs + kCelebrationResetMs + kCelebrationArrowMs) {
        remainingMs = kDurationMs[currentFace];
        phase = kRunning; // "reset timer to start" once the phrase+arrow sequence finishes
        lastDisplaySeconds = -1;
      }
    }

    render(nowMs);
  }

  const char *description() {
    return "WorkoutTimer";
  }
};

// A shake-to-decide pattern with 3 modes sharing one gesture language: a
// genuine physical shake (several real acceleration spikes in a short
// window, not just one bump) rolls a fresh answer in whichever mode is
// active; tilting the hexa onto its side and holding cycles to the next
// mode. Results persist (like a real magic 8-ball) until the next shake.
// 8-ball and coin answers are shown as color + a simple symbol rather than
// spelled-out phrases -- the board doesn't have a full alphabet designed in
// the Hexa Font Forge tool yet, so full classic 8-ball text isn't practical
// here; dice needs no letters at all, just real pip layouts.
class DecisionMaker : public Pattern {
  enum Mode { kMode8Ball, kModeCoin, kModeDice, kModeCount };
  enum Phase { kWaiting, kRolling, kSettled };

  const unsigned long kRollDurationMs = 900;   // how long the shuffle/flicker runs before settling
  const unsigned long kFlickerIntervalMs = 90; // how often the preview changes during the roll

  const float kSpikeRatio = 1.35f;        // raw reading must exceed smoothed baseline by this ratio to count as a spike
  const int kShakeSpikesNeeded = 4;       // this many spikes inside kShakeWindowMs = a genuine shake, not one bump
  const unsigned long kShakeWindowMs = 500;
  const unsigned long kShakeCooldownMs = 1200; // minimum gap between triggered shakes

  const float kModeSwitchTiltThreshold = 0.6f; // in-plane tilt fraction, either direction
  const unsigned long kModeSwitchHoldMs = 700;

  Mode mode = kMode8Ball;
  Phase phase = kWaiting;
  unsigned long phaseStartMs = 0;
  unsigned long lastFlickerMs = 0;
  int previewValue = 0;
  int settledValue = 0; // meaning depends on mode: 8ball 0=no/1=yes/2=unclear, coin 0=dont/1=doit, dice 0..5 (+1 shown)

  vector32 smoothAcc;
  int spikeCount = 0;
  unsigned long lastSpikeMs = 0;
  unsigned long lastShakeTriggerMs = 0;
  unsigned long modeSwitchHoldStart = 0;
  bool modeSwitchFired = false;

  void drawLocalPixel(float x, float y, CRGB color) {
    Axial ipos = axial.rectToHex(vectorf(x, y), 1.0).cubeRound();
    auto pxOpt = axial.indexAtAxial(ipos);
    if (pxOpt.has_value()) {
      ctx.leds[pxOpt.value()] = color;
    }
  }

  void drawDisc(float radius, CRGB color) {
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      vectorf rect = axial.rectFromPixelIndex(px);
      if (sqrtf(rect.x * rect.x + rect.y * rect.y) <= radius) {
        ctx.leds[px] = color;
      }
    }
  }

  void drawCheckmark(CRGB color) {
    static const float pts[][2] = {
      {-2.5f, 0.5f}, {-1.5f, -0.5f}, {-0.5f, -1.5f}, {0.2f, -0.8f}, {1.2f, 0.5f}, {2.2f, 2.0f},
    };
    for (auto &p : pts) drawLocalPixel(p[0], p[1], color);
  }

  void drawX(CRGB color) {
    static const float pts[][2] = {
      {-2, 2}, {-1, 1}, {0, 0}, {1, -1}, {2, -2}, {-2, -2}, {-1, -1}, {1, 1}, {2, 2},
    };
    for (auto &p : pts) drawLocalPixel(p[0], p[1], color);
  }

  // classic 6-sided die pip layouts, real dot arrangements not just a number
  void drawDiceFace(int value, CRGB color) {
    const float N = -1.7f, P = 1.7f; // near/far pip position on each axis
    switch (value) {
      case 1:
        drawLocalPixel(0, 0, color);
        break;
      case 2:
        drawLocalPixel(N, P, color); drawLocalPixel(P, N, color);
        break;
      case 3:
        drawLocalPixel(N, P, color); drawLocalPixel(0, 0, color); drawLocalPixel(P, N, color);
        break;
      case 4:
        drawLocalPixel(N, P, color); drawLocalPixel(P, P, color);
        drawLocalPixel(N, N, color); drawLocalPixel(P, N, color);
        break;
      case 5:
        drawLocalPixel(N, P, color); drawLocalPixel(P, P, color);
        drawLocalPixel(0, 0, color);
        drawLocalPixel(N, N, color); drawLocalPixel(P, N, color);
        break;
      default: // 6
        drawLocalPixel(N, P, color); drawLocalPixel(P, P, color);
        drawLocalPixel(N, 0, color); drawLocalPixel(P, 0, color);
        drawLocalPixel(N, N, color); drawLocalPixel(P, N, color);
        break;
    }
  }

  void renderWaiting() {
    if (mode == kMode8Ball) {
      ctx.leds.fill_solid(CRGB(6, 4, 20));
      drawHexGlyphSteps(ctx, hexFontGlyphForChar('8'), 0, 0, 0, CRGB(225, 225, 250));
    } else if (mode == kModeCoin) {
      ctx.leds.fill_solid(CRGB(10, 10, 10));
      drawDisc(6.0f, CRGB(90, 90, 95));
    } else {
      ctx.leds.fill_solid(CRGB(8, 8, 12));
    }
  }

  void renderOutcome(int value) {
    if (mode == kMode8Ball) {
      CRGB bg, fg;
      if (value == 0) { bg = CRGB(24, 4, 4); fg = CRGB(255, 60, 60); }
      else if (value == 1) { bg = CRGB(4, 22, 8); fg = CRGB(60, 230, 110); }
      else { bg = CRGB(26, 18, 2); fg = CRGB(255, 190, 40); }
      ctx.leds.fill_solid(bg);
      if (value == 0) drawX(fg);
      else if (value == 1) drawCheckmark(fg);
      else drawHexGlyphSteps(ctx, hexFontGlyphForChar('?'), 0, 0, 0, fg);
    } else if (mode == kModeCoin) {
      ctx.leds.fill_solid(CRGB(12, 12, 14));
      CRGB c = value ? CRGB(40, 210, 80) : CRGB(220, 40, 40);
      drawDisc(6.0f, c);
    } else {
      ctx.leds.fill_solid(CRGB(8, 8, 12));
      drawDiceFace(value + 1, CRGB(235, 235, 245));
    }
  }

  int randomValueForMode() {
    if (mode == kMode8Ball) return random8(3);
    if (mode == kModeCoin) return random8(2);
    return random8(6);
  }

  void triggerShake(unsigned long nowMs) {
    phase = kRolling;
    phaseStartMs = nowMs;
    lastFlickerMs = 0;
  }

public:
  void update() {
    unsigned long nowMs = millis();

    ICM_20948_AGMT_t agmt = MotionManager::motionFrame.agmt;
    vector32 acc(agmt.acc.axes.x, agmt.acc.axes.y, agmt.acc.axes.z);
    float rawMag = sqrtf((float)acc.x * acc.x + (float)acc.y * acc.y + (float)acc.z * acc.z);
    smoothAcc = (8 * smoothAcc + acc) / 9;
    float ax = smoothAcc.x, ay = smoothAcc.y;
    float smoothMag = sqrtf((float)smoothAcc.x * smoothAcc.x + (float)smoothAcc.y * smoothAcc.y + (float)smoothAcc.z * smoothAcc.z);

    // a shake is several real spikes above the smoothed baseline arriving in
    // a short window -- one bump alone (a single spike) doesn't count, which
    // is what keeps this from firing on an ordinary tilt or a single tap
    if (smoothMag > 1 && rawMag > smoothMag * kSpikeRatio) {
      if (nowMs - lastSpikeMs > kShakeWindowMs) spikeCount = 0;
      spikeCount++;
      lastSpikeMs = nowMs;
      if (spikeCount >= kShakeSpikesNeeded && nowMs - lastShakeTriggerMs > kShakeCooldownMs) {
        triggerShake(nowMs);
        spikeCount = 0;
        lastShakeTriggerMs = nowMs;
      }
    }

    // tilting the hexa onto its side and holding cycles to the next mode --
    // works regardless of which way it's tilted, there's no "previous"
    if (smoothMag > 1 && fabsf(ax) / smoothMag > kModeSwitchTiltThreshold) {
      if (modeSwitchHoldStart == 0) {
        modeSwitchHoldStart = nowMs;
      } else if (!modeSwitchFired && nowMs - modeSwitchHoldStart >= kModeSwitchHoldMs) {
        mode = (Mode)((mode + 1) % kModeCount);
        phase = kWaiting;
        modeSwitchFired = true;
      }
    } else {
      modeSwitchHoldStart = 0;
      modeSwitchFired = false;
    }
    (void)ay;

    if (phase == kRolling) {
      if (nowMs - lastFlickerMs > kFlickerIntervalMs) {
        lastFlickerMs = nowMs;
        previewValue = randomValueForMode();
      }
      if (nowMs - phaseStartMs > kRollDurationMs) {
        settledValue = randomValueForMode();
        phase = kSettled;
      }
    }

    if (phase == kWaiting) renderWaiting();
    else if (phase == kRolling) renderOutcome(previewValue);
    else renderOutcome(settledValue);
  }

  const char *description() {
    return "DecisionMaker";
  }
};

// Breakout: tilt-controlled paddle, hex-shaped brick field, real ball
// physics off the board's own 6 walls (the exact wall-line reflection
// LargeBouncyBall/Billiards already use), brick explosions, a tap-to-fire
// laser that cuts a whole column of bricks, and a high score persisted to
// flash so it survives a power cycle.
class Breakout : public Pattern {
  static const int kBrickCount = 21;
  struct Brick {
    float x, y;
    bool alive;
    uint8_t hue;
  };
  Brick bricks[kBrickCount];

  const float kBallRadius = 0.45f;
  const float kPaddleY = -5.6f;
  const float kPaddleHalfWidth = 1.35f;
  const float kBrickHalfWidth = 0.85f;
  const float kBrickHalfHeight = 0.42f;
  const float kBallSpeed = 0.0072f;       // units/ms
  const float kMaxBounceSteer = 0.85f;    // how much paddle hit-offset can redirect the ball
  const int kStartLives = 3;
  const int kEepromAddr = 100;

  const float kSpikeRatio = 1.35f;
  const int kShakeSpikesNeeded = 4;
  const unsigned long kShakeWindowMs = 500;
  const unsigned long kShakeCooldownMs = 1000;

  const float kTapMinimumMagnitude = 8000.0f; // same proven floor as AnalogClock's tap gesture
  const float kTapMagnitudeRatio = 1.4f;
  const unsigned long kTapCooldownMs = 300;

  enum GameState { kPlaying, kBallLost, kGameOver };
  GameState state = kPlaying;

  vectorf ballPos, ballVel;
  float paddleX = 0;
  int lives = kStartLives;
  int score = 0;
  int highScore = 0;
  bool newHighScore = false;
  int laserCharge = 0;

  unsigned long lastUpdateMs = 0;
  unsigned long stateChangedAtMs = 0;

  struct Explosion { float x, y; unsigned long startMs; bool active; };
  static const int kMaxExplosions = 8;
  Explosion explosions[kMaxExplosions];

  struct Laser { float x; unsigned long startMs; bool active; };
  static const int kMaxLasers = 2;
  Laser lasers[kMaxLasers];

  vector32 smoothAcc;
  int spikeCount = 0;
  unsigned long lastSpikeMs = 0;
  unsigned long lastShakeMs = 0;
  bool tapWasActive = false;
  unsigned long lastTapMs = 0;

  bool eepromReady = false;

  void ensureEeprom() {
    if (!eepromReady) {
      EEPROM.begin(512);
      eepromReady = true;
      int stored = 0;
      EEPROM.get(kEepromAddr, stored);
      if (stored >= 0 && stored < 100000) highScore = stored; // guard against reading unwritten flash
    }
  }

  void saveHighScore() {
    ensureEeprom();
    EEPROM.put(kEepromAddr, highScore);
    EEPROM.commit();
  }

  // the hex's own x-boundary at a given height, for keeping brick rows and
  // the paddle's travel range comfortably inside the true wall
  float boundaryXAt(float y) {
    float apothem = 9.0f * kSqrtThreeOverTwo;
    float ay = fabsf(y);
    if (ay > apothem) return 0;
    return 9.0f - 4.5f * ay / apothem;
  }

  void setupBricks() {
    // 4 rows, each a little narrower toward the top -- widths chosen by hand
    // against boundaryXAt() so every brick sits with real margin inside the
    // true wall, not just barely fitting
    static const float rowY[4] = {6.0f, 4.6f, 3.2f, 1.8f};
    static const int rowCount[4] = {3, 5, 5, 8};
    static const uint8_t rowHue[4] = {0, 32, 60, 96}; // red -> orange -> yellow -> green, top to bottom
    int bi = 0;
    for (int r = 0; r < 4; ++r) {
      int n = rowCount[r];
      float spacing = (n > 1) ? (2.0f * kBrickHalfWidth + 0.35f) : 0;
      float totalWidth = spacing * (n - 1);
      float startX = -totalWidth / 2.0f;
      for (int i = 0; i < n; ++i) {
        bricks[bi].x = startX + i * spacing;
        bricks[bi].y = rowY[r];
        bricks[bi].alive = true;
        bricks[bi].hue = rowHue[r] + (uint8_t)(i * 3);
        bi++;
      }
    }
  }

  void serveBall() {
    ballPos = vectorf(paddleX, kPaddleY + 0.9f);
    float angle = (float)M_PI / 2.0f + (((int)random8() - 128) / 128.0f) * 0.5f; // mostly upward, a little random
    ballVel = vectorf(cosf(angle) * kBallSpeed, sinf(angle) * kBallSpeed);
  }

  void startNewGame() {
    setupBricks();
    lives = kStartLives;
    score = 0;
    laserCharge = 0;
    newHighScore = false;
    for (int i = 0; i < kMaxExplosions; ++i) explosions[i].active = false;
    for (int i = 0; i < kMaxLasers; ++i) lasers[i].active = false;
    serveBall();
    state = kPlaying;
  }

  void spawnExplosion(float x, float y, unsigned long nowMs) {
    for (int i = 0; i < kMaxExplosions; ++i) {
      if (!explosions[i].active) {
        explosions[i] = {x, y, nowMs, true};
        return;
      }
    }
  }

  // reflect off the hexagon's own 6 walls -- identical construction to
  // LargeBouncyBall/Billiards, proven on real hardware. The bottom-ish walls
  // are effectively moot here: a missed ball is caught by the paddle-miss
  // check well above them, so play never actually reaches those edges.
  void wallCollide(vectorf &pos, vectorf &vel) {
    static const float apothem = 9.0f * kSqrtThreeOverTwo - 0.45f;
    const linef urLine(kSqrtThree,   1, -apothem * kSqrtThree);
    const linef uLine ( 0,           1, -apothem * kSqrtThree / 2);
    const linef ulLine(-kSqrtThree,  1, -apothem * kSqrtThree);
    const linef dlLine(-kSqrtThree, -1, -apothem * kSqrtThree);
    const linef dLine ( 0,          -1, -apothem * kSqrtThree / 2);
    const linef drLine(kSqrtThree,  -1, -apothem * kSqrtThree);
    const linef lines[] = {uLine, urLine, drLine, dLine, dlLine, ulLine};
    for (int it = 0; it < 3; ++it) {
      bool collided = false;
      for (int w = 0; w < 6; ++w) {
        const linef &wall = lines[w];
        float dist = wall.A * pos.x + wall.B * pos.y + wall.C;
        if (dist > 0) {
          float nLenSq = wall.A * wall.A + wall.B * wall.B;
          pos.x -= 2 * wall.A * dist / nLenSq;
          pos.y -= 2 * wall.B * dist / nLenSq;
          float vDotN = vel.x * wall.A + vel.y * wall.B;
          if (vDotN > 0) {
            vel.x -= 2 * wall.A * vDotN / nLenSq * 0.98f;
            vel.y -= 2 * wall.B * vDotN / nLenSq * 0.98f;
          }
          collided = true;
          break;
        }
      }
      if (!collided) break;
    }
  }

  void destroyBrick(int i, unsigned long nowMs) {
    bricks[i].alive = false;
    score++;
    laserCharge = min(3, laserCharge + 1);
    spawnExplosion(bricks[i].x, bricks[i].y, nowMs);
  }

  void fireLaser(unsigned long nowMs) {
    for (int i = 0; i < kMaxLasers; ++i) {
      if (!lasers[i].active) {
        lasers[i] = {paddleX, nowMs, true};
        for (int b = 0; b < kBrickCount; ++b) {
          if (bricks[b].alive && fabsf(bricks[b].x - paddleX) < kBrickHalfWidth + 0.3f) {
            destroyBrick(b, nowMs);
          }
        }
        return;
      }
    }
  }

  void drawLocalPixel(float x, float y, CRGB color) {
    Axial ipos = axial.rectToHex(vectorf(x, y), 1.0).cubeRound();
    auto pxOpt = axial.indexAtAxial(ipos);
    if (pxOpt.has_value()) {
      ctx.leds[pxOpt.value()] = color;
    }
  }

  void drawRectFilled(float cx, float cy, float hw, float hh, CRGB color) {
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      vectorf rect = axial.rectFromPixelIndex(px);
      if (fabsf(rect.x - cx) <= hw && fabsf(rect.y - cy) <= hh) {
        ctx.leds[px] = color;
      }
    }
  }

  void renderPlayfield(unsigned long nowMs) {
    // subtle background gradient -- darker toward the bottom (the "pit"),
    // a faint glow toward the brick field
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      vectorf rect = axial.rectFromPixelIndex(px);
      float t = constrain((rect.y + 7.8f) / 15.6f, 0.0f, 1.0f); // 0 bottom, 1 top
      uint8_t v = (uint8_t)(10 + 22 * t);
      ctx.leds[px] = CHSV(150, 140, v);
    }

    for (int i = 0; i < kBrickCount; ++i) {
      if (!bricks[i].alive) continue;
      CRGB c = CHSV(bricks[i].hue, 0xFF, 0xFF);
      drawRectFilled(bricks[i].x, bricks[i].y, kBrickHalfWidth, kBrickHalfHeight, c);
    }

    // paddle, brighter at its center
    for (float dx = -kPaddleHalfWidth; dx <= kPaddleHalfWidth; dx += 0.35f) {
      uint8_t v = 255 - (uint8_t)(120.0f * fabsf(dx) / kPaddleHalfWidth);
      drawLocalPixel(paddleX + dx, kPaddleY, CRGB(v, v, 255));
    }

    // ball with a short fading trail for a laser-streak feel
    for (int i = 0; i < 4; ++i) {
      float t = i * 0.6f;
      CRGB c = CHSV(45, 60, (uint8_t)(255 - i * 55));
      drawLocalPixel(ballPos.x - ballVel.x * t * 30.0f, ballPos.y - ballVel.y * t * 30.0f, c);
    }
    drawLocalPixel(ballPos.x, ballPos.y, CRGB::White);

    // explosions: a few rings of sparks expanding and fading over ~350ms
    for (int i = 0; i < kMaxExplosions; ++i) {
      if (!explosions[i].active) continue;
      float t = (nowMs - explosions[i].startMs) / 350.0f;
      if (t >= 1.0f) { explosions[i].active = false; continue; }
      float r = t * 2.2f;
      uint8_t v = (uint8_t)(255 * (1.0f - t));
      for (int a = 0; a < 6; ++a) {
        float ang = a * (float)M_PI / 3.0f + t * 2.0f;
        drawLocalPixel(explosions[i].x + cosf(ang) * r, explosions[i].y + sinf(ang) * r, CRGB(v, v / 2, 0));
      }
    }

    // lasers: a bright beam from the paddle straight up, fading out quickly
    for (int i = 0; i < kMaxLasers; ++i) {
      if (!lasers[i].active) continue;
      float t = (nowMs - lasers[i].startMs) / 250.0f;
      if (t >= 1.0f) { lasers[i].active = false; continue; }
      uint8_t v = (uint8_t)(255 * (1.0f - t));
      for (float y = kPaddleY; y < 7.5f; y += 0.4f) {
        drawLocalPixel(lasers[i].x, y, CRGB(v, 255, 255));
      }
    }

    if (laserCharge > 0) {
      for (int i = 0; i < laserCharge; ++i) {
        drawLocalPixel(paddleX - kPaddleHalfWidth - 0.6f - i * 0.5f, kPaddleY, CRGB(60, 220, 220));
      }
    }
  }

  void renderGameOver(unsigned long nowMs) {
    unsigned long elapsed = nowMs - stateChangedAtMs;
    bool showHigh = (elapsed / 2200) % 2 == 1;
    uint8_t hue = showHigh ? 42 : 0;
    ctx.leds.fill_solid(CRGB(CHSV(hue, 0xFF, 30)));
    int value = min(99, showHigh ? highScore : score);
    int tens = (value / 10) % 10, ones = value % 10;
    CRGB color = showHigh ? CRGB(255, 210, 60) : CRGB(255, 90, 90);
    drawHexGlyphSteps(ctx, hexFontGlyphForChar('0' + tens), -5, 0, 0, color);
    drawHexGlyphSteps(ctx, hexFontGlyphForChar('0' + ones), 5, 0, 0, color);
    if (showHigh && newHighScore) {
      uint8_t b = beatsin8(100, 120, 255);
      drawLocalPixel(0, 6.2f, CRGB(b, b, 0));
    }
  }

public:
  Breakout() {
    ensureEeprom();
    startNewGame();
  }

  void update() {
    unsigned long nowMs = millis();
    unsigned long elapsedMs = (lastUpdateMs > 0) ? (nowMs - lastUpdateMs) : 16;
    elapsedMs = min(elapsedMs, 60UL);
    lastUpdateMs = nowMs;

    ICM_20948_AGMT_t agmt = MotionManager::motionFrame.agmt;
    vector32 acc(agmt.acc.axes.x, agmt.acc.axes.y, agmt.acc.axes.z);
    float rawMag = sqrtf((float)acc.x * acc.x + (float)acc.y * acc.y + (float)acc.z * acc.z);
    smoothAcc = (6 * smoothAcc + acc) / 7;
    float ax = smoothAcc.x;
    float smoothMag = sqrtf((float)smoothAcc.x * smoothAcc.x + (float)smoothAcc.y * smoothAcc.y + (float)smoothAcc.z * smoothAcc.z);

    // shake to (re)start, from any state
    if (smoothMag > 1 && rawMag > smoothMag * kSpikeRatio) {
      if (nowMs - lastSpikeMs > kShakeWindowMs) spikeCount = 0;
      spikeCount++;
      lastSpikeMs = nowMs;
      if (spikeCount >= kShakeSpikesNeeded && nowMs - lastShakeMs > kShakeCooldownMs) {
        spikeCount = 0;
        lastShakeMs = nowMs;
        if (state == kGameOver) {
          startNewGame();
        }
      }
    }

    // tilt steers the paddle directly -- tilt fraction maps straight to
    // paddle position, same "raw ax = downhill direction" convention every
    // other tilt-controlled pattern in this codebase uses
    float tiltRight = (smoothMag > 1) ? constrain(ax / smoothMag, -1.0f, 1.0f) : 0;
    float paddleRange = 9.0f - 4.5f * fabsf(kPaddleY) / (9.0f * kSqrtThreeOverTwo) - kPaddleHalfWidth - 0.2f;
    paddleX = tiltRight * paddleRange;

    // a sharp tap fires a laser, same spike-vs-baseline tap detector
    // AnalogClock uses for its minute-adjust gesture
    bool tapNow = rawMag > kTapMinimumMagnitude && rawMag > smoothMag * kTapMagnitudeRatio;
    if (tapNow && !tapWasActive && nowMs - lastTapMs > kTapCooldownMs) {
      if (state == kPlaying && laserCharge > 0) {
        laserCharge--;
        fireLaser(nowMs);
      }
      lastTapMs = nowMs;
    }
    tapWasActive = tapNow;

    if (state == kPlaying) {
      ballPos.x += ballVel.x * elapsedMs;
      ballPos.y += ballVel.y * elapsedMs;
      wallCollide(ballPos, ballVel);

      // paddle: only a downward-moving ball can hit it, and where it lands
      // on the paddle steers the return angle
      if (ballVel.y < 0 && fabsf(ballPos.y - kPaddleY) < 0.35f && fabsf(ballPos.x - paddleX) < kPaddleHalfWidth + kBallRadius) {
        float hitOffset = constrain((ballPos.x - paddleX) / kPaddleHalfWidth, -1.0f, 1.0f);
        float speed = sqrtf(ballVel.x * ballVel.x + ballVel.y * ballVel.y);
        float angle = (float)M_PI / 2.0f + hitOffset * kMaxBounceSteer;
        ballVel.x = cosf(angle) * speed;
        ballVel.y = sinf(angle) * speed;
        ballPos.y = kPaddleY + 0.35f;
      }

      for (int i = 0; i < kBrickCount; ++i) {
        if (!bricks[i].alive) continue;
        if (fabsf(ballPos.x - bricks[i].x) <= kBrickHalfWidth + kBallRadius &&
            fabsf(ballPos.y - bricks[i].y) <= kBrickHalfHeight + kBallRadius) {
          float dx = ballPos.x - bricks[i].x, dy = ballPos.y - bricks[i].y;
          if (fabsf(dx) / kBrickHalfWidth > fabsf(dy) / kBrickHalfHeight) {
            ballVel.x = fabsf(ballVel.x) * (dx > 0 ? 1 : -1);
          } else {
            ballVel.y = fabsf(ballVel.y) * (dy > 0 ? 1 : -1);
          }
          destroyBrick(i, nowMs);
          break;
        }
      }

      bool anyAlive = false;
      for (int i = 0; i < kBrickCount; ++i) if (bricks[i].alive) { anyAlive = true; break; }
      if (!anyAlive) {
        setupBricks(); // cleared the field -- another wave, same score and lives
        serveBall();
      }

      if (ballPos.y < kPaddleY - 1.2f) {
        lives--;
        if (lives <= 0) {
          if (score > highScore) {
            highScore = score;
            newHighScore = true;
            saveHighScore();
          }
          state = kGameOver;
          stateChangedAtMs = nowMs;
        } else {
          state = kBallLost;
          stateChangedAtMs = nowMs;
        }
      }
    } else if (state == kBallLost) {
      if (nowMs - stateChangedAtMs > 700) {
        serveBall();
        state = kPlaying;
      }
    }

    if (state == kGameOver) {
      renderGameOver(nowMs);
    } else {
      renderPlayfield(nowMs);
      if (state == kBallLost) {
        uint8_t b = beatsin8(160, 40, 255);
        ctx.leds.fadeToBlackBy(255 - b);
      }
    }
  }

  const char *description() {
    return "Breakout";
  }
};

// Hourglass: a fixed glass bowtie silhouette (two tapering chambers joined by
// a one-pixel neck) drawn along the hex's own vertical axis, with sand that
// piles and drains between the chambers the same way PixelSand's grains
// settle toward whatever edge is currently downhill -- tilting the device
// forward/back decides which chamber is "up" and pours its sand through the
// neck into the other, exactly like flipping a real hourglass. Side tilt
// gives the sand's resting surface a subtle slant. The glass outline sparkles
// on its own, brighter and twitchier the more the device is actually moving.
class Hourglass : public Pattern {
  const float kChamberHeight = 6.2f;
  const float kNeckHalfWidth = 0.45f;
  const float kBaseHalfWidth = 4.3f;
  const int kTotalGrains = 40;
  const float kFlowDeadzone = 0.10f;
  const unsigned long kGrainIntervalMs = 280;
  const unsigned long kFallDurationMs = 420;

  int grainsInTop = kTotalGrains;
  int flowSign = 0; // +1: draining bottom->top, -1: top->bottom, 0: paused
  unsigned long lastGrainMoveMs = 0;

  struct FallingGrain { float x, startY, targetY; unsigned long startMs; bool active; };
  static const int kMaxFallingGrains = 5;
  FallingGrain fallingGrains[kMaxFallingGrains];

  std::vector<PixelIndex> glassPixels;
  std::vector<PixelIndex> haloPixels; // one ring of neighbors around the glass, for a soft glow

  vector32 smoothAcc;
  float motionEnergy = 0;

  // one shared slow-drifting ambient field -- the sky outside the glass, the
  // frosted interior, and the glow halo all sample the same hue/brightness
  // functions (at different scales) so the whole scene reads as one
  // continuous moodily-lit space rather than separately-tuned regions
  uint8_t ambientHueAt(float x, float y, unsigned long nowMs) {
    float ang = atan2f(y, x);
    return 150 + (uint8_t)(18.0f * sinf(ang * 2.0f + nowMs / 5000.0f));
  }
  uint8_t ambientBriAt(float x, float y, unsigned long nowMs) {
    float dist = sqrtf(x * x + y * y) / 9.0f;
    return 4 + (uint8_t)(7.0f * (0.5f + 0.5f * sinf(dist * 3.0f - nowMs / 4000.0f)));
  }

  float chamberHalfWidthAt(float yy) {
    float t = constrain(yy / kChamberHeight, 0.0f, 1.0f);
    return kNeckHalfWidth + (kBaseHalfWidth - kNeckHalfWidth) * t;
  }

  bool insideChamber(float x, float y) {
    float yy = fabsf(y);
    if (yy > kChamberHeight) return false;
    return fabsf(x) <= chamberHalfWidthAt(yy);
  }

  // hand-designed in Hexa Object Forge (kElement_HOURGLASS_LARGE) rather than
  // sampled off the procedural bowtie lines -- the sand simulation below
  // still uses the mathematical chamber shape (kChamberHeight etc.) for its
  // own fill/collision math, so this only swaps the decorative glass outline
  void buildGlassOutline() {
    glassPixels.clear();
    for (int i = 0; i < 271; ++i) {
      if (!((kElement_HOURGLASS_LARGE[i >> 3] >> (i & 7)) & 1)) continue;
      auto pxOpt = axial.indexAtAxial(kHexCellQR_LG[i][0], kHexCellQR_LG[i][1]);
      if (pxOpt.has_value()) glassPixels.push_back(pxOpt.value());
    }
  }

  // one ring of hex-neighbors around every rim pixel, minus the rim itself --
  // rendered dim and soft, it reads as a glow bleeding off the glass edge
  void buildGlow() {
    static const int dq[6] = {0, -1, -1, 0, 1, 1};
    static const int dr[6] = {-1, 0, 1, 1, 0, -1};
    bool isRim[LED_COUNT] = { false };
    bool added[LED_COUNT] = { false };
    for (PixelIndex px : glassPixels) isRim[px] = true;
    haloPixels.clear();
    for (PixelIndex px : glassPixels) {
      Axial a = axial.axialFromPixelIndex(px);
      for (int d = 0; d < 6; ++d) {
        auto nOpt = axial.indexAtAxial(a.q() + dq[d], a.r() + dr[d]);
        if (!nOpt.has_value()) continue;
        PixelIndex npx = nOpt.value();
        if (isRim[npx] || added[npx]) continue;
        added[npx] = true;
        haloPixels.push_back(npx);
      }
    }
  }

  // y-coordinate of the current pile's neck-facing surface -- where the fill
  // boundary sits, and where a newly-arriving grain visually lands
  float currentSurfaceTop() {
    float fA = grainsInTop / (float)kTotalGrains;
    bool pileAtFar = flowSign > 0;
    return pileAtFar ? kChamberHeight * (1 - fA) : fA * kChamberHeight;
  }
  float currentSurfaceBottom() {
    float fB = (kTotalGrains - grainsInTop) / (float)kTotalGrains;
    bool pileAtFar = flowSign < 0;
    return pileAtFar ? -kChamberHeight * (1 - fB) : -fB * kChamberHeight;
  }

  void drawLocalPixel(float x, float y, CRGB color) {
    Axial ipos = axial.rectToHex(vectorf(x, y), 1.0).cubeRound();
    auto pxOpt = axial.indexAtAxial(ipos);
    if (pxOpt.has_value()) {
      ctx.leds[pxOpt.value()] = color;
    }
  }

  void spawnFallingGrain(bool intoTop, float tiltX) {
    for (int i = 0; i < kMaxFallingGrains; ++i) {
      if (!fallingGrains[i].active) {
        float target = intoTop ? currentSurfaceTop() : currentSurfaceBottom();
        float maxX = chamberHalfWidthAt(fabsf(target)) * 0.75f;
        float jitter = (((int)random8() - 128) / 128.0f) * 0.3f;
        fallingGrains[i] = {
          constrain(tiltX * 1.4f + jitter, -maxX, maxX),
          0.0f,
          target,
          millis(),
          true
        };
        return;
      }
    }
  }

public:
  Hourglass() {
    buildGlassOutline();
    buildGlow();
    for (int i = 0; i < kMaxFallingGrains; ++i) fallingGrains[i].active = false;
  }

  void update() {
    unsigned long nowMs = millis();

    ICM_20948_AGMT_t agmt = MotionManager::motionFrame.agmt;
    vector32 acc(agmt.acc.axes.x, agmt.acc.axes.y, agmt.acc.axes.z);
    float rawMag = sqrtf((float)acc.x * acc.x + (float)acc.y * acc.y + (float)acc.z * acc.z);
    smoothAcc = (8 * smoothAcc + acc) / 9;
    float smoothMag = sqrtf((float)smoothAcc.x * smoothAcc.x + (float)smoothAcc.y * smoothAcc.y + (float)smoothAcc.z * smoothAcc.z);
    float tiltY = (smoothMag > 1) ? constrain(smoothAcc.y / smoothMag, -1.0f, 1.0f) : 0;
    float tiltX = (smoothMag > 1) ? constrain(smoothAcc.x / smoothMag, -1.0f, 1.0f) : 0;

    float jerk = max(0.0f, rawMag - smoothMag);
    motionEnergy = max(motionEnergy * 0.92f, min(1.0f, jerk / 4000.0f));

    flowSign = (fabsf(tiltY) > kFlowDeadzone) ? (tiltY > 0 ? 1 : -1) : 0;

    if (flowSign != 0 && nowMs - lastGrainMoveMs > kGrainIntervalMs) {
      lastGrainMoveMs = nowMs;
      if (flowSign > 0 && grainsInTop < kTotalGrains) {
        grainsInTop++;
        spawnFallingGrain(true, tiltX);
      } else if (flowSign < 0 && grainsInTop > 0) {
        grainsInTop--;
        spawnFallingGrain(false, tiltX);
      }
    }

    float fA = grainsInTop / (float)kTotalGrains;
    float fB = 1.0f - fA;
    bool pileAtFarA = flowSign > 0;
    bool pileAtFarB = flowSign < 0;
    float surfaceA = pileAtFarA ? kChamberHeight * (1 - fA) : fA * kChamberHeight;
    float surfaceB = pileAtFarB ? -kChamberHeight * (1 - fB) : -fB * kChamberHeight;
    float slant = tiltX * 0.6f;

    // one shared ambient field for the whole scene (see ambientHueAt/BriAt) --
    // the frosted interior reads a touch brighter than the sky outside, as if
    // the glass itself is catching that same light, not a separate palette
    for (PixelIndex px = 0; px < LED_COUNT; ++px) {
      vectorf r = axial.rectFromPixelIndex(px);
      uint8_t bgHue = ambientHueAt(r.x, r.y, nowMs);
      uint8_t bgBri = ambientBriAt(r.x, r.y, nowMs);
      CRGB c;
      if (insideChamber(r.x, r.y)) {
        bool topChamber = r.y >= 0;
        float slantedY = r.y - slant * (r.x / kBaseHalfWidth);
        bool filled = topChamber
          ? (pileAtFarA ? (slantedY >= surfaceA) : (slantedY <= surfaceA))
          : (pileAtFarB ? (slantedY <= surfaceB) : (slantedY >= surfaceB));
        if (filled) {
          uint8_t hueJitter = (px * 47) % 11;
          uint8_t sparkle = sin8(px * 13 + nowMs / 90);
          uint8_t v = 180 + sparkle / 14;
          c = CHSV(28 + hueJitter, 235, v);
        } else {
          c = CHSV(bgHue, 110, qadd8(bgBri, 10)); // frosted glass, same field, a touch brighter
        }
      } else {
        c = CHSV(bgHue, 205, bgBri);
      }
      ctx.leds[px] = c;
    }

    // soft glow bleeding off the rim -- drawn before the rim itself so the
    // rim always reads crisp and bright on top of it
    uint8_t glowPulse = beatsin8(4, 18, 50);
    CRGB glowColor = CHSV(150, 75, glowPulse);
    for (PixelIndex px : haloPixels) {
      ctx.leds[px] = glowColor;
    }

    for (int i = 0; i < kMaxFallingGrains; ++i) {
      FallingGrain &g = fallingGrains[i];
      if (!g.active) continue;
      float t = (nowMs - g.startMs) / (float)kFallDurationMs;
      if (t >= 1.0f) { g.active = false; continue; }
      float y = g.startY + (g.targetY - g.startY) * t;
      drawLocalPixel(g.x, y, CRGB(255, 220, 140));
    }

    // the rim itself: steady and bright -- no random twinkle at all now, just
    // a very slow, shallow shimmer plus a gentle motion-linked brighten so it
    // still feels alive without ever reading as "flashing"
    for (int i = 0; i < (int)glassPixels.size(); ++i) {
      PixelIndex px = glassPixels[i];
      uint8_t shimmer = beatsin8(2, 0, 18, 0, i * 23);
      uint8_t v = qadd8(228, shimmer);
      v = qadd8(v, (uint8_t)(motionEnergy * 20));
      ctx.leds[px] = CHSV(150, 45, v);
    }
  }

  const char *description() {
    return "Hourglass";
  }
};

#endif
