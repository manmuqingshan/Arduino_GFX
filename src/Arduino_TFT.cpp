/*
 * start rewrite from:
 * https://github.com/adafruit/Adafruit-GFX-Library.git
 */
#include "Arduino_DataBus.h"
#include "Arduino_GFX.h"
#include "Arduino_TFT.h"
#include "font/glcdfont.h"

// TODO: temp move drawSlashLine() variables to static to avoid enable PSRAM performamce issue
static int16_t dx;
static int16_t dy;
static int16_t err;
static int16_t xs;
static int16_t step;
static int16_t len;

Arduino_TFT::Arduino_TFT(
    Arduino_DataBus *bus, int8_t rst, uint8_t r,
    bool ips, int16_t w, int16_t h,
    uint8_t col_offset1, uint8_t row_offset1, uint8_t col_offset2, uint8_t row_offset2)
    : Arduino_GFX(w, h)
{
  _bus = bus;
  _rst = rst;
  _rotation = r;
  _ips = ips;
  WIDTH = w;
  HEIGHT = h;
  COL_OFFSET1 = col_offset1;
  ROW_OFFSET1 = row_offset1;
  COL_OFFSET2 = col_offset2;
  ROW_OFFSET2 = row_offset2;
}

void Arduino_TFT::begin(int32_t speed)
{
  if (_override_datamode >= 0)
  {
    _bus->begin(speed, _override_datamode);
  }
  else
  {
    _bus->begin(speed);
  }

  if (_rst >= 0)
  {
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, HIGH);
    delay(120);
    digitalWrite(_rst, LOW);
    delay(120);
    digitalWrite(_rst, HIGH);
    delay(120);
  }

  tftInit();
  setRotation(_rotation); // apply the setting rotation to the display
  setAddrWindow(0, 0, _width, _height);
}

void Arduino_TFT::startWrite()
{
  _bus->beginWrite();
}

void Arduino_TFT::writeColor(uint16_t color)
{
  _bus->write16(color);
}

void Arduino_TFT::writePixelPreclipped(int16_t x, int16_t y, uint16_t color)
{
  writeAddrWindow(x, y, 1, 1);
  writeColor(color);
}

void Arduino_TFT::writeRepeat(uint16_t color, uint32_t len)
{
  _bus->writeRepeat(color, len);
}

void Arduino_TFT::writePixels(uint16_t *data, uint32_t len)
{
  _bus->writePixels(data, len);
}

void Arduino_TFT::writeBytes(uint8_t *data, uint32_t len)
{
  _bus->writeBytes(data, len);
}

/*!
    @brief  Draw a vertical line on the display. Performs edge clipping and
            rejection. Not self-contained; should follow startWrite().
            Typically used by higher-level graphics primitives; user code
            shouldn't need to call this and is likely to use the self-
            contained drawFastVLine() instead.
    @param  x      Horizontal position of first point.
    @param  y      Vertical position of first point.
    @param  h      Line height in pixels (positive = below first point,
                   negative = above first point).
    @param  color  16-bit line color in '565' RGB format.
*/
void Arduino_TFT::writeFastVLine(int16_t x, int16_t y, int16_t h,
                                 uint16_t color)
{
  if (_ordered_in_range(x, 0, _max_x) && h)
  { // X on screen, nonzero height
    if (h < 0)
    {             // If negative height...
      y += h + 1; //   Move Y to top edge
      h = -h;     //   Use positive height
    }
    if (y <= _max_y)
    { // Not off bottom
      int16_t y2 = y + h - 1;
      if (y2 >= 0)
      { // Not off top
        // Line partly or fully overlaps screen
        if (y < 0)
        {
          y = 0;
          h = y2 + 1;
        } // Clip top
        if (y2 > _max_y)
        {
          h = _max_y - y + 1;
        } // Clip bottom
        writeFillRectPreclipped(x, y, 1, h, color);
      }
    }
  }
}

