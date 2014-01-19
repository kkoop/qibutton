#include "mainwindow.h"
#include "../ds1922.h"
#include "../ds9490.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
 : QMainWindow(parent)
{
   setupUi(this);
   m_ds9490 = new DS9490;
   m_ds1922 = new DS1922(m_ds9490);
}


MainWindow::~MainWindow()
{
   delete m_ds1922;
   delete m_ds9490;
}

void MainWindow::onReadConfig()
{
   if (!m_ds9490->DeviceOpen())
      if (!m_ds9490->OpenUsbDevice()) {
         QMessageBox::critical(this, "Error", tr("Error opening USB device:\n")+m_ds9490->GetLastError().c_str());
         return;
      }
      
   if (!m_ds1922->ReadRegister()) {
     QMessageBox::critical(this, "Error", tr("Error reading config:\n")+m_ds1922->GetLastError().c_str());
     return;
   }
   // set all config settings 
   tm rtc;
   m_ds1922->GetRtc(&rtc);
   rtcEdit->setDate(QDate(rtc.tm_year+1900, rtc.tm_mon+1, rtc.tm_mday));
   rtcEdit->setTime(QTime(rtc.tm_hour, rtc.tm_min, rtc.tm_sec));
   
}

void MainWindow::onReadData()
{
}

void MainWindow::onWriteConfig()
{
}

void MainWindow::onStopMission()
{
}

void MainWindow::onClearData()
{
}

void MainWindow::onStartMission()
{
}