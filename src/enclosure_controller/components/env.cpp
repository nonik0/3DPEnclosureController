#include "../enclosure_controller.h"

#define SDA 13
#define SCL 15

void EnclosureController::_env_init()
{
    log_i("Initializing environment sensors");

    if (!_sht4Sensor.begin(&Wire, SHT40_I2C_ADDR_44, SDA, SCL, 400000U)) {
        log_e("SHT4X sensor not found");
        return;
    }

    _sht4Sensor.setPrecision(SHT4X_HIGH_PRECISION);
    _sht4Sensor.setHeater(SHT4X_NO_HEATER);

    if (!_bmpSensor.begin(&Wire, BMP280_I2C_ADDR, SDA, SCL, 400000U)) {
        log_e("BMP280 sensor not found");
        return;
    }

    _bmpSensor.setSampling(BMP280::MODE_NORMAL,     /* Operating Mode. */
                           BMP280::SAMPLING_X2,     /* Temp. oversampling */
                           BMP280::SAMPLING_X16,    /* Pressure oversampling */
                           BMP280::FILTER_X16,      /* Filtering. */
                           BMP280::STANDBY_MS_500); /* Standby time. */

    log_i("Environment sensors initialized");
    _env_inited = true;
}

void EnclosureController::_env_update()
{
    if (!_env_inited)
        return;

    _sht4Sensor.update();
    _bmpSensor.update();

    _temp = _sht4Sensor.cTemp * 9 / 5 + 32;
    _hum = _sht4Sensor.humidity;
    _press = _bmpSensor.pressure / 100.0;
}