/*!
    @brief  Draw a horizontal line on the display. Performs edge clipping
            and rejection. Not self-contained; should follow startWrite().
            Typically used by higher-level graphics primitives; user code
            shouldn't need to call this and is likely to use the self-
            contained drawFastHLine() instead.
    @param  x      Horizontal position of first point.
    @param  y      Vertical position of first point.
    @param  w      Line width in pixels (positive = right of first point,
                   negative = point of first corner).
    @param  color  16-bit line color in '565' RGB format.
*/
void Arduino_TFT::writeFastHLine(int16_t x, int16_t y, int16_t w,
                                 uint16_t color)
{
  if (_ordered_in_range(y, 0, _max_y) && w)
  { // Y on screen, nonzero width
    if (w < 0)
    {             // If negative width...
      x += w + 1; //   Move X to left edge
      w = -w;     //   Use positive width
    }
    if (x <= _max_x)
    { // Not off right
      int16_t x2 = x + w - 1;
      if (x2 >= 0)
      { // Not off left
        // Line partly or fully overlaps screen
        if (x < 0)
        {
          x = 0;
          w = x2 + 1;
        } // Clip left
        if (x2 > _max_x)
        {
          w = _max_x - x + 1;
        } // Clip right
        writeFillRectPreclipped(x, y, w, 1, color);
      }
    }
  }
}

/*!
    @brief  A lower-level version of writeFillRect(). This version requires
            all inputs are in-bounds, that width and height are positive,
            and no part extends offscreen. NO EDGE CLIPPING OR REJECTION IS
            PERFORMED. If higher-level graphics primitives are written to
            handle their own clipping earlier in the drawing process, this
            can avoid unnecessary function calls and repeated clipping
            operations in the lower-level functions.
    @param  x      Horizontal position of first corner. MUST BE WITHIN
                   SCREEN BOUNDS.
    @param  y      Vertical position of first corner. MUST BE WITHIN SCREEN
                   BOUNDS.
    @param  w      Rectangle width in pixels. MUST BE POSITIVE AND NOT
                   EXTEND OFF SCREEN.
    @param  h      Rectangle height in pixels. MUST BE POSITIVE AND NOT
                   EXTEND OFF SCREEN.
    @param  color  16-bit fill color in '565' RGB format.
    @note   This is a new function, no graphics primitives besides rects
            and horizontal/vertical lines are written to best use this yet.
*/
void Arduino_TFT::writeFillRectPreclipped(int16_t x, int16_t y,
                                          int16_t w, int16_t h, uint16_t color)
{
#ifdef ESP8266
  yield();
#endif
  writeAddrWindow(x, y, w, h);
  writeRepeat(color, (uint32_t)w * h);
}

void Arduino_TFT::endWrite()
{
  _bus->endWrite();
}

/**************************************************************************/
/*!
   @brief    Push a pixel, overwrite in subclasses if startWrite is defined!
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Arduino_TFT::pushColor(uint16_t color)
{
  _bus->beginWrite();
  writeColor(color);
  _bus->endWrite();
}

void Arduino_TFT::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t w,
                                uint16_t h)
{
  startWrite();

  writeAddrWindow(x0, y0, w, h);

  endWrite();
}

/**************************************************************************/
/*!
    @brief      Set rotation setting for display
    @param  x   0 thru 3 corresponding to 4 cardinal rotations
*/
/**************************************************************************/
void Arduino_TFT::setRotation(uint8_t r)
{
  Arduino_GFX::setRotation(r);
  switch (_rotation)
  {
  case 0:
    _xStart = COL_OFFSET1;
    _yStart = ROW_OFFSET1;
    break;

  case 1:
    _xStart = ROW_OFFSET1;
    _yStart = COL_OFFSET2;
    break;

  case 2:
    _xStart = COL_OFFSET2;
    _yStart = ROW_OFFSET2;
    break;

  case 3:
    _xStart = ROW_OFFSET2;
    _yStart = COL_OFFSET1;
    break;
  }
}

// TFT optimization code, too big for ATMEL family
#if defined(ARDUINO_ARCH_SAMD) || defined(ESP8266) || defined(ESP32)

