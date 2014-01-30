#include "mainwindow.h"
#include "../ds1922.h"
#include "../ds9490.h"
#include <QMessageBox>
#include <QClipboard>
#include <QFileDialog>
#include <QTextStream>
#include <QProcess>
#include <QTemporaryFile>
#include "ui_about.h"
#include <QLocale>
#include <string>
#include <iostream>
#include <QString>

MainWindow::MainWindow(QWidget *parent)
 : QMainWindow(parent)
{
   setupUi(this);
   m_ds9490 = new DS9490;
   m_ds1922 = new DS1922(m_ds9490);
   //connect(&m_clockSet, SIGNAL(timeout()), this, SLOT(rtcEdit->stepUp()));
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
   
   //m_clockSet.start(1000);
   
   sampleRateSpinBox->setValue(m_ds1922->GetSampleRate());
   sampleRateSecondsRadioButton->setChecked(m_ds1922->GetHighspeedSampling());
   sampleRateMinutesRadioButton->setChecked(!m_ds1922->GetHighspeedSampling());
   
   if(m_ds1922->GetAlarmLow()) {
      lowAlarmLabel->setText("yes");
   } else {
      lowAlarmLabel->setText("no");
   }
   alarmLowEdit->setText(QString::number(m_ds1922->GetAlarmLowThreshold()));
   
   if(m_ds1922->GetAlarmHigh()) {
      highAlarmLabel->setText("yes");
   } else {

      highAlarmLabel->setText("no");
   }
   alarmHighEdit->setText(QString::number(m_ds1922->GetAlarmHighThreshold()));
   
   loggingCheckBox->setChecked(m_ds1922->GetLoggingEnabled());
   
   highResLoggingCheckBox->setChecked(m_ds1922->GetHighResLogging());
   
   startUponAlarmCheckBox->setChecked(m_ds1922->GetStartUponAlarm());
   
   rolloverCheckBox->setChecked(m_ds1922->GetRollover());
   
   if(m_ds1922->GetMissionInProgress()) {
      missionInProgressLabel->setText("yes");
      actionStartMission->setEnabled(0);
      actionStopMission->setEnabled(1);
   } else {
      missionInProgressLabel->setText("no");
      actionStopMission->setEnabled(0);
      actionStartMission->setEnabled(1);
   }
   
   if(m_ds1922->GetWaitingForAlarm() && m_ds1922->GetMissionInProgress()) {
      waitingForAlarmLabel->setText("yes");
   } else {
      waitingForAlarmLabel->setText("no");
   }
   
   missionStartDelaySpinBox->setValue(m_ds1922->GetMissionStartDelay());
   
   tm timeStampValue;
   m_ds1922->GetMissionTimestamp(&timeStampValue);
   
   QDate date(QDate(timeStampValue.tm_year+1900, timeStampValue.tm_mon+1, timeStampValue.tm_mday));
   QTime time(QTime(timeStampValue.tm_hour, timeStampValue.tm_min, timeStampValue.tm_sec));
   
   missionTimestampEdit->setText(date.toString(Qt::SystemLocaleShortDate)+" "+time.toString(Qt::TextDate));
   
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
      
      QDateTime addedTime=dateTime.addSecs(i*sampleRate);
      
      QDate date=addedTime.date();
      QTime timeT=addedTime.time();
      QString dateLocaleTime=date.toString(Qt::SystemLocaleShortDate)+" "+timeT.toString(Qt::TextDate);
      
      QTableWidgetItem *time = new QTableWidgetItem(dateLocaleTime);
      dataTable->setItem(i, 1, temperature);
      dataTable->setItem(i, 0, time);     
   }   
}

