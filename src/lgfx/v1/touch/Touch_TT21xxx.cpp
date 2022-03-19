/*----------------------------------------------------------------------------/
  Lovyan GFX - Graphics library for embedded devices.

Original Source:
 https://github.com/lovyan03/LovyanGFX/

Licence:
 [FreeBSD](https://github.com/lovyan03/LovyanGFX/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)

Contributors:
 [ciniml](https://github.com/ciniml)
 [mongonta0716](https://github.com/mongonta0716)
 [tobozo](https://github.com/tobozo)
/----------------------------------------------------------------------------*/
#include "Touch_TT21xxx.hpp"

#include "../platforms/common.hpp"
#include <esp_log.h>

namespace lgfx
{
 inline namespace v1
 {
//----------------------------------------------------------------------------

  bool Touch_TT21xxx::_check_init(void)
  {
    if (_inited) return true;

    uint8_t tmp[2] = { 0 };
    _inited = lgfx::i2c::transactionRead(_cfg.i2c_port, _cfg.i2c_addr, tmp, 2, _cfg.freq).has_value()
          && tmp[0] != 0
          && tmp[1] == 0
          ;

    return _inited;
  }

  bool Touch_TT21xxx::init(void)
  {
    _inited = false;
    if (isSPI()) return false;

    if (_cfg.pin_int >= 0)
    {
      lgfx::pinMode(_cfg.pin_int, pin_mode_t::input_pullup);
    }
    return (lgfx::i2c::init(_cfg.i2c_port, _cfg.pin_sda, _cfg.pin_scl).has_value()) && _check_init();
  }

  uint_fast8_t Touch_TT21xxx::getTouchRaw(touch_point_t *tp, uint_fast8_t count)
  {
    if (!_check_init() || count == 0) { return 0; }

    if (_cfg.pin_int < 0 || !gpio_in(_cfg.pin_int))
    {
      uint_fast16_t data_len = 0;
      if (lgfx::i2c::beginTransaction(_cfg.i2c_port, _cfg.i2c_addr, _cfg.freq, true).has_value())
      {
        if (lgfx::i2c::readBytes(_cfg.i2c_port, (uint8_t*)&data_len, 2).has_error()
         || ((size_t)(data_len -= 2) > sizeof(_readdata))
         || (data_len == 0)
         || lgfx::i2c::readBytes(_cfg.i2c_port, _readdata, data_len).has_error())
        {
          ESP_LOGE("dlen","%d",data_len);
          memset(_readdata, 0, sizeof(_readdata));
        }
        else
        {
          ESP_LOGE("DLEN","%d",data_len);
        }
      }
      lgfx::i2c::endTransaction(_cfg.i2c_port).has_value();
    }

    uint32_t points = std::min<uint_fast8_t>(count, _readdata[3] & 3);

    for (size_t idx = 0; idx < points; ++idx)
    {
      auto data = &_readdata[6 + idx * 10];
      tp[idx].id = data[0] & 1;
      tp[idx].x  = data[1] + (data[2] << 8);
      tp[idx].y  = data[3] + (data[4] << 8);
      tp[idx].size = data[5];
    }
    return points;

/*
    uint8_t data[32];
    size_t data_len = 0;
    size_t touch_num = 0;

    int retry = 5;
    while (
      (lgfx::i2c::beginTransaction(_cfg.i2c_port, _cfg.i2c_addr, _cfg.freq, true).has_error()
      || lgfx::i2c::readBytes(_cfg.i2c_port, data, 2).has_error()
      || (data[1] != 0)
      || ((size_t)((data_len = data[0] - 2) - 1) >= 30)
      || lgfx::i2c::readBytes(_cfg.i2c_port, data, data_len).has_error()
      ) && --retry) { lgfx::i2c::endTransaction(_cfg.i2c_port).has_value(); }
    { touch_num = data[3] & 1; }
    lgfx::i2c::endTransaction(_cfg.i2c_port).has_value();
    _flg_released = (touch_num == 0);
ESP_LOGE("T", "%d", touch_num);

    if (count > touch_num) { count = touch_num; }
    for (size_t idx = 0; idx < touch_num; ++idx)
    {
      auto d = &data[6 + idx * 10];
      tp[idx].id = d[0] & 1;
      tp[idx].x = d[1] + (d[2] << 8);
      tp[idx].y = d[3] + (d[4] << 8);
      tp[idx].size = d[5];
    }
    return count;
//*/
  }

//----------------------------------------------------------------------------
 }
}
