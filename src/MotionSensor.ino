/*
    MotionSensor - A MySensors Motion Sensor sketch for use with hacked Ikea Molgan.

    Created by Ivo Pullens, Emmission, 2016

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define MY_RADIO_NRF24
#define MY_BAUD_RATE                (57600)

#define MY_WITH_LEDS_BLINKING_INVERSE
#define MY_DEFAULT_TX_LED_PIN       (A5)

#include <MySensors.h>
#include <Vcc.h>

#define PIR_PIN       	            (3)

#define CHILD_ID_PIR                (0)

const float VccMin        = 1.8;             // Minimum expected Vcc level, in Volts: Brownout at 1.8V    -> 0%
const float VccMax        = 2.0 * 1.6;       // Maximum expected Vcc level, in Volts: 2xAAA fresh Alkaline -> 100%
const float VccCorrection = 1.0;             // Measured Vcc by multimeter divided by reported Vcc
static Vcc vcc(VccCorrection);

#define SENSOR_SLEEP_TIME_MS (24UL*60UL*60UL*1000UL)  // Maximum time between sensor wakeups, e.g. to check battery level.
#define PIR_BLIND_TIME_MS    (30UL*1000UL)            // Time PIR will not report new triggers after being triggered

void presentation()
{
  sendSketchInfo("Molgan", "1.0");
  present(CHILD_ID_PIR, S_MOTION);
}

void setup()
{
  pinMode(PIR_PIN, INPUT);
}

void loop()
{
  // Sleep until woken by interrupt or timer.
  bool pirTripped = sleep(digitalPinToInterrupt(PIR_PIN), CHANGE, SENSOR_SLEEP_TIME_MS) >= 0;

  // Only send tripped state. Tripped goes low again after a configurable time on the PIR sensor,
  // and it doesn't make sense to send this state to the gateway. Better save some battery juice.
  // If you still want to send the low level, then remove the if-statement.
  if (pirTripped)
  {
    send( MyMessage(CHILD_ID_PIR, V_TRIPPED).set(uint8_t(pirTripped)) );
  }

  // Try to determine battery level after sending sensor data, so battery will just have experienced some load.
  uint8_t batteryPcnt = static_cast<uint8_t>(0.5 + vcc.Read_Perc(VccMin, VccMax));

  // Battery readout should only go down. So report only when new value is smaller than previous one.
  static uint8_t oldBatteryPcnt = 200;
  if ( batteryPcnt < oldBatteryPcnt )
  {
    sendBatteryLevel(batteryPcnt);
    oldBatteryPcnt = batteryPcnt;
  }

  // Sleep for a short while, ignoring any new triggers.
  if (pirTripped)
  {
    sleep(PIR_BLIND_TIME_MS);
  }
}

