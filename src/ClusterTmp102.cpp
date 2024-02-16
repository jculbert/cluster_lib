/*
 * ClusterTmp102.cpp
 *
 *  Created on: Jan. 23, 2024
 *      Author: jeff
 */

#include "AppConfig.h"

#ifdef CLUSTER_TMP102

#include "ClusterTmp102.h"

#include "em_i2c.h"
#include "sl_i2cspm.h"
#include "sl_i2cspm_instances.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#include <app/util/attribute-storage.h>

#define TMP102_ADDRESS (0x48)

namespace cluster_lib
{
static ClusterTmp102 *cluster;


/***************************************************************************//**
 * Function to perform I2C transactions on the Tmp102
 *
 ******************************************************************************/
static I2C_TransferReturn_TypeDef Tmp102Transaction(uint16_t flag,
  uint8_t *writeCmd, size_t writeLen, uint8_t *readCmd, size_t readLen)
{
  I2C_TransferSeq_TypeDef seq;
  I2C_TransferReturn_TypeDef ret;

  seq.addr = TMP102_ADDRESS << 1;
  seq.flags = flag;

  switch (flag) {
    // Send the write command from writeCmd
    case I2C_FLAG_WRITE:
      seq.buf[0].data = writeCmd;
      seq.buf[0].len  = writeLen;
      break;

    // Receive data into readCmd of readLen
    case I2C_FLAG_READ:
      seq.buf[0].data = readCmd;
      seq.buf[0].len  = readLen;
      break;

      // Send the write command from writeCmd
      // and receive data into readCmd of readLen
    case I2C_FLAG_WRITE_READ:
      seq.buf[0].data = writeCmd;
      seq.buf[0].len  = writeLen;

      seq.buf[1].data = readCmd;
      seq.buf[1].len  = readLen;
      break;

    default:
      return i2cTransferUsageFault;
  }

  // Perform the transfer and return status from the transfer
  ret = I2CSPM_Transfer(sl_i2cspm_i2c_tmp102, &seq);

  return ret;
}

static void UpdateClusterState(intptr_t notused)
{
    SILABS_LOG("Tmp102 %d", cluster->temperature);
    chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(cluster->endpoint, cluster->temperature);
}

ClusterTmp102::ClusterTmp102 (uint32_t _endpoint, PostEventCallback _postEventCallback, uint32_t read_period_sec)
    : ClusterWorker(_endpoint, _postEventCallback), state(STATE_TRIGGER_READING),
      read_period_ms(read_period_sec * 1000), temperature(0)
{
    cluster = this;

    //SILABS_LOG("sl_si7021_init status = %d\n", status);

    // Put Tmp102 in powerdown mode
    // Write 0x01 in pointer register to address config register
    // and 0x01 to config register
    uint8_t write[] = {0x01, 0x01};
    I2C_TransferReturn_TypeDef status = Tmp102Transaction(I2C_FLAG_WRITE, write, sizeof(write), NULL, 0);
    SILABS_LOG("Tmp102 tran = %d", status);

    // First run
    RequestProcess(10000);
}

void ClusterTmp102::Process(const AppEvent * event)
{
    //SILABS_LOG("Tmp102 Process");

    switch(state)
    {
        case STATE_TRIGGER_READING:
        {
            // Write 0x81 to config register to trigger one shot reading
            // and remain in power down mode
            uint8_t write[] = {0x01, 0x81};
            Tmp102Transaction(I2C_FLAG_WRITE, write, sizeof(write), NULL, 0);
            //SILABS_LOG("Tmp102 tran = %d", status);

            // read value after one second (conversion time is approx 25 ms
            state = STATE_READ;
            RequestProcess(1000);
            break;
        }
        case STATE_READ:
        {
            // Read two bytes from address 0
            //  1. Write 0 to pointer register to set address for read
            //  2. read 2 bytes
            uint8_t write[] = {0x00};
            uint8_t read[2];
            Tmp102Transaction(I2C_FLAG_WRITE_READ, write, sizeof(write), read, sizeof(read));

            // Temperature is coded in signed 12 bits, 0.0625 C per bit
            // Construct raw value with sign exention down to lower 12 bits
            int16_t raw = ((int16_t)(read[0]<<8 | read[1])) / 16;
            // Temperature as C * 100 is: (raw to lower 12 bits) * 6.25
            temperature = (raw * 625) / 100;
            //SILABS_LOG("Tmp102 tran = %d %d", status, temperature);
            chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateClusterState, reinterpret_cast<intptr_t>(nullptr));

            // trigger next reading after delay
            state = STATE_TRIGGER_READING;
            RequestProcess(read_period_ms);
            break;
        }
        default:
            break;
    }
}

} // namespace cluster_lib

#endif // CLUSTER_TMP102
