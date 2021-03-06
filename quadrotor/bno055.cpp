#include "bno055.h"
#include "hw_conf.h"

using namespace Eigen;

const I2CConfig BNO055::i2ccfg = {
    OPMODE_I2C,
    100000,
    STD_DUTY_CYCLE,
};

//static uint8_t calibData[] = {0x3, 0x0, 0x3A, 0x0, 0x1A, 0x0, 0xFE, 0x1, 0x45, 0xFF, 0xBF, 0xFC, 0xC1, 0x2, 0x31, 0x0, 0x7B, 0xFE, 0xE8, 0x3, 0x3F, 0x2};

BNO055::BNO055()
{
    i2cStart(&IMU_I2C_DEV, &i2ccfg);
    palSetPadMode(IMU_SCL_GPIO, IMU_SCL_PIN, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN);
    palSetPadMode(IMU_SDA_GPIO, IMU_SDA_PIN, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN);
    uint8_t id = ReadAddress(BNO055_CHIP_ID_ADDR);
    if(id != BNO055_ID)
    {
        chThdSleepMilliseconds(1000);
        id = ReadAddress(BNO055_CHIP_ID_ADDR);
        if(id != BNO055_ID) {
        }
    }
    WriteAddress(BNO055_OPR_MODE_ADDR, OPERATION_MODE_CONFIG);
    chThdSleepMilliseconds(30);
    WriteAddress(BNO055_SYS_TRIGGER_ADDR, 0x20);
    while (ReadAddress(BNO055_CHIP_ID_ADDR) != BNO055_ID)
    {
        chThdSleepMilliseconds(10);
    }
    chThdSleepMilliseconds(50);

    //[> Set to normal power mode <]
    WriteAddress(BNO055_PWR_MODE_ADDR, POWER_MODE_NORMAL);
    chThdSleepMilliseconds(10);

    WriteAddress(BNO055_PAGE_ID_ADDR, 0);

    uint8_t unitsel = (0 << 7) | // Orientation = Android
        (0 << 4) | // Temperature = Celsius
        (0 << 2) | // Euler = Degrees
        (1 << 1) | // Gyro = Rads
        (0 << 0);  // Accelerometer = m/s^2
    WriteAddress(BNO055_UNIT_SEL_ADDR, unitsel);

    WriteAddress(BNO055_SYS_TRIGGER_ADDR, 0x80);
    chThdSleepMilliseconds(10);

    //WriteAddress(ACCEL_OFFSET_X_LSB_ADDR, calibData[0]);
    //WriteAddress(ACCEL_OFFSET_X_MSB_ADDR, calibData[1]);
    //WriteAddress(ACCEL_OFFSET_Y_LSB_ADDR, calibData[2]);
    //WriteAddress(ACCEL_OFFSET_Y_MSB_ADDR, calibData[3]);
    //WriteAddress(ACCEL_OFFSET_Z_LSB_ADDR, calibData[4]);
    //WriteAddress(ACCEL_OFFSET_Z_MSB_ADDR, calibData[5]);

    //WriteAddress(GYRO_OFFSET_X_LSB_ADDR, calibData[6]);
    //WriteAddress(GYRO_OFFSET_X_MSB_ADDR, calibData[7]);
    //WriteAddress(GYRO_OFFSET_Y_LSB_ADDR, calibData[8]);
    //WriteAddress(GYRO_OFFSET_Y_MSB_ADDR, calibData[9]);
    //WriteAddress(GYRO_OFFSET_Z_LSB_ADDR, calibData[10]);
    //WriteAddress(GYRO_OFFSET_Z_MSB_ADDR, calibData[11]);

    //WriteAddress(MAG_OFFSET_X_LSB_ADDR, calibData[12]);
    //WriteAddress(MAG_OFFSET_X_MSB_ADDR, calibData[13]);
    //WriteAddress(MAG_OFFSET_Y_LSB_ADDR, calibData[14]);
    //WriteAddress(MAG_OFFSET_Y_MSB_ADDR, calibData[15]);
    //WriteAddress(MAG_OFFSET_Z_LSB_ADDR, calibData[16]);
    //WriteAddress(MAG_OFFSET_Z_MSB_ADDR, calibData[17]);

    //WriteAddress(ACCEL_RADIUS_LSB_ADDR, calibData[18]);
    //WriteAddress(ACCEL_RADIUS_MSB_ADDR, calibData[19]);

    //WriteAddress(MAG_RADIUS_LSB_ADDR, calibData[20]);
    //WriteAddress(MAG_RADIUS_MSB_ADDR, calibData[21]);

    WriteAddress(BNO055_OPR_MODE_ADDR, OPERATION_MODE_IMUPLUS);
    chThdSleepMilliseconds(50);
}

