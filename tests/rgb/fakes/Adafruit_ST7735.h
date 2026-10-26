#pragma once
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <glcdfont.c>

// Host raster/operation recorder implementing the GFX calls used by the monitor.
class Adafruit_ST7735 {
    int w_, h_, cursorX = 0, cursorY = 0, size = 1;
    uint16_t fg = 0xFFFF, bg = 0;
    void pixel(int x, int y, uint16_t color) {
        if (x >= 0 && y >= 0 && x < w_ && y < h_) pixels[y * w_ + x] = color;
    }
    void record(int x, int y, int w, int h) {
        if (w <= 0 || h <= 0 || x < 0 || y < 0 || x + w > w_ || y + h > h_)
            throw std::runtime_error("TFT drawing outside screen");
#ifdef PUPPYBOT_TFT_OPERATION_HOOK
        PUPPYBOT_TFT_OPERATION_HOOK();
#endif
        operations.push_back({x, y, w, h});
    }
    static bool rounded(int x, int y, int w, int h, int radius) {
        if (radius <= 0) return true;
        const int cx = x < radius ? radius : x >= w - radius ? w - radius - 1 : x;
        const int cy = y < radius ? radius : y >= h - radius ? h - radius - 1 : y;
        return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= radius * radius;
    }
public:
    struct Operation { int x, y, w, h; };
    std::vector<Operation> operations;
    std::vector<uint16_t> pixels;
    int fullClears = 0;
    explicit Adafruit_ST7735(int w = 160, int h = 128) : w_(w), h_(h), pixels(w * h) {}
    int width() const { return w_; }
    int height() const { return h_; }
    void setFont(const void *) {}
    void setTextWrap(bool) {}
    void setTextSize(int value) { size = value; }
    void setTextColor(uint16_t color, uint16_t background) { fg = color; bg = background; }
    void setCursor(int x, int y) { cursorX = x; cursorY = y; }
    void fillScreen(uint16_t color) {
        ++fullClears; record(0, 0, w_, h_); std::fill(pixels.begin(), pixels.end(), color);
    }
    void fillRect(int x, int y, int w, int h, uint16_t color) {
        record(x, y, w, h);
        for (int row = y; row < y + h; ++row)
            for (int col = x; col < x + w; ++col) pixel(col, row, color);
    }
    void drawRect(int x, int y, int w, int h, uint16_t color) {
        record(x, y, w, h);
        for (int col = x; col < x + w; ++col) { pixel(col, y, color); pixel(col, y + h - 1, color); }
        for (int row = y; row < y + h; ++row) { pixel(x, row, color); pixel(x + w - 1, row, color); }
    }
    void drawFastHLine(int x, int y, int w, uint16_t color) { fillRect(x, y, w, 1, color); }
    void fillRoundRect(int x, int y, int w, int h, int radius, uint16_t color) {
        record(x, y, w, h);
        for (int row = 0; row < h; ++row)
            for (int col = 0; col < w; ++col)
                if (rounded(col, row, w, h, radius)) pixel(x + col, y + row, color);
    }
    void drawRoundRect(int x, int y, int w, int h, int radius, uint16_t color) {
        record(x, y, w, h);
        for (int row = 0; row < h; ++row)
            for (int col = 0; col < w; ++col)
                if (rounded(col, row, w, h, radius) &&
                    (col == 0 || row == 0 || col == w - 1 || row == h - 1 ||
                     !rounded(col - 1, row - 1, w - 2, h - 2, radius - 1)))
                    pixel(x + col, y + row, color);
    }
    void print(const char *content) {
        const int length = int(std::char_traits<char>::length(content));
        record(cursorX, cursorY, length * 6 * size, 8 * size);
        for (int index = 0; index < length; ++index) {
            const uint8_t character = uint8_t(content[index]);
            for (int col = 0; col < 6; ++col) {
                const uint8_t bits = col < 5 ? font[character * 5 + col] : 0;
                for (int row = 0; row < 8; ++row)
                    for (int dy = 0; dy < size; ++dy)
                        for (int dx = 0; dx < size; ++dx)
                            pixel(cursorX + col * size + dx, cursorY + row * size + dy,
                                  bits & (1 << row) ? fg : bg);
            }
            cursorX += 6 * size;
        }
    }
    void save(const std::string &path) const {
        std::ofstream file(path, std::ios::binary);
        file << "P6\n" << w_ << ' ' << h_ << "\n255\n";
        for (uint16_t color : pixels) {
            const char bytes[3] = {char(((color >> 11) & 31) * 255 / 31),
                                   char(((color >> 5) & 63) * 255 / 63), char((color & 31) * 255 / 31)};
            file.write(bytes, 3);
        }
    }
};
