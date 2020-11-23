/*
    QIButton: read DS1922 IButton via DS9490B USB 1-Wire
    Copyright (C) 2014  Karsten Koop <karsten.koop@gmx.de>

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


#include "ds1922.h"
#include "ds9490.h"
#include <string.h>


/**
 * @brief Constructor
 * 
 * @param DS9490* ds9490: Pointer to an instance of class DS9490, which needs to have a lifetime longer than this object.
 */
DS1922::DS1922(DS9490* ds9490)
{
   m_ds9490 = ds9490;
   m_statusRegisterValid = false;
   m_calibrationValid = false;
   m_rtcChanged = false;
}

DS1922::~DS1922()
{

}

/**
 * @brief Read configuration memory pages
 * 
 * After reading the configuration memory pages, individual settings are available using the Get...() functions.
 * 
 * @return bool: true on success, false on error. On error, the error message is available from GetLastError().
 */
bool DS1922::ReadRegister()
{
   if (!m_ds9490->DeviceOpen())
      if (!m_ds9490->OpenUsbDevice()) {
         m_lastError = m_ds9490->GetLastError();
         return false;
      }

   if (ReadMemPage(0x0200, m_statusRegister)
         && ReadMemPage(0x0220, m_statusRegister+32))
   {
      m_statusRegisterValid = true;
      m_rtcChanged = false;
      return true;
   }
      
   return false;
}

/**
 * @brief Writes the configuration memory pages
 * 
 * After setting all configuration registers using the Set...() function of this class,
 * this function can be used to write these settings to the device.
 * @return bool: true on success, false on error.
 */
bool DS1922::WriteRegister()
{
   if (!m_statusRegisterValid) {
      m_lastError = "no valid data to write";
      return false;
   }
   // 1st page: write complete if clock was changed, if not,
   // skip first 6 bytes
   // procedure: 1. write data to scratchpad, 
   //            2. verify scratchpad, 
   //            3. copy scratchpad to target address
   uint8_t command[32+3] = {0x0F, // write scratchpad
      (uint8_t)(m_rtcChanged ? 0x00 : 0x06), 0x02, // address
   };
   if (m_rtcChanged) {
      memcpy(command+3, m_statusRegister, 32);
   } else {
      memcpy(command+3, m_statusRegister+6, 32-6);
   }
   if (!m_ds9490->Write1W(command, 3+32-(m_rtcChanged?0:6))) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   // read scratchpad to verify
   uint8_t readspcommand[] = {0xAA}; // read scratchpad
   if (!m_ds9490->Write1W(readspcommand, 1)) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   uint8_t scratchpad[3+32];
   if (!m_ds9490->Read1W(scratchpad, 3+32)) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   // verify:
   if (scratchpad[0]!=command[1] || scratchpad[1]!=command[2]) {
      m_lastError = "read Scratchpad target address wrong";
      return false;
   }
   for (int i=0; i<(m_rtcChanged?32:(32-6)); i++) {
      if (scratchpad[i+3]!=command[i+3]) {
         m_lastError = "read Scratchpad data wrong";
         return false;
      }
   }
   // copy scratchpad 
   uint8_t copycommand[] = {0x99,  // copy scratchpad
      scratchpad[0], scratchpad[1], scratchpad[2], // verification code
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // dummy password (TODO: password)
   };
   if (!m_ds9490->Write1W(copycommand, sizeof(copycommand))) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   sleep(1);
   // check AA bit
   if (!m_ds9490->Write1W(readspcommand, 1)) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   if (!m_ds9490->Read1W(scratchpad, 3+32)) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   if (!(scratchpad[2]&0x80)) {
      m_lastError = "copy Scratchpad: AA bit not 1";
      return false;
   }
   
   // 2nd page
   // write scratchpad
   command[1] = 0x20;   // address
   command[2] = 0x02;   // "
   memcpy(command+3, m_statusRegister+32, 32);
   if (!m_ds9490->Write1W(command, 3+32)) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   // read scratchpad to verify
   if (!m_ds9490->Write1W(readspcommand, 1)) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   if (!m_ds9490->Read1W(scratchpad, 3+32)) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   // verify:
   if (scratchpad[0]!=command[1] || scratchpad[1]!=command[2]) {
      m_lastError = "read Scratchpad target address wrong";
      return false;
   }
   for (int i=0; i<32; i++) {
      if (scratchpad[i+3]!=command[i+3]) {
         m_lastError = "read Scratchpad data wrong";
         return false;
      }
   }
   // copy scratchpad 
   copycommand[1] = scratchpad[0];  // verification code
   copycommand[2] = scratchpad[1];  // "
   copycommand[3] = scratchpad[2];  // "
   if (!m_ds9490->Write1W(copycommand, sizeof(copycommand))) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   sleep(1);
   // check AA bit
   if (!m_ds9490->Write1W(readspcommand, 1)) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   if (!m_ds9490->Read1W(scratchpad, 3+32)) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   if (!(scratchpad[2]&0x80)) {
      m_lastError = "copy Scratchpad: AA bit not 1";
      return false;
   }   
   
   return true;
}