/**************************************************************************/
/*!
   @brief    Write a line.  Bresenham's algorithm - thx wikpedia
    @param    x0  Start point x coordinate
    @param    y0  Start point y coordinate
    @param    x1  End point x coordinate
    @param    y1  End point y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Arduino_TFT::writeSlashLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                 uint16_t color)
{
  bool steep = _diff(y1, y0) > _diff(x1, x0);
  if (steep)
  {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1)
  {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  dx = x1 - x0;
  dy = _diff(y1, y0);
  err = dx >> 1;
  xs = x0;
  step = (y0 < y1) ? 1 : -1;
  len = 0;

  while (x0 <= x1)
  {
    x0++;
    len++;
    err -= dy;
    if ((err < 0) || ((x0 > x1) && len))
    {
      if (steep)
      {
        writeFillRectPreclipped(y0, xs, 1, len, color);
      }
      else
      {
        writeFillRectPreclipped(xs, y0, len, 1, color);
      }
      err += dx;
      y0 += step;
      len = 0;
      xs = x0;
    }
  }
}

// TFT tuned BITMAP / XBITMAP / GRAYSCALE / RGB BITMAP FUNCTIONS ---------------------

/**************************************************************************/
/*!
   @brief      Draw a PROGMEM-resident 1-bit image at the specified (x,y) position, using the specified foreground (for set bits) and background (unset bits) colors.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw pixels with
    @param    bg 16-bit 5-6-5 Color to draw background with
*/
/**************************************************************************/
void Arduino_TFT::drawBitmap(int16_t x, int16_t y,
                             const uint8_t bitmap[], int16_t w, int16_t h,
                             uint16_t color, uint16_t bg)
{
  if (
      ((x + w - 1) < 0) || // Outside left
      ((y + h - 1) < 0) || // Outside top
      (x > _max_x) ||      // Outside right
      (y > _max_y)         // Outside bottom
  )
  {
    return;
  }
  else if (
      (x < 0) ||                // Clip left
      (y < 0) ||                // Clip top
      ((x + w - 1) > _max_x) || // Clip right
      ((y + h - 1) > _max_y)    // Clip bottom
  )
  {
    Arduino_GFX::drawBitmap(x, y, bitmap, w, h, color, bg);
  }
  else
  {
    int32_t pixels = w * h;
    uint8_t byte = 0;
    uint16_t idx = 0;
    startWrite();
    writeAddrWindow(x, y, w, h);
    for (int32_t i = 0; i < pixels; i++)
    {
      if (i & 7)
      {
        byte <<= 1;
      }
      else
      {
        byte = pgm_read_byte(&bitmap[idx++]);
      }
      writeColor((byte & 0x80) ? color : bg);
    }
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief      Draw a RAM-resident 1-bit image at the specified (x,y) position, using the specified foreground (for set bits) and background (unset bits) colors.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw pixels with
    @param    bg 16-bit 5-6-5 Color to draw background with
*/
/**************************************************************************/
void Arduino_TFT::drawBitmap(int16_t x, int16_t y,
                             uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg)
{
  if (
      ((x + w - 1) < 0) || // Outside left
      ((y + h - 1) < 0) || // Outside top
      (x > _max_x) ||      // Outside right
      (y > _max_y)         // Outside bottom
  )
  {
    return;
  }
  else if (
      (x < 0) ||                // Clip left
      (y < 0) ||                // Clip top
      ((x + w - 1) > _max_x) || // Clip right
      ((y + h - 1) > _max_y)    // Clip bottom
  )
  {
    Arduino_GFX::drawBitmap(x, y, bitmap, w, h, color, bg);
  }
  else
  {
    int32_t pixels = w * h;
    uint8_t byte = 0;
    startWrite();
    writeAddrWindow(x, y, w, h);
    for (int32_t i = 0; i < pixels; i++)
    {
      if (i & 7)
      {
        byte <<= 1;
      }
      else
      {
        byte = *(bitmap++);
      }
      writeColor((byte & 0x80) ? color : bg);
    }
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief   Draw a PROGMEM-resident 8-bit image (grayscale) at the specified (x,y) pos.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with grayscale bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void Arduino_TFT::drawGrayscaleBitmap(int16_t x, int16_t y,
                                      const uint8_t bitmap[], int16_t w, int16_t h)
{
  if (
      ((x + w - 1) < 0) || // Outside left
      ((y + h - 1) < 0) || // Outside top
      (x > _max_x) ||      // Outside right
      (y > _max_y)         // Outside bottom
  )
  {
    return;
  }
  else if (
      (x < 0) ||                // Clip left
      (y < 0) ||                // Clip top
      ((x + w - 1) > _max_x) || // Clip right
      ((y + h - 1) > _max_y)    // Clip bottom
  )
  {
    Arduino_GFX::drawGrayscaleBitmap(x, y, bitmap, w, h);
  }
  else
  {
    uint32_t len = (uint32_t)w * h;
    uint8_t v;
    startWrite();
    writeAddrWindow(x, y, w, h);
    for (uint32_t i = 0; i < len; i++)
    {
      v = (uint8_t)pgm_read_byte(&bitmap[i]);
      writeColor(color565(v, v, v));
    }
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief   Draw a RAM-resident 8-bit image (grayscale) at the specified (x,y) pos.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with grayscale bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void Arduino_TFT::drawGrayscaleBitmap(int16_t x, int16_t y,
                                      uint8_t *bitmap, int16_t w, int16_t h)
{
  if (
      ((x + w - 1) < 0) || // Outside left
      ((y + h - 1) < 0) || // Outside top
      (x > _max_x) ||      // Outside right
      (y > _max_y)         // Outside bottom
  )
  {
    return;
  }
  else if (
      (x < 0) ||                // Clip left
      (y < 0) ||                // Clip top
      ((x + w - 1) > _max_x) || // Clip right
      ((y + h - 1) > _max_y)    // Clip bottom
  )
  {
    Arduino_GFX::drawGrayscaleBitmap(x, y, bitmap, w, h);
  }
  else
  {
    uint32_t len = (uint32_t)w * h;
    uint8_t v;
    startWrite();
    writeAddrWindow(x, y, w, h);
    while (len--)
    {
      v = *(bitmap++);
      writeColor(color565(v, v, v));
    }
    endWrite();
  }
}

/**************************************************************************/
/*!
    @brief  Draw a Indexed 16-bit image (RGB 5/6/5) at the specified (x,y) position.
    @param  bitmap      byte array of Indexed color bitmap
    @param  color_index byte array of 16-bit color index
    @param  w           Width of bitmap in pixels
    @param  h           Height of bitmap in pixels
*/
/**************************************************************************/
void Arduino_TFT::writeIndexedPixels(uint8_t *bitmap, uint16_t *color_index, uint32_t len)
{
  _bus->writeIndexedPixels(bitmap, color_index, len);
}

/**************************************************************************/
/*!
    @brief  Draw a Indexed 16-bit image (RGB 5/6/5) at the specified (x,y) position.
    @param  bitmap      byte array of Indexed color bitmap
    @param  color_index byte array of 16-bit color index
    @param  w           Width of bitmap in pixels
    @param  h           Height of bitmap in pixels
*/
/**************************************************************************/
void Arduino_TFT::writeIndexedPixelsDouble(uint8_t *bitmap, uint16_t *color_index, uint32_t len)
{
  _bus->writeIndexedPixelsDouble(bitmap, color_index, len);
}

/**************************************************************************/
/*!
    @brief  Draw a Indexed 16-bit image (RGB 5/6/5) at the specified (x,y) position.
    @param  x           Top left corner x coordinate
    @param  y           Top left corner y coordinate
    @param  bitmap      byte array of Indexed color bitmap
    @param  color_index byte array of 16-bit color index
    @param  w           Width of bitmap in pixels
    @param  h           Height of bitmap in pixels
*/
/**************************************************************************/
void Arduino_TFT::drawIndexedBitmap(int16_t x, int16_t y,
                                    uint8_t *bitmap, uint16_t *color_index, int16_t w, int16_t h)
{
  if (
      ((x + w - 1) < 0) || // Outside left
      ((y + h - 1) < 0) || // Outside top
      (x > _max_x) ||      // Outside right
      (y > _max_y)         // Outside bottom
  )
  {
    return;
  }
  else if (
      (x < 0) ||                // Clip left
      (y < 0) ||                // Clip top
      ((x + w - 1) > _max_x) || // Clip right
      ((y + h - 1) > _max_y)    // Clip bottom
  )
  {
    Arduino_GFX::drawIndexedBitmap(x, y, bitmap, color_index, w, h);
  }
  else
  {
    uint32_t len = w * h;
    startWrite();
    writeAddrWindow(x, y, w, h);
    _bus->writeIndexedPixels(bitmap, color_index, len);
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief   Draw a PROGMEM-resident 16-bit image (RGB 5/6/5) at the specified (x,y) position.
   For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void Arduino_TFT::draw16bitRGBBitmap(int16_t x, int16_t y,
                                     const uint16_t bitmap[], int16_t w, int16_t h)
{
  if (
      ((x + w - 1) < 0) || // Outside left
      ((y + h - 1) < 0) || // Outside top
      (x > _max_x) ||      // Outside right
      (y > _max_y)         // Outside bottom
  )
  {
    return;
  }
  else if (
      (x < 0) ||                // Clip left
      (y < 0) ||                // Clip top
      ((x + w - 1) > _max_x) || // Clip right
      ((y + h - 1) > _max_y)    // Clip bottom
  )
  {
    Arduino_GFX::draw16bitRGBBitmap(x, y, bitmap, w, h);
  }
  else
  {
    uint32_t len = (uint32_t)w * h;
    startWrite();
    writeAddrWindow(x, y, w, h);
    for (uint32_t i = 0; i < len; i++)
    {
      writeColor(pgm_read_word(&bitmap[i]));
    }
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief   Draw a RAM-resident 16-bit image (RGB 5/6/5) at the specified (x,y) position.
   For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void Arduino_TFT::draw16bitRGBBitmap(int16_t x, int16_t y,
                                     uint16_t *bitmap, int16_t w, int16_t h)
{
  if (
      ((x + w - 1) < 0) || // Outside left
      ((y + h - 1) < 0) || // Outside top
      (x > _max_x) ||      // Outside right
      (y > _max_y)         // Outside bottom
  )
  {
    return;
  }
  else if (
      (x < 0) ||                // Clip left
      (y < 0) ||                // Clip top
      ((x + w - 1) > _max_x) || // Clip right
      ((y + h - 1) > _max_y)    // Clip bottom
  )
  {
    Arduino_GFX::draw16bitRGBBitmap(x, y, bitmap, w, h);
  }
  else
  {
    startWrite();
    writeAddrWindow(x, y, w, h);
    _bus->writePixels(bitmap, (uint32_t)w * h);
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief   Draw a RAM-resident 16-bit Big Endian image (RGB 5/6/5) at the specified (x,y) position.
   For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void Arduino_TFT::draw16bitBeRGBBitmap(int16_t x, int16_t y,
                                       uint16_t *bitmap, int16_t w, int16_t h)
{
  if (
      ((x + w - 1) < 0) || // Outside left
      ((y + h - 1) < 0) || // Outside top
      (x > _max_x) ||      // Outside right
      (y > _max_y)         // Outside bottom
  )
  {
    return;
  }
  else if (
      (x < 0) ||                // Clip left
      (y < 0) ||                // Clip top
      ((x + w - 1) > _max_x) || // Clip right
      ((y + h - 1) > _max_y)    // Clip bottom
  )
  {
    Arduino_GFX::draw16bitBeRGBBitmap(x, y, bitmap, w, h);
  }
  else
  {
    startWrite();
    writeAddrWindow(x, y, w, h);
    _bus->writeBytes((uint8_t *)bitmap, (uint32_t)w * h * 2);
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief   Draw a PROGMEM-resident 24-bit image (RGB 5/6/5) at the specified (x,y) position.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void Arduino_TFT::draw24bitRGBBitmap(int16_t x, int16_t y,
                                     const uint8_t bitmap[], int16_t w, int16_t h)
{
  if (
      ((x + w - 1) < 0) || // Outside left
      ((y + h - 1) < 0) || // Outside top
      (x > _max_x) ||      // Outside right
      (y > _max_y)         // Outside bottom
  )
  {
    return;
  }
  else if (
      (x < 0) ||                // Clip left
      (y < 0) ||                // Clip top
      ((x + w - 1) > _max_x) || // Clip right
      ((y + h - 1) > _max_y)    // Clip bottom
  )
  {
    Arduino_GFX::draw24bitRGBBitmap(x, y, bitmap, w, h);
  }
  else
  {
    uint32_t len = (uint32_t)w * h;
    uint32_t offset = 0;
    startWrite();
    writeAddrWindow(x, y, w, h);
    while (len--)
    {
      writeColor(color565(pgm_read_byte(&bitmap[offset]), pgm_read_byte(&bitmap[offset + 1]), pgm_read_byte(&bitmap[offset + 2])));
      offset += 3;
    }
    endWrite();
  }
}

/**************************************************************************/
/*!
   @brief   Draw a RAM-resident 24-bit image (RGB 5/6/5) at the specified (x,y) position.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void Arduino_TFT::draw24bitRGBBitmap(int16_t x, int16_t y,
                                     uint8_t *bitmap, int16_t w, int16_t h)
{
  if (
      ((x + w - 1) < 0) || // Outside left
      ((y + h - 1) < 0) || // Outside top
      (x > _max_x) ||      // Outside right
      (y > _max_y)         // Outside bottom
  )
  {
    return;
  }
  else if (
      (x < 0) ||                // Clip left
      (y < 0) ||                // Clip top
      ((x + w - 1) > _max_x) || // Clip right
      ((y + h - 1) > _max_y)    // Clip bottom
  )
  {
    Arduino_GFX::draw24bitRGBBitmap(x, y, bitmap, w, h);
  }
  else
  {
    uint32_t len = (uint32_t)w * h;
    uint32_t offset = 0;
    startWrite();
    writeAddrWindow(x, y, w, h);
    while (len--)
    {
      writeColor(color565(bitmap[offset], bitmap[offset + 1], bitmap[offset + 2]));
      offset += 3;
    }
    endWrite();
  }
}

// Draw a character
/**************************************************************************/
/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    color 16-bit 5-6-5 Color to draw chraracter with
    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color, no background)
*/
/**************************************************************************/
void Arduino_TFT::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg)
{
  uint16_t block_w;
  uint16_t block_h;

  if (!gfxFont) // 'Classic' built-in font
  {
    block_w = 6 * textsize_x;
    block_h = 8 * textsize_y;
    if (
        (x < 0) ||                      // Clip left
        (y < 0) ||                      // Clip top
        ((x + block_w - 1) > _max_x) || // Clip right
        ((y + block_h - 1) > _max_y)    // Clip bottom
    )
    {
      // partial draw char by parent class
      Arduino_GFX::drawChar(x, y, c, color, bg);
    }
    else
    {
      if (!_cp437 && (c >= 176))
      {
        c++; // Handle 'classic' charset behavior
      }

      uint8_t col[5];
      for (int8_t i = 0; i < 5; i++)
      {
        col[i] = pgm_read_byte(&font[c * 5 + i]);
      }

      startWrite();
      if (bg != color) // have background color
      {
        writeAddrWindow(x, y, block_w, block_h);

        uint16_t line_buf[block_w];
        if (textsize_x == 1)
        {
          line_buf[5] = bg; // last column always bg
        }
        else
        {
          for (int8_t k = 0; k < textsize_x; k++)
          {
            line_buf[5 * textsize_x + k] = bg;
          }
        }
        uint8_t bit = 1;
        bool draw_dot;

        while (bit)
        {
          for (int8_t i = 0; i < 5; i++)
          {
            draw_dot = col[i] & bit;
            if (textsize_x == 1)
            {
              line_buf[i] = (draw_dot) ? color : bg;
            }
            else
            {
              if (draw_dot)
              {
                for (int8_t k = 0; k < textsize_x; k++)
                {
                  line_buf[i * textsize_x + k] = (k < (textsize_x - text_pixel_margin)) ? color : bg;
                }
              }
              else
              {
                for (int8_t k = 0; k < textsize_x; k++)
                {
                  line_buf[i * textsize_x + k] = bg;
                }
              }
            }
          }
          if (textsize_y == 1)
          {
            writePixels(line_buf, block_w);
          }
          else
          {
            for (int8_t l = 0; l < textsize_y; l++)
            {
              if (l < (textsize_y - text_pixel_margin))
              {
                writePixels(line_buf, block_w);
              }
              else
              {
                writeRepeat(bg, block_w);
              }
            }
          }
          bit <<= 1;
        }
      }
      else // (bg == color), no background color
      {
        for (int8_t i = 0; i < 5; i++)
        { // Char bitmap = 5 columns
          uint8_t line = col[i];
          for (int8_t j = 0; j < 8; j++, line >>= 1)
          {
            if (line & 1)
            {
              if (textsize_x == 1 && textsize_y == 1)
              {
                writePixelPreclipped(x + i, y + j, color);
              }
              else
              {
                writeFillRectPreclipped(x + i * textsize_x, y + j * textsize_y, textsize_x - text_pixel_margin, textsize_y - text_pixel_margin, color);
              }
            }
          }
        }
      }
      endWrite();
    }
  }
  else // Custom font
  {
    // Character is assumed previously filtered by write() to eliminate
    // newlines, returns, non-printable characters, etc.  Calling
    // drawChar() directly with 'bad' characters of font may cause mayhem!
    uint8_t first = pgm_read_byte(&gfxFont->first);
    GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c - first);
    uint8_t *bitmap = pgm_read_bitmap_ptr(gfxFont);

    uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
    uint8_t w = pgm_read_byte(&glyph->width),
            h = pgm_read_byte(&glyph->height),
            xAdvance = pgm_read_byte(&glyph->xAdvance),
            yAdvance = pgm_read_byte(&gfxFont->yAdvance),
            baseline = yAdvance * 2 / 3; // TODO: baseline is an arbitrary currently, may be define in font file
    int8_t xo = pgm_read_byte(&glyph->xOffset),
           yo = pgm_read_byte(&glyph->yOffset);
    uint8_t xx, yy, bits = 0, bit = 0;
    int16_t xo16 = xo, yo16 = yo;

    if (xAdvance < w)
    {
      xAdvance = w; // Don't know why it exists
    }

    block_w = xAdvance * textsize_x;
    block_h = yAdvance * textsize_y;
    int16_t x1 = (xo < 0) ? (x + xo) : x;
    if (
        (x1 < 0) ||                             // Clip left
        ((y - baseline) < 0) ||                 // Clip top
        ((x1 + block_w - 1) > _max_x) ||        // Clip right
        ((y - baseline + block_h - 1) > _max_y) // Clip bottom
    )
    {
      // partial draw char by parent class
      Arduino_GFX::drawChar(x, y, c, color, bg);
    }
    else
    {
      // NOTE: Different from Adafruit_GFX design, Adruino_GFX also cater background.
      // Since it may introduce many ugly output, it should limited using on mono font only.
      if (xo < 0) // padding X offset to >= 0
      {
        x += xo;
        xo = 0;
      }

      startWrite();
      if (bg != color) // have background color
      {
        writeAddrWindow(x, y - (baseline * textsize_y), block_w, block_h);

        uint16_t line_buf[block_w];
        int8_t i;
        bool draw_dot;
        for (yy = 0; yy < yAdvance; yy++)
        {
          if ((yy < (baseline + yo)) || (yy > (baseline + yo + h - 1)))
          {
            writeRepeat(bg, block_w * textsize_y);
          }
          else
          {
            i = 0;
            for (xx = 0; xx < xAdvance; xx++)
            {
              if ((xx < xo) || (xx > (xo + w - 1)))
              {
                draw_dot = false;
              }
              else
              {
                if (!(bit++ & 7))
                {
                  bits = pgm_read_byte(&bitmap[bo++]);
                }
                draw_dot = bits & 0x80;
                bits <<= 1;
              }

              if (textsize_x == 1)
              {
                line_buf[i++] = draw_dot ? color : bg;
              }
              else
              {
                if (draw_dot)
                {
                  for (int8_t k = 0; k < textsize_x; k++)
                  {
                    line_buf[i++] = (k < (textsize_x - text_pixel_margin)) ? color : bg;
                  }
                }
                else
                {
                  for (int8_t k = 0; k < textsize_x; k++)
                  {
                    line_buf[i++] = bg;
                  }
                }
              }
            }
            if (textsize_y == 1)
            {
              writePixels(line_buf, block_w);
            }
            else
            {
              for (int8_t l = 0; l < textsize_y; l++)
              {
                if (l < (textsize_y - text_pixel_margin))
                {
                  writePixels(line_buf, block_w);
                }
                else
                {
                  writeRepeat(bg, block_w);
                }
              }
            }
          }
        }
      }
      else // (bg == color), no background color
      {
        for (yy = 0; yy < h; yy++)
        {
          for (xx = 0; xx < w; xx++)
          {
            if (!(bit++ & 7))
            {
              bits = pgm_read_byte(&bitmap[bo++]);
            }
            if (bits & 0x80)
            {
              if (textsize_x == 1 && textsize_y == 1)
              {
                writePixelPreclipped(x + xo + xx, y + yo + yy, color);
              }
              else
              {
                writeFillRectPreclipped(x + (xo16 + xx) * textsize_x, y + (yo16 + yy) * textsize_y,
                                        textsize_x - text_pixel_margin, textsize_y - text_pixel_margin, color);
              }
            }
            bits <<= 1;
          }
        }
      }
      endWrite();
    }
  } // End classic vs custom font
}

#endif // defined(ARDUINO_ARCH_SAMD) || defined(ESP8266) || defined(ESP32)