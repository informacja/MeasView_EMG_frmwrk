#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    centralWidget()->layout()->setContentsMargins(0,0,0,0);
    centralWidget()->layout()->setSpacing(0);

    connect(ui->actionRun, &QAction::triggered, this, &MainWindow::sendCommand);
    connect(ui->actionFile, &QAction::triggered, this, &MainWindow::systemOperation);
    connect(&serial, &QSerialPort::readyRead, this, &MainWindow::readData); // jak dodać play/pause pod spacja?
    connect(&serial, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(error(QSerialPort::SerialPortError)));

    connect(ui->actionFilter, &QAction::triggered, this, &MainWindow::inPan);

    ui->plot->setSeries(1, DSIZE2);
    ui->plot->plotMode=Plot::LinearPlot;
    ui->plot->plotColor[0]=Qt::red;
    ui->plot->setRange(0, DSIZE2-1, -1, 1);
    ui->plot->setAxes(10, 0,  1000*DSIZE2/FS, 10, -1, 1);

    ui->plot_2->setSeries(2, DSIZE2);
    ui->plot_2->plotMode = Plot::LinearPlot;
    ui->plot_2->plotColor[0] = Qt::green;
    ui->plot_2->plotColor[1] = Qt::yellow;
    ui->plot_2->setRange(0, DSIZE2-1, 0, 1);
    ui->plot_2->setAxes(10, 0,  1000*DSIZE2/FS, 10, 0, 1);
    displayFinitCycle = false;

    maskFilter.resize(MSIZE);
    maskFilter.fill(1.0/MSIZE);

    openComPort();
}

MainWindow::~MainWindow()
{
    serial.close();
    delete ui;
}

void MainWindow::sendCommand()
{
    DTOtoEmbbeded structTemp;

    //Fill structTemp

    char buffTemp[sizeof(structTemp)];

    memcpy(buffTemp, &structTemp, sizeof(structTemp));

    senddata = QByteArray::fromRawData(buffTemp, sizeof(structTemp));

    //    if( serial.isOpen() )
    {
        senddata.clear();
        int sizeDTO = sizeof(DTOtoEmbbeded);
        senddata.resize(sizeDTO);
        senddata[0]=static_cast<uint8_t>(ui->actionRun->isChecked() << 7); // Run/Stop
        serial.write(senddata);
    }
    //    else
    {
        //        openPort();
    }
}



void MainWindow::readData()
{

//    readdata=serial.readAll();
//    qDebug()<<readdata.size();
//    for(auto &i : readdata) {
//        qDebug() << (int)i;
//    }
//    return;

    static uint32_t fr=0;
    double avg = 0;

    if (serial.size() >= RSIZE) {

        readdata=serial.read(RSIZE);
        qDebug()<<readdata.size();

        uint16_t *sample=reinterpret_cast<uint16_t*>(readdata.data());

        for(auto &i : readdata) {
            qDebug() << i;
        }

        // remove DC offset
        double dc=0;
        for (int n=0; n<RSIZE2; n++) {
            dc+=(sample[n]-32768)/32768.0;
        }

        dc/=RSIZE2;


        for (int n=0; n<RSIZE2; n++) {

            //            if (displayFinitCycle)  {
            //                newBegin = (1 + newBegin) % DSIZE2;
            ui->plot->dataSeries[0][fr+n] = ((sample[n]-32768)/32768.0)-dc;

//            ui->plot->dataSeries[0].push_back(((sample[n]-32768)/32768.0)-dc);
//            ui->plot->dataSeries[0].pop_front();

            //                 ui->plot->dataSeries[0].startsWith(*(ui->plot_2->dataSeries[0].data()+1));
            //                 ui->plot->dataSeries[0].replace(fr+n,  ((sample[n]-32768)/32768.0)-dc);
            //                 ui->plot->dataSeries[0].append( ((sample[n]-32768)/32768.0)-dc );

            //            }else
            // Raw data - DC offset
            //            ui->plot->dataSeries[0][fr+n] = ((sample[n]-32768)/32768.0)-dc;


            // example - other plots
            ui->plot_2->dataSeries[0][fr+n] = abs(ui->plot->dataSeries[0][fr+n]);
            //            ui->plot_2->dataSeries[1][fr+n]=0.618*ui->plot_2->dataSeries[0][fr+n];
            //            ui->plot_2->dataSeries[0][fr+n]=abs(ui->plot->dataSeries[0][DSIZE2-1]);
            //            ui->plot_2->dataSeries[1].push_back( 0.618*ui->plot_2->dataSeries[0][DSIZE2-1] );
            //            ui->plot_2->dataSeries[1].pop_front();
            // trochę mało wydajne dlatego zakomentowane powyższe

            //            ui->plot-> ui->plot_2->dataSeries[0].count() );

            // if the data have values, we establish the last value to max Limit in X axis
            //            if( ui->plot_2->dataSeries[0].count() > 1 ){
            //                 ui->plot->setMaximumWidth( ui->plot_2->dataSeries[0].at( ui->plot_2->dataSeries[0].count()-1));
            //            }

            int stepL = ( fr+n < MSIZE ) ? fr+n : MSIZE;
            for(int i = 1; i < stepL; i++)
            {
                avg += ui->plot_2->dataSeries[0][fr+n+i - stepL-1]*maskFilter[i];
            }
            ui->plot_2->dataSeries[1][fr+n]=avg;
            avg = 0;
        }

        fr += RSIZE2;

        if(fr>=DSIZE2)
        {
            fr=0;
            displayFinitCycle = true;
        }

        ui->plot->update();
        ui->plot_2->update();
    }
}