/**
 * @brief Starts a logging mission.
 * 
 * See the datasheet of the DS1922 for details about missions.
 */
bool DS1922::StartMission()
{
   uint8_t command[] = {0xCC, // start mission
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // dummy password (TODO: password)
      0xFF  // dummy byte
   };
   if (!m_ds9490->Write1W(command, sizeof(command))) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   return true;
}

/**
 * @brief Stops a logging mission.
 */
bool DS1922::StopMission()
{
   uint8_t command[] = {0x33, // stop mission
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // dummy password (TODO: password)
      0xFF  // dummy byte
   };
   if (!m_ds9490->Write1W(command, sizeof(command))) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   return true;
}

/**
 * @brief Clears the logging memory.
 */
bool DS1922::ClearMemory()
{
   uint8_t command[] = {0x96, // clear memory
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // dummy password (TODO: password)
      0xFF  // dummy byte
   };
   if (!m_ds9490->Write1W(command, sizeof(command))) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   return true;
}

/**
 * @brief Read calibration data
 * 
 * Calibration data is included as coefficients of a correction polynomial, and set during production of
 * the device, see the datasheet for details.
 */
bool DS1922::ReadCalibration()
{
   if (GetType()==DS1922E) // DS1922 does not support calibration
      return false;
   if (!m_ds9490->DeviceOpen())
      if (!m_ds9490->OpenUsbDevice()) {
         m_lastError = m_ds9490->GetLastError();
         return false;
      }
   uint8_t page[32];
   if (!ReadMemPage(0x0240, page)) {
      return false;
   }
   // calculation according to datasheet
   int tempOffset = 1;
   double Tr1 = 90;
   if (GetType()==DS1922L) {
      tempOffset = 41;
      Tr1 = 60;
   }
   double Tr2 = page[0]/2.0-tempOffset + page[1]/512.0,
      Tc2 = page[2]/2.0-tempOffset + page[3]/512.0,
      Tr3 = page[4]/2.0-tempOffset + page[5]/512.0,
      Tc3 = page[6]/2.0-tempOffset + page[7]/512.0;
         
   double   Err2 = Tc2 - Tr2,
      Err3 = Tc3 - Tr3;
   double   Err1 = Err2; 
   
   m_calibration[1] = (Tr2*Tr2 - Tr1*Tr1)*(Err3-Err1)/((Tr2*Tr2-Tr1*Tr1)*(Tr3-Tr1)+(Tr3*Tr3-Tr1*Tr1)*(Tr1-Tr2));
   m_calibration[0] = m_calibration[1]*(Tr1-Tr2)/(Tr2*Tr2-Tr1*Tr1);
   m_calibration[2] = Err1 - m_calibration[0]*Tr1*Tr1 - m_calibration[1]*Tr1;
   m_calibrationValid = true;
   return true;
}

/**
 * @brief Read the logged data into buffer
 * 
 * The number of samples read is determined by the number of available values and
 * @p size. The values returned in @p buffer are in °C.
 */
