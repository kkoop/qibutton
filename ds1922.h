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

class DS9490;

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
   enum Type {DS1922L, DS1922T, Other};
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
   bool ReadMemPage(unsigned short address, unsigned char* buffer);
   double ConvertValue(unsigned char hiByte, unsigned char loByte);
   bool ReadCalibration();
   
   // Data
protected:
   DS9490* m_ds9490;
   std::string m_lastError;
   unsigned char m_statusRegister[32*2];
   bool m_statusRegisterValid;
   bool m_rtcChanged;
   double m_calibration[3];
   bool m_calibrationValid;
};

#endif // DS1922_H
