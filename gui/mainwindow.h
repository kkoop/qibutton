#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qtimer.h>
#include "ui_main.h"



class DS1922;
class DS9490;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
   Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();

public slots:
   virtual void onReadConfig();
   virtual void onReadData();
   virtual void onWriteConfig();
   virtual void onStopMission();
   virtual void onClearData();
   virtual void onStartMission();
   virtual void onCopy();
   virtual void onSafe();
   virtual void onPlot();
   virtual void onAbout();
   
private:
QString GetDataAsCsv();
   
   
   // data
protected:
   DS1922* m_ds1922;
   DS9490* m_ds9490;
   //QTimer m_clockSet;
};

#endif