bool DS1922::ReadData(double* buffer, int size)
{
   if (!m_statusRegisterValid) {
      m_lastError = "Read register first";
      return false;
   }
   if (!m_calibrationValid) {
      ReadCalibration();
   }
   // read min(numValues,missionSampleCount) values into the buffer
   // converted to °C
   // read one pages at a time  
   uint8_t page[32];
   int missionSamples = GetSampleCount();
   if (size > missionSamples)
   {
      size = missionSamples;
   }
   if (GetHighResLogging()) { // 2 byte per value
      int numPages = size/16 + (size%16 ? 1 : 0);
      for (int i=0; i<numPages; i++)
      {
         if (!ReadMemPage(0x1000+i*32, page)) {
            return false;
         }
         for (int j=0; j<16 && i*16+j<size; j++)
         {
            buffer[i*16+j] = ConvertValue(page[0+j*2], page[1+j*2]);
         }
      }  
   } else { // 1 byte per value
      int numPages = size/32 + (size%32 ? 1 : 0);
      for (int i=0; i<numPages; i++)
      {
         if (!ReadMemPage(0x1000+i*32, page)) {
            return false;
         }
         for (int j=0; j<32 && i*32+j<size; j++)
         {
            buffer[i*32+j] = ConvertValue(page[j], 0);
         }
      }  
   }
   return true;
}

double DS1922::ConvertValue(uint8_t hiByte, uint8_t loByte)
{
   // convert to °C
   int tempOffset = 1;
   if (GetType()==DS1922L)
      tempOffset = 41;
   double result = hiByte/2.0-tempOffset + loByte/512.0; 
   // apply calibration correction
   if (m_calibrationValid) {
      result -= m_calibration[0]*result*result + m_calibration[1]*result + m_calibration[2];
   }
   return result;
}

bool DS1922::ReadMemPage(uint16_t address, uint8_t* buffer)
{
   uint8_t command[] = {0x69, // read memory
      (uint8_t)(address&0xFF), (uint8_t)((address&0xFF00)>>8),
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // dummy password (TODO: password)
   };
   if (!m_ds9490->Write1W(command, sizeof(command))) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   if (!m_ds9490->Read1W(buffer, 32)) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   uint8_t crcdata[3+32+2] = {0x69,
      (uint8_t)(address&0xFF), (uint8_t)((address&0xFF00)>>8)};
   memcpy(crcdata+3, buffer, 32);
   if (!m_ds9490->Read1W(crcdata+3+32, 2)) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   if (!VerifyCrc(crcdata, 3+32+2)) {
      m_lastError = "Wrong CRC reading data";
      return false;
   }
   return true;
}

bool DS1922::VerifyCrc(uint8_t* data, int length)
{  // the CRC to verify has to be the last 2 bytes of data
   const uint8_t oddparity[] = {0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0};

   uint16_t crcReg = 0;
   for (int i=0; i<length; i++) 
   {
      uint16_t dbyte = (data[i]^ (crcReg&0xff) )&0xff;
      crcReg >>= 8;
      if (oddparity[dbyte&0xf]^oddparity[dbyte>>4]) {
         crcReg ^= 0xc001;
      }
      dbyte <<= 6;
      crcReg ^= dbyte;
      dbyte <<= 1;
      crcReg ^= dbyte;
   }
   return crcReg==0xB001;
}

/**
 * @brief Number of samples in current mission
 */
int DS1922::GetSampleCount()
{
   return (m_statusRegister[0x22]<<16) | (m_statusRegister[0x21]<<8)
      | (m_statusRegister[0x20]);
}

/**
 * @brief Total number of samples created by this device
 * 
 * This value is not reset when a new mission starts. It can be used as measure of the devices' total use time,
 * and to estimate the remaining battery life.
 */
int DS1922::GetDeviceSampleCount()
{
   return m_statusRegister[0x25]<<16 | m_statusRegister[0x24]<<8
      | m_statusRegister[0x23];
}

