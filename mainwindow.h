#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>

#include <armadillo>

// - [] play/pause on button

//----------------------------
//#define FS          8000 // dla procesora M33 z ostatnich zajęć
//#define DSIZE       8192
//----------------------------

#define FS          2048

#define RSIZE       1024        // Read
#define RSIZE2      (RSIZE/2)


#define DSIZE       (RSIZE*5)   // Data
#define DSIZE2      (DSIZE/2)

#define FFT_SIZE    2048
#define FFT_SIZE2   (FFT_SIZE/2)

#define MSIZE       (RSIZE/32)   // Mask

#define FILENAME "dataFile"

struct DTOtoEmbbeded { // GET // data transfer object from embeded
    char isChecked(); // action button
    int32_t Set=0, motorPower=0;    
};

struct DTOfromEmbbeded { // POST // data transfer object from embeded
    int32_t Set=0, motorPower=0;
    float Speed=0;
};


// --------------------
//struct smallConfig {

//};

//    int dataTransfer[RSIZE2-sizeof(smallConfig)]; // by wykozystac całą ramkę

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void sendCommand();
    void readData();
    void error(QSerialPort::SerialPortError error);
    void closeEvent(QCloseEvent *event);
    void openComPort();
    void loadFile();
    void saveFile();
    void systemOperation();
    void inPan();

private:
    Ui::MainWindow *ui;
     void calculateFFT();

    QSerialPort serial;
    QByteArray senddata;
    QByteArray readdata;

    QVector<double> magnitudeData;
    QVector<double> phaseData;
    QVector<double> fftWin;
    QVector<double> maskFilter;

    arma::cx_vec fftData;
    bool displayFinitCycle;

};
#endif // MAINWINDOW_H