BNO055::~BNO055()
{
}

Vector3f BNO055::GetVector(uint8_t addr)
{
    double vx, vy, vz;
    uint8_t buffer[6];
    memset (buffer, 0, 6);
    ReadAddressLength(addr, 6, buffer);
    int16_t x = ((int16_t)buffer[0]) | (((int16_t)buffer[1]) << 8);
    int16_t y = ((int16_t)buffer[2]) | (((int16_t)buffer[3]) << 8);
    int16_t z = ((int16_t)buffer[4]) | (((int16_t)buffer[5]) << 8);
    switch(addr)
    {
        case VECTOR_MAGNETOMETER:
            /* 1uT = 16 LSB */
            vx = ((double)x)/16.0;
            vy = ((double)y)/16.0;
            vz = ((double)z)/16.0;
            break;
        case VECTOR_GYROSCOPE:
            /* 1rps = 900 LSB */
            vx = -((double)x)/900.0;
            vy = -((double)y)/900.0;
            vz = -((double)z)/900.0;
            break;
        case VECTOR_EULER:
            /* 1 degree = 16 LSB */
            vz = ((double)x)/16.0;
            vy = ((double)y)/16.0;
            vx = ((double)z)/16.0;
            break;
        case VECTOR_ACCELEROMETER:
        case VECTOR_LINEARACCEL:
        case VECTOR_GRAVITY:
            /* 1m/s^2 = 100 LSB */
            vx = ((double)x)/100.0;
            vy = ((double)y)/100.0;
            vz = ((double)z)/100.0;
            break;
    }

    return Vector3f(vx, vy, vz);
}

void BNO055::GetSensorOffsets(uint8_t *calib_data)
{
    if (ReadAddress(BNO055_CALIB_STAT_ADDR) == 255)
    {
        WriteAddress(BNO055_OPR_MODE_ADDR, OPERATION_MODE_CONFIG);
        chThdSleepMilliseconds(55);
        ReadAddressLength(ACCEL_OFFSET_X_LSB_ADDR, NUM_BNO055_OFFSET_REGISTERS, calib_data);
        chThdSleepMilliseconds(20);
        WriteAddress(BNO055_OPR_MODE_ADDR, OPERATION_MODE_NDOF);
        chThdSleepMilliseconds(50);
    }
}

uint8_t BNO055::GetStatus()
{
    return ReadAddress(BNO055_SYS_STAT_ADDR);
}

uint8_t BNO055::GetError()
{
    return ReadAddress(BNO055_SYS_ERR_ADDR);
}

uint8_t BNO055::ReadAddress(uint8_t addr)
{
    systime_t tmo = MS2ST(4);
    uint8_t txbuf[1];
    uint8_t rxbuf[1];
    txbuf[0] = addr;
    i2cAcquireBus(&IMU_I2C_DEV);
    uint8_t stat = i2cMasterTransmitTimeout(&IMU_I2C_DEV, BNO055_ADDR, txbuf, 1, rxbuf, 1, tmo);
    i2cReleaseBus(&IMU_I2C_DEV);
    return rxbuf[0];
}

void BNO055::ReadAddressLength(uint8_t addr, uint8_t len, uint8_t *buffer)
{
    systime_t tmo = MS2ST(4);
    uint8_t txbuf[1];
    txbuf[0] = addr;
    i2cAcquireBus(&IMU_I2C_DEV);
    uint8_t stat = i2cMasterTransmitTimeout(&IMU_I2C_DEV, BNO055_ADDR, txbuf, 1, buffer, len, tmo);
    i2cReleaseBus(&IMU_I2C_DEV);
}

void BNO055::WriteAddress(uint8_t addr, uint8_t value)
{
    systime_t tmo = MS2ST(4);
    uint8_t txbuf[2];
    uint8_t rxbuf[1];
    txbuf[0] = addr;
    txbuf[1] = value;
    i2cAcquireBus(&IMU_I2C_DEV);
    uint8_t stat = i2cMasterTransmitTimeout(&IMU_I2C_DEV, BNO055_ADDR, txbuf, 2, rxbuf, 0, tmo);
    i2cReleaseBus(&IMU_I2C_DEV);
}