void DS1922::GetRtc(tm* time)
{
   time->tm_year  = 100+((m_statusRegister[5] & 0xf0)>>4) * 10 + (m_statusRegister[5] & 0x0f);
   time->tm_mon   = ((m_statusRegister[4] & 0xf0)>>4) * 10 + (m_statusRegister[4] & 0x0f)-1;
   time->tm_mday  = ((m_statusRegister[3] & 0xf0)>>4) * 10 + (m_statusRegister[3] & 0x0f);
   time->tm_hour  = ((m_statusRegister[2] & 0xf0)>>4) * 10 + (m_statusRegister[2] & 0x0f);
   time->tm_min   = ((m_statusRegister[1] & 0xf0)>>4) * 10 + (m_statusRegister[1] & 0x0f);
   time->tm_sec   = ((m_statusRegister[0] & 0xf0)>>4) * 10 + (m_statusRegister[0] & 0x0f);
}

void DS1922::GetMissionTimestamp(tm* time)
{
   time->tm_year  = 100+((m_statusRegister[0x1e] & 0xf0)>>4) * 10 + (m_statusRegister[0x1e] & 0x0f);
   time->tm_mon   = ((m_statusRegister[0x1d] & 0xf0)>>4) * 10 + (m_statusRegister[0x1d] & 0x0f)-1;
   time->tm_mday  = ((m_statusRegister[0x1c] & 0xf0)>>4) * 10 + (m_statusRegister[0x1c] & 0x0f);
   time->tm_hour  = ((m_statusRegister[0x1b] & 0xf0)>>4) * 10 + (m_statusRegister[0x1b] & 0x0f);
   time->tm_min   = ((m_statusRegister[0x1a] & 0xf0)>>4) * 10 + (m_statusRegister[0x1a] & 0x0f);
   time->tm_sec   = ((m_statusRegister[0x19] & 0xf0)>>4) * 10 + (m_statusRegister[0x19] & 0x0f);
}

/**
 * @brief Returns the time between to samples
 * 
 * Unit is seconds if GetHighspeedSampling() returns true, otherwise it's minutes.
 */
int DS1922::GetSampleRate()
{
   return m_statusRegister[7]<<8 | m_statusRegister[6];
}

/**
 * @brief Returns wether the RTC oscillator is enabled
 * 
 * Switching the RTC off when no mission is active saves battery.
 */
bool DS1922::GetRtcEnabled()
{
   return (m_statusRegister[0x12]&0x1)==0x1;
}

bool DS1922::GetHighspeedSampling()
{
   return (m_statusRegister[0x12]&0x2)==0x2;
}

bool DS1922::GetAlarmLowEnabled()
{
   return (m_statusRegister[0x10]&0x1)==0x1;
}

bool DS1922::GetAlarmHighEnabled()
{
   return (m_statusRegister[0x10]&0x2)==0x2;
}

double DS1922::GetAlarmLowThreshold()
{
   int tempOffset = 1;
   if (GetType()==DS1922L)
      tempOffset = 41;

   return m_statusRegister[0x08]/2.0 - tempOffset;
}

double DS1922::GetAlarmHighThreshold()
{
   int tempOffset = 1;
   if (GetType()==DS1922L)
      tempOffset = 41;

   return m_statusRegister[0x09]/2.0 - tempOffset;
}

bool DS1922::GetAlarmLow()
{
   return (m_statusRegister[0x14]&0x1)==0x1;
}

bool DS1922::GetAlarmHigh()
{
   return (m_statusRegister[0x14]&0x2)==0x2;
}

bool DS1922::GetPasswordEnabled()
{
   return m_statusRegister[0x27]==0xaa;
}

bool DS1922::GetMissionInProgress()
{
   return (m_statusRegister[0x15]&0x2)==0x2;
}

bool DS1922::GetWaitingForAlarm()
{
   return (m_statusRegister[0x15]&0x8)==0x8;
}

bool DS1922::GetLoggingEnabled()
{
   return (m_statusRegister[0x13]&0x1)==0x1;
}

bool DS1922::GetHighResLogging()
{
   return (m_statusRegister[0x13]&0x4)==0x4;
}

/**
 * @brief Returns the value of the Rollover Control bit
 * 
 * With rollover enabled, the device overwrites old data once the logging memory is full. If disabled,
 * data logging stops on full memory.
 * The calculation of the timestamps of logged values has to take rollover into account.
 */
