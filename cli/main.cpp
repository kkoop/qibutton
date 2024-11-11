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
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>

#include "ds1922.h"
#include "ds9490.h"

using namespace std;

int main(int argc, char **argv) 
{
   setlocale(LC_ALL,"");
   int optCount = 0;
   bool scan=false, config=false, data=false;
   int arg;
   while ( (arg=getopt(argc, argv, "scd")) !=-1) {
      optCount++;
      switch (arg) {
         case 's':
            scan = true;
            break;
         case 'c':
            config = true;
            break;
         case 'd':
            data = true;
            break;
         case '?':
         case 'h':
            cout << "Usage: ibutton [-s] [-c] [-d]\n"
                 << "  -s: Scan 1W bus\n"
                 << "  -c: Read config\n"
                 << "  -d: Read data\n"
                 << " (default: -cd)" << endl;
            return 1;
      }
   }
   if (optCount==0) {
      config = data = true;
   }
   
   DS9490 ds9490;
   DS1922 ds1922(&ds9490);
   
   if (!ds9490.OpenUsbDevice()) {
      cerr << ds9490.GetLastError() << endl;
      return 1;
   }
   
   if (scan) {
      list<uint64_t> serials;
      if (!ds9490.Scan1WBus(serials)) {
         cout << ds9490.GetLastError() << endl;
      } else {
         for (list<uint64_t>::iterator it=serials.begin(); it!=serials.end(); ++it)
         {
            printf("Found device %016lx\n", *it);
         }
      }
   }

   if (!ds1922.ReadRegister()) {
      cout << ds1922.GetLastError() << endl;
      return 1;
   }
   if (config) {
      tm time;
      ds1922.GetRtc(&time);
      cout << "Clock enabled: " << ds1922.GetRtcEnabled() << endl;
      time_t tt = mktime(&time);
      char buffer[64];
      strftime(buffer, 64, "%x %X", localtime(&tt));
      printf("Clock: %s\n", buffer);
      cout << "Mission in progress: " << ds1922.GetMissionInProgress() << endl;
      cout << "Sample rate: " << ds1922.GetSampleRate() << endl;
      cout << "Sample rate high res: " << ds1922.GetHighResLogging() << endl;
      cout << "Sample count: " << ds1922.GetSampleCount() << endl;
      cout << "Device sample count: " << ds1922.GetDeviceSampleCount() << endl;
      cout << "Start upon alarm: " << ds1922.GetStartUponAlarm() << endl;
      cout << "Alarm activated: low temp: " << ds1922.GetAlarmLow()
         << ", high temp: " << ds1922.GetAlarmHigh() << endl;
      cout << "Alarm low temp: " << ds1922.GetAlarmLow()
         << ", high temp: " << ds1922.GetAlarmHigh() << endl;
      cout << "Waiting for alarm: " << ds1922.GetWaitingForAlarm() << endl;
      cout << "Logging enabled: " << ds1922.GetLoggingEnabled() << endl;
      cout << "Rollover: " << ds1922.GetRollover() << endl;
      cout << "Mission start delay: " << ds1922.GetMissionStartDelay() << endl;
      ds1922.GetMissionTimestamp(&time);
      tt = mktime(&time);
      strftime(buffer, 64, "%x %X", localtime(&tt));
      printf("Mission timestamp: %s\n", buffer);
   }
   if (config && data)
      cout << "-Data--------------------------------------------" << endl;
   
   if (data) {
      tm time;
      ds1922.GetMissionTimestamp(&time);
      time_t tt = mktime(&time);
      // handle rollover: the oldest value may not be the first one
      int missionSamples = ds1922.GetSampleCount();
      int maxMissionSamples = (ds1922.GetHighResLogging() ? 4096 : 8192);
      int posOldestValue = missionSamples % maxMissionSamples;
      int sampleRate = ds1922.GetSampleRate();
      if (!ds1922.GetHighspeedSampling()) {
         sampleRate *= 60;
      }
      if (missionSamples>maxMissionSamples) {
         tt += sampleRate*(missionSamples-maxMissionSamples);
         missionSamples = maxMissionSamples;
      } else {
         posOldestValue = 0;
      }        
      double values[missionSamples];
      if (!ds1922.ReadData(values, missionSamples)) {
         cout << ds1922.GetLastError() << endl;
      } else {
         for (int i=0; i<missionSamples; i++) {
            char buffer[64];
            strftime(buffer, 64, "%F %X", localtime(&tt));
            cout << buffer << ": " << values[(i+posOldestValue)%maxMissionSamples] << endl;
            tt += sampleRate;
         }
      }
   }
   return 0;
}