void MainWindow::onWriteConfig()
{
   int sampleRate=sampleRateSpinBox->value();
   bool seconds=sampleRateSecondsRadioButton->isChecked();
   
   m_ds1922->SetRtcHighspeed(seconds);
   m_ds1922->SetSampleRate(sampleRate);
   
   bool low =alarmLowEnabledCheckBox->isChecked();
   bool high=alarmHighEnabledCheckBox->isChecked();
      
   m_ds1922->SetAlarmEnabled(low, high);
   
   double lowAlarm=alarmLowEdit->text().toDouble();
   double highAlarm=alarmHighEdit->text().toDouble();
   m_ds1922->SetAlarmLowThreshold(lowAlarm);
   m_ds1922->SetAlarmHighThreshold(highAlarm);
   
   m_ds1922->SetHighResLogging(highResLoggingCheckBox->isChecked());
   m_ds1922->SetStartUponAlarm(startUponAlarmCheckBox->isChecked());

   int delay=missionStartDelaySpinBox->value();
   m_ds1922->SetMissionStartDelay(delay);
   
   m_ds1922->SetRollover(rolloverCheckBox->isChecked());
   
   m_ds1922->SetRtcEnabled(rtcEnabledCheckBox->isChecked());
   
   m_ds1922->SetLoggingEnabled(loggingCheckBox->isChecked());
   
   if (!m_ds1922->WriteRegister()) {
     QMessageBox::critical(this, "Error", tr("Error writing config:\n")+m_ds1922->GetLastError().c_str());
     return;
   }  
   
   onReadConfig();
}

void MainWindow::onStopMission()
{
   if (!m_ds1922->StopMission()) {
     QMessageBox::critical(this, "Error", tr("Error stopping mission:\n")+m_ds1922->GetLastError().c_str());
   } else {
      onReadConfig();
   }
}

void MainWindow::onClearData()
{
   if (!m_ds1922->ClearMemory()) {
     QMessageBox::critical(this, "Error", tr("Error clearing memory:\n")+m_ds1922->GetLastError().c_str());
   } else {
      onReadConfig();
   }
}

void MainWindow::onStartMission()
{
   if (!m_ds1922->ClearMemory() || !m_ds1922->StartMission()) {
     QMessageBox::critical(this, "Error", tr("Error starting mission:\n")+m_ds1922->GetLastError().c_str());
   } else {
      onReadConfig();
   }
}


QString MainWindow::GetDataAsCsv()
{
   QString data;
   for (int i=0; i<dataTable->rowCount(); i++) {
      data += dataTable->item(i, 0)->text();
      data += "\t";
      data += dataTable->item(i, 1)->text();
      data += "\n";
     }
   return data;
}

void MainWindow::onCopy()
{
   QClipboard* clipboard = QApplication::clipboard();
   clipboard->setText(GetDataAsCsv());
}

void MainWindow::onSafe()
{
   QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                            "",
                            "CSV (*.csv)");
   if (!fileName.isEmpty())
   {
      QFile file(fileName);
      if (file.open(QIODevice::WriteOnly))
      {
         QTextStream out(&file);
         out << GetDataAsCsv();
      }
      else
      {
         QMessageBox::critical(this, tr("Error"), tr("Cannot open file for writing"));
      }
   }
}

void MainWindow::onPlot()
{
   QTemporaryFile dataFile;
   QTemporaryFile scriptFile;
   QLocale locale = QLocale::system();
   QString date=locale.dateFormat(QLocale::ShortFormat);

   QString time="%H:%M:%S";
   
   date.replace("dd","%d");
   date.replace("MM","%m");
   date.replace("yy","%y");

   if (dataFile.open() && scriptFile.open()) 
   {
      dataFile.setAutoRemove(false);
      scriptFile.setAutoRemove(false);
      QTextStream out(&dataFile);
      QString data = GetDataAsCsv();
      data.replace(" ", "_");
      out << data;
      std::cout << data.toStdString() << std::endl;
      QTextStream scriptOut(&scriptFile);
      scriptOut   << "set xdata time\n"
                  << "set timefmt" << " " << "\"" << date << "_" << time <<"\"\n" 
                  << "set grid\n"
                  << "plot '" << dataFile.fileName() << "' u 1:2 title \"Temperature\" with linespoints lt 4 lw 2\n" 
                  << "pause -1\n";
      QProcess *gnuplot = new QProcess();
      QStringList arguments(scriptFile.fileName());
      arguments << "-";
      gnuplot->start("gnuplot", arguments);
      gnuplot->waitForStarted();
//    gnuplot->close();
   }
}

void MainWindow::onAbout()
{
   Ui_AboutDialog aboutDlg;
   QDialog* dlg = new QDialog(this);
   aboutDlg.setupUi(dlg);
   dlg->show();
}