bool DS1922::GetRollover()
{
   return (m_statusRegister[0x13]&0x10)==0x10;
}

bool DS1922::GetStartUponAlarm()
{
   return (m_statusRegister[0x13]&0x20)==0x20;
}

int DS1922::GetMissionStartDelay()
{
   return m_statusRegister[0x18]<<16 | m_statusRegister[0x17]<<8
      | m_statusRegister[0x16];

}

void DS1922::SetRtc(tm* time)
{
   m_statusRegister[5] = ((time->tm_year-100)/10)<<4 | (time->tm_year-100)%10;
   m_statusRegister[4] = ((time->tm_mon+1)/10)<<4 | (time->tm_mon+1)%10;
   m_statusRegister[3] = (time->tm_mday/10)<<4 | time->tm_mday%10;
   m_statusRegister[2] = (time->tm_hour/10)<<4 | time->tm_hour%10;
   m_statusRegister[1] = (time->tm_min/10)<<4 | time->tm_min%10;
   m_statusRegister[0] = (time->tm_sec/10)<<4 | time->tm_sec%10;
   
   m_rtcChanged = true;
}

void DS1922::SetSampleRate(int rate)
{
   if ( (rate&0x3fff) > 0)
   {  // Datasheet: Setting rate=0 results in unrecoverable state
      m_statusRegister[7] = (rate&0x3f00)>>8;
      m_statusRegister[6] = rate&0xff;
   }
}

/**
 * @brief Enables or disables temperature threshold start conditions
 */
void DS1922::SetAlarmEnabled(bool tempLow, bool tempHigh)
{
   if (tempLow)
      m_statusRegister[0x10] |= 1;
   else
      m_statusRegister[0x10] &= ~1;
   if (tempHigh)
      m_statusRegister[0x10] |= 2;
   else
      m_statusRegister[0x10] &= ~2;
}

void DS1922::SetAlarmLowThreshold(double temp)
{
   int tempOffset = 1;
   if (GetType()==DS1922L)
      tempOffset = 41;

   m_statusRegister[0x08] = (uint8_t)((temp+tempOffset)*2);
}

void DS1922::SetAlarmHighThreshold(double temp)
{
   int tempOffset = 1;
   if (GetType()==DS1922L)
      tempOffset = 41;

   m_statusRegister[0x09] = (uint8_t)((temp+tempOffset)*2);
}

void DS1922::SetRtcEnabled(bool enabled)
{
   if (enabled)
      m_statusRegister[0x12] |= 1;
   else
      m_statusRegister[0x12] &= ~1;
}

void DS1922::SetRtcHighspeed(bool highspeed)
{
   if (highspeed)
      m_statusRegister[0x12] |= 2;
   else
      m_statusRegister[0x12] &= ~2;
}

void DS1922::SetLoggingEnabled(bool enabled)
{
   if (enabled)
      m_statusRegister[0x13] |= 1;
   else
      m_statusRegister[0x13] &= ~1;
}

void DS1922::SetHighResLogging(bool highres)
{
   if (highres)
      m_statusRegister[0x13] |= 4;
   else
      m_statusRegister[0x13] &= ~4;
}

void DS1922::SetRollover(bool rollover)
{
   if (rollover)
      m_statusRegister[0x13] |= 0x10;
   else
      m_statusRegister[0x13] &= ~0x10;
}

void DS1922::SetStartUponAlarm(bool startAlarm)
{
   if (startAlarm)
      m_statusRegister[0x13] |= 0x20;
   else
      m_statusRegister[0x13] &= ~0x20;
}

void DS1922::SetMissionStartDelay(int delay)
{
   m_statusRegister[0x18] = (delay&0xff0000)>>16;
   m_statusRegister[0x17] = (delay&0xff00)>>8;
   m_statusRegister[0x16] = (delay&0xff)>>0;
}

DS1922::Type DS1922::GetType()
{
   if (m_statusRegister[0x26]==0x40)
      return DS1922L;
   if (m_statusRegister[0x26]==0x60)
      return DS1922T;
   if (m_statusRegister[0x26]==0x80)
      return DS1922E;
   return Other;
}