void MainWindow::error(QSerialPort::SerialPortError error)
{
    qDebug()<<error;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    if(ui->actionRun->isChecked())
        ui->actionRun->trigger();
}

void MainWindow::calculateFFT()
{
    fftData.fill(0);
    for(int i=0;i<DSIZE2; i++)
        fftData[static_cast<uint>(i)].real(ui->plot->dataSeries[0][i]*fftWin[i]);

    fftData=arma::fft(fftData);

    for(int i=0;i<FFT_SIZE2; i++) {
        magnitudeData[i]=abs(fftData[static_cast<uint>(i)])/(FFT_SIZE2);
        phaseData[i]=arg(fftData[static_cast<uint>(i)]);
    }
}

// ----------------------------------------------------------------------------

void MainWindow::openComPort()
{
    ui->statusbar->showMessage("No device");
    QString portname;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
    qDebug() << "Available communication port description: " << info.description() << info.portName();
#ifdef Q_OS_UNIX
        if (info.description()=="VCOM")
            // on linux check port permissions error, try run studio with higher privileges
#else

        // Attention: if still doesn't work try different USB port, checked works :|
        if (info.description() == "Urządzenie szeregowe USB")
#endif
        {
            portname=info.portName();
            serial.setPortName(portname);
            if (serial.open(QIODevice::ReadWrite)) {
                ui->statusbar->showMessage(tr("Device: %1").arg(info.description()));
                serial.clear();
                ui->statusbar->setEnabled(true);
            } else {
                ui->statusbar->showMessage(tr("Can't open %1, error code %2") .arg(serial.portName()).arg(serial.error()));
                return;
            }
            break;
        }
    }
    ui->actionRun->setChecked( serial.isOpen() );
    if (!serial.isOpen()) {
//            ui->plot
//             ui->plot->drawText(painter, gw/3+(font.pointSize()), 90-(font.pointSize()), "Nie nawiązano połączenia, spróbuj podłączyć płytkę i uruchomić program ponownie");
         }
}

void MainWindow::systemOperation()
{
    qDebug() << "fileSystemOperation()";
    if ( !ui->actionRun->isChecked() )
        loadFile();
    else
        saveFile();
}

void MainWindow::loadFile()
{
    QString dataSeries;
    QFile file(FILENAME);
    qDebug()<<"startLoading" << FILENAME;

    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
         qDebug()<<file.size();
        while( !in.atEnd() ){
            dataSeries = in.readLine();
        }
        qDebug()<<dataSeries.size();
        QStringList list = dataSeries.split(QLatin1Char(','));
        qDebug()<<"cou" <<list.count();

        qDebug()<<list.size();
        for(int i=0; i < ui->plot->dataSeries[0].size(); i++)
        {
            ui->plot->dataSeries[0][i] = list.at(i).toDouble();
            ui->plot_2->dataSeries[0][i] = abs(list.at(i).toDouble());
        }
        file.close();
    }

    qDebug()<<"endReadingFile";

    ui->plot->update();
    ui->plot_2->update();
}

void MainWindow::saveFile()
{
    QFile file(FILENAME);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if ( file.isWritable() ) qDebug()<<"Saving to"<< FILENAME << endl << "begPos:" << file.size();
        QTextStream out(&file);
        for(int n=0; n < ui->plot->dataSeries[0].size(); n++)
        {
            out << ui->plot->dataSeries[0][n]<<",";
        }
        out << "\n";
        file.close();
        qDebug()<< "endPos:" << file.size();
    }
}

void MainWindow::inPan() {

//    if(/*ui->plot->dataSeries[0].begin()*/

//        double min = *std::min_element(ui->plot->dataSeries[0].begin(), ui->plot->dataSeries[0].end());
//        double max = *std::max_element(ui->plot->dataSeries[0].begin(), ui->plot->dataSeries[0].end());



}
