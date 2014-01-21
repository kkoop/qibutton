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
   rtcEnabledCheckBox->setChecked(m_ds1922->GetRtcEnabled());
   
   sampleRateSpinBox->setValue(m_ds1922->GetSampleRate());
   sampleRateSecondsRadioButton->setChecked(m_ds1922->GetHighspeedSampling());
   sampleRateMinutesRadioButton->setChecked(!m_ds1922->GetHighspeedSampling());
   
   if(m_ds1922->GetAlarmLowEnabled()) {
      lowAlarmLabel->setText("yes");
   }
   
   if(!m_ds1922->GetAlarmLowEnabled()) {
      lowAlarmLabel->setText("no");
   }
   alarmLowEdit->setText(QString::number(m_ds1922->GetAlarmLow()));
   
      if(m_ds1922->GetAlarmHighEnabled()) {
      highAlarmLabel->setText("yes");
   }
   
   if(!m_ds1922->GetAlarmHighEnabled()) {
      highAlarmLabel->setText("no");
   }
   alarmHighEdit->setText(QString::number(m_ds1922->GetAlarmHigh()));
   
   loggingCheckBox->setChecked(m_ds1922->GetLoggingEnabled());
   
   highResLoggingCheckBox->setChecked(m_ds1922->GetHighResLogging());
   
   startUponAlarmCheckBox->setChecked(m_ds1922->GetStartUponAlarm());
   
   rolloverCheckBox->setChecked(m_ds1922->GetRollover());
   
   if(m_ds1922->GetMissionInProgress()) {
      missionInProgressLabel->setText("yes");
   }
   
   if(!m_ds1922->GetMissionInProgress()) {
      missionInProgressLabel->setText("no");
   }
   
   if(m_ds1922->GetWaitingForAlarm()) {
      waitingForAlarmLabel->setText("yes");
   }
   
   if(!m_ds1922->GetWaitingForAlarm() || !m_ds1922->GetMissionInProgress()) {
      waitingForAlarmLabel->setText("no");
   }
   
   missionStartDelaySpinBox->setValue(m_ds1922->GetMissionStartDelay());
   
   QString timeStamp;
   tm timeStampValue;
   m_ds1922->GetMissionTimestamp(&timeStampValue);
   timeStamp.sprintf("%02d.%02d.%02d %02d:%02d:%02d", timeStampValue.tm_mday, 
                     timeStampValue.tm_mon+1, timeStampValue.tm_year-100, timeStampValue.tm_hour, 
                     timeStampValue.tm_min, timeStampValue.tm_sec);
   missionTimestampEdit->setText(timeStamp);
   
   missionSampleCounterEdit->setText(QString::number(m_ds1922->GetSampleCount()));
   
   deviceSampleCounterEdit->setText(QString::number(m_ds1922->GetDeviceSampleCount()));
   
//  actionReadData->setEnabled(true);
}

void MainWindow::onReadData()
{  
   
   
   onReadConfig();
   int sampleCount=m_ds1922->GetSampleCount();
   int sampleRate=m_ds1922->GetSampleRate();
   
   tm timeStampValue;
   m_ds1922->GetMissionTimestamp(&timeStampValue);
   
   double buffer[sampleCount];
   if (!m_ds1922->ReadData(buffer, sampleCount)) {
     QMessageBox::critical(this, "Error", tr("Error reading data:\n")+m_ds1922->GetLastError().c_str());
     return;
   }
   
   dataTable->setRowCount(sampleCount);
   QDateTime dateTime(QDate(timeStampValue.tm_year+1900, timeStampValue.tm_mon+1, timeStampValue.tm_mday),
                      QTime(timeStampValue.tm_hour, timeStampValue.tm_min, timeStampValue.tm_sec));
   
   if (!m_ds1922->GetHighspeedSampling()) {
     sampleRate=sampleRate*60;
   }
   
   for(int i=0; i<sampleCount; i++){
      QTableWidgetItem *temperature = new QTableWidgetItem(QString::number(buffer[i]));
      QTableWidgetItem *time = new QTableWidgetItem(dateTime.addSecs(i*sampleRate).toString(Qt::SystemLocaleShortDate));
      dataTable->setItem(i, 1, temperature);
      dataTable->setItem(i, 0, time);     
   }
   
   
}

void MainWindow::onWriteConfig()
{
   
}

void MainWindow::onStopMission()
{
   m_ds1922->StopMission();
   onReadConfig();
}

void MainWindow::onClearData()
{
   m_ds1922->ClearMemory();
   onReadData();
}

void MainWindow::onStartMission()
{
   m_ds1922->ClearMemory();
   m_ds1922->StartMission();
   onReadConfig();
}