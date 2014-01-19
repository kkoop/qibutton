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

/*#include <iostream>
using namespace std;*/

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
   unsigned char command[32+3] = {0x0F, // write scratchpad
      m_rtcChanged ? 0x00 : 0x06, 0x02, // address
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
   unsigned char readspcommand[] = {0xAA}; // read scratchpad
   if (!m_ds9490->Write1W(readspcommand, 1)) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   unsigned char scratchpad[3+32];
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
   unsigned char copycommand[] = {0x99,  // copy scratchpad
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

bool DS1922::StartMission()
{
   unsigned char command[] = {0xCC, // start mission
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // dummy password (TODO: password)
      0xFF  // dummy byte
   };
   if (!m_ds9490->Write1W(command, sizeof(command))) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   return true;
}

bool DS1922::StopMission()
{
   unsigned char command[] = {0x33, // stop mission
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // dummy password (TODO: password)
      0xFF  // dummy byte
   };
   if (!m_ds9490->Write1W(command, sizeof(command))) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   return true;
}

bool DS1922::ClearMemory()
{
   unsigned char command[] = {0x96, // start mission
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // dummy password (TODO: password)
      0xFF  // dummy byte
   };
   if (!m_ds9490->Write1W(command, sizeof(command))) {
      m_lastError = m_ds9490->GetLastError();
      return false;
   }
   return true;
}

bool DS1922::ReadCalibration()
{
   if (!m_ds9490->DeviceOpen())
      if (!m_ds9490->OpenUsbDevice()) {
         m_lastError = m_ds9490->GetLastError();
         return false;
      }
   unsigned char page[32];
   if (!ReadMemPage(0x0240, page)) {
      return false;
   }
   // calculation according to datasheet
   double Tr1 = 60,   // for DS1922L
      Tr2 = page[0]/2.0-41 + page[1]/512.0,
      Tc2 = page[2]/2.0-41 + page[3]/512.0,
      Tr3 = page[4]/2.0-41 + page[5]/512.0,
      Tc3 = page[6]/2.0-41 + page[7]/512.0;
         
   double   Err2 = Tc2 - Tr2,
      Err3 = Tc3 - Tr3;
   double   Err1 = Err2; 
   
   m_calibration[1] = (Tr2*Tr2 - Tr1*Tr1)*(Err3-Err1)/((Tr2*Tr2-Tr1*Tr1)*(Tr3-Tr1)+(Tr3*Tr3-Tr1*Tr1)*(Tr1-Tr2));
   m_calibration[0] = m_calibration[1]*(Tr1-Tr2)/(Tr2*Tr2-Tr1*Tr1);
   m_calibration[2] = Err1 - m_calibration[0]*Tr1*Tr1 - m_calibration[1]*Tr1;
   m_calibrationValid = true;
   return true;
}

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
   unsigned char page[32];
   int missionSamples = GetSampleCount();
   if (size > missionSamples)
   {
      size = missionSamples;
   }
   int numPages = size/16 + (size%16 ? 1 : 0);
   for (int i=0; i<numPages; i++)
   {
      if (!ReadMemPage(0x1000+i*32, page))
      {
         return false;
      }
      for (int j=0; j<16 && i*16+j<size; j++)
      {
         buffer[i*16+j] = ConvertValue(page[0+j*2], page[1+j*2]); 
      }
   }  
   return true;
}

double DS1922::ConvertValue(unsigned char hiByte, unsigned char loByte)
{
   // convert to °C
   double result = hiByte/2.0-41 + loByte/512.0; 
   // apply calibration correction
   if (m_calibrationValid) {
      result -= m_calibration[0]*result*result + m_calibration[1]*result + m_calibration[2];
   }
   return result;
}

bool DS1922::ReadMemPage(short unsigned int address, unsigned char* buffer)
{
   unsigned char command[] = {0x69, // read memory
      address&0xFF, (address&0xFF00)>>8,
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
   return true;
}

int DS1922::GetSampleCount()
{
   return (m_statusRegister[0x22]<<16) | (m_statusRegister[0x21]<<8)
      | (m_statusRegister[0x20]);
}

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

int DS1922::GetSampleRate()
{
   return m_statusRegister[7]<<8 | m_statusRegister[6];
}

bool DS1922::GetRtcEnabled()
{
   return (m_statusRegister[0x12]&0x1)==0x1;
}

bool DS1922::GetHighspeedSample()
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
   return m_statusRegister[0x08]/2.0 - 41;
}

double DS1922::GetAlarmHighThreshold()
{
   return m_statusRegister[0x09]/2.0 - 41;
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
   m_statusRegister[7] = (rate&0x3f00)>>8;
   m_statusRegister[6] = rate&0xff;
}

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
   m_statusRegister[0x08] = (unsigned char)((temp+41)*2);
}

void DS1922::SetAlarmHighThreshold(double temp)
{
   m_statusRegister[0x09] = (unsigned char)((temp+41)*2);
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
