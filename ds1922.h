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


#ifndef DS1922_H
#define DS1922_H
#include <string>
#include <cstdint>

class DS9490;

/**
 * @brief Represents a Maxim DS1922 1-Wire temperature sensor
 * 
 * This class can be used together with DS9490 to read from and write to the DS1922 temperature logger.
 * As most configuration data is stored in two memory pages, all configuration is read and written at once,
 * using the functions ReadRegister() and WriteRegister(). After reading, the configuration settings are 
 * available using the individual Get...() functions, and can be modified using the respective Set...()
 * functions. Only after calling WriteRegister() are the modified configurations written to the device.
 * Functions that interact with the hardware return false on error, and the error message can be retrieved 
 * using GetLastError().
 */
class DS1922
{

public:
    DS1922(DS9490* ds9490);
    ~DS1922();
    
public:
   std::string GetLastError() {return m_lastError;}
   bool ReadRegister();
   bool WriteRegister();
   bool ReadData(double* buffer, int size);
   bool StartMission();
   bool StopMission();
   bool ClearMemory();
   
   int GetSampleCount();      // only valid after successful ReadRegister
   int GetDeviceSampleCount();// "
   void GetRtc(tm* time);     // "
   void GetMissionTimestamp(tm* time); // "
   int GetSampleRate();       // "
   bool GetRtcEnabled();
   bool GetHighspeedSampling(); // "
   bool GetAlarmLowEnabled(); // "
   bool GetAlarmHighEnabled();// "
   double GetAlarmLowThreshold(); // "
   double GetAlarmHighThreshold(); // "
   bool GetAlarmLow();        // "
   bool GetAlarmHigh();       // "
   bool GetPasswordEnabled(); // "
   bool GetMissionInProgress(); // "
   bool GetWaitingForAlarm(); // "
   bool GetLoggingEnabled();  // "
   bool GetHighResLogging();  // "
   bool GetRollover();        // "
   bool GetStartUponAlarm();  // "
   int GetMissionStartDelay();// "
   enum Type {DS1922L, DS1922T, DS1922E, Other};
   DS1922::Type GetType();
   
   void SetRtc(tm* time);
   void SetSampleRate(int rate);
   void SetAlarmEnabled(bool tempLow, bool tempHigh);
   void SetAlarmLowThreshold(double temp);
   void SetAlarmHighThreshold(double temp);
   void SetRtcEnabled(bool enabled);
   void SetRtcHighspeed(bool highspeed);
   void SetLoggingEnabled(bool enabled);
   void SetHighResLogging(bool highres);
   void SetRollover(bool rollover);
   void SetStartUponAlarm(bool startAlarm);
   void SetMissionStartDelay(int delay);
   // TODO: void SetPasswords()
   
protected:
   bool ReadMemPage(uint16_t address, uint8_t* buffer);
   bool VerifyCrc(uint8_t* data, int length);
   double ConvertValue(uint8_t hiByte, uint8_t loByte);
   bool ReadCalibration();
   
   // Data
private:
   DS9490* m_ds9490;
   std::string m_lastError;
   uint8_t m_statusRegister[32*2];
   bool m_statusRegisterValid;
   bool m_rtcChanged;
   double m_calibration[3];
   bool m_calibrationValid;
};

#endif // DS1922_H
