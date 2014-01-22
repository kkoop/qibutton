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


#include "ds9490.h"
#include <stdio.h>

#include <iostream>
using namespace std;

DS9490::DS9490()
{
   m_usbDevHandle = NULL;
}

DS9490::~DS9490()
{
   if (m_usbDevHandle)
   {
      Release();
   }  
}

bool DS9490::OpenUsbDevice()
{
   struct usb_bus *bus;
   struct usb_device *dev;

   // initialize USB subsystem
   usb_init();
   usb_set_debug(0);
   usb_find_busses();
   usb_find_devices();

   for (bus = usb_busses; bus; bus = bus->next) {
      for (dev = bus->devices; dev; dev = dev->next) {
         if (dev->descriptor.idVendor == 0x04FA &&
               dev->descriptor.idProduct == 0x2490) {
            // take the first one, support only one DS9490B 
            return AquireUsb(dev);
         }
      }
   }

   m_lastError = "No DS2490 found";
   return false;
}

bool DS9490::AquireUsb(struct usb_device* dev)
{
   if (m_usbDevHandle!=NULL)
   {
      Release();
   }
   m_usbDevHandle = usb_open(dev);
   if (m_usbDevHandle==NULL)
   {
      m_lastError = "Failed to open USB device";
      return false;
   }
   if (usb_set_configuration(m_usbDevHandle, 1)!=0)
   {
      m_lastError = "Failed to set configuration";
      Release();
      return false;
   }
   if (usb_claim_interface(m_usbDevHandle, 0)!=0)
   {
      m_lastError = "Failed to claim interface";
      Release();
      return false;
   }
   if (usb_set_altinterface(m_usbDevHandle, 3)!=0)
   {
      m_lastError = "Failed to set altinterface\n";
      Release();
      return false;
   }
   return true;
}

bool DS9490::Release()
{
   usb_release_interface(m_usbDevHandle, 0);
   usb_close(m_usbDevHandle);
   m_usbDevHandle = NULL;
   return true;
}

bool DS9490::Scan1WBus()
{
   if (!DeviceOpen()) {
      m_lastError = "Device not open";
      return false;
   }
   return false;
}

bool DS9490::GetSerialNumber(uint64_t& serial)
{
   // TODO:
   return false;
}

bool DS9490::Read1W(unsigned char* buffer, uint length)
{
   if (!DeviceOpen()) {
      m_lastError = "Device not open";
      return false;
   }
   for (int i=0; i<length; i++) {
      // TODO: use block transfer
      if (!ReadByte(buffer[i]))
         return false;
   }
   return true;
}

bool DS9490::Write1W(unsigned char* buffer, uint length)
{
   if (!DeviceOpen()) {
      m_lastError = "Device not open";
      return false;
   }
   if (!Reset1W())
      return false;
   if (!WriteByte(0xCC))   // skip ROM
      return false;
   
   for (int i=0; i<length; i++)
   {
      if (!WriteByte(buffer[i]))
         return false;
   }
   return true;
}

bool DS9490::Reset1W()
{
   if (!DeviceOpen()) {
      m_lastError = "Device not open";
      return false;
   }
   int result = usb_control_msg(m_usbDevHandle, 0x40, COMM_CMD, 
                                0x0043, 0, NULL, 0, m_timeout);
   if (result!=0) {
      m_lastError = "Error writing USB command";
      return false;
   }

   char buffer[32];
   // read status until unit is idle
   do {
      result = usb_bulk_read(m_usbDevHandle, 0x81, // EP1: control
                             buffer, 0x20, m_timeout);
   } while(!(buffer[0x08] & 0x20) && (result>=0));
   if (result<0) {
      m_lastError = "Error reading device status";
      return false;
   }
   
   return true;
}

bool DS9490::ReadByte(unsigned char& read)
{
   return TouchByte(0xFF, read);
}

bool DS9490::WriteByte(unsigned char data)
{
   unsigned char read;
   if (TouchByte(data, read))
   {
      if (read!=data) {
         m_lastError = "WriteByte: read data != written data";
         return false;
      }
      return true;
   }
   return false;
}

bool DS9490::TouchByte(unsigned char write, unsigned char& read)
{
   int result = usb_control_msg(m_usbDevHandle, 0x40, COMM_CMD, 
                                0x0053, write, NULL, 0, m_timeout);
   if (result!=0) {
      m_lastError = "Error writing USB command";
      return false;
   }
   
   char buffer[32];
   // read status until unit is idle
   do {
      result = usb_bulk_read(m_usbDevHandle, 0x81, // EP1: control
                             buffer, 0x20, m_timeout);
   } while(!(buffer[0x08] & 0x20) && (result>=0));
   if (result<0) {
      m_lastError = "Error reading device status";
      return false;
   }
  
   // read data
   result = usb_bulk_read(m_usbDevHandle, 0x83, // EP3: bulk read
                           buffer, 1, m_timeout);
   read = buffer[0];
   if (result<0) {
      m_lastError = "Error reading data";
      return false;
   }
   return true;
}
