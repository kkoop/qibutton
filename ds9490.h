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


#ifndef DS2490_H
#define DS2490_H

#include <string>
#include <list>
#include <usb.h>

/**
 * @brief Represents a Maxim DS9490 USB 1-Wire reader
 * 
 * This class handles the communication with the USB 1-Wire reader using libusb. It includes functions to scan for devices
 * on the 1-Wire bus, read from and write to the found devices, and reset the bus. On error, these functions return false,
 * and the error message can be retrieved using GetLastError().
 */
class DS9490
{
public:
   DS9490();
   ~DS9490();
   
public:
   std::string GetLastError() {return m_lastError;}
   bool OpenUsbDevice();
   bool DeviceOpen() {return m_usbDevHandle!=NULL;}
   bool Scan1WBus(std::list<uint64_t>& serials);
   bool Read1W(uint8_t* buffer, uint length);
   bool Write1W(uint8_t* buffer, uint length);
   bool Reset1W();
protected:
   bool AquireUsb(struct usb_device* dev);
   bool Release();
   bool ReadByte(uint8_t& read);
   bool WriteByte(uint8_t data);
   bool TouchByte(uint8_t write, uint8_t& read);
   bool TouchBit(uint8_t write, uint8_t& read);
   
   // Data
private:
   std::string m_lastError;
   usb_dev_handle* m_usbDevHandle;
   
   // codes from DS2490 datasheet
   enum Commands {CONTROL_CMD=0x00,COMM_CMD=0x01,MODE_CMD=0x02,
         TEST_CMD=0x03};
   enum Ctls {CTL_RESET_DEVICE=0x00,CTL_START_EXE=0x01,CTL_RESUME_EXE=0x02,
         CTL_HALT_EXE_IDLE=0x03,CTL_HALT_EXE_DONE=0x04,CTL_FLUSH_COMM_CMDS=0x07,
         CTL_FLUSH_CV_BUFFER=0x08,CTL_FLUSH_CMT_BUFFER=0x09,
         CTL_GET_COMM_CMDS=0x0A};
   enum Modes {MOD_PULSE_EN=0x00,MOD_SPEED_CHANGE_EN=0x01,MOD_1WIRE_SPEED=0x02,
         MOD_STRONG_PU_DURATION=0x03,MOD_PULLDOWN_SLEWRATE=0x04,
         MOD_PROG_PULSE_DURATION=0x05,MOD_WRITE1_LOWTIME=0x06,
         MOD_DSOW0_TREC=0x07};

   static const int m_timeout=5000;
};

#endif // DS2490_H
