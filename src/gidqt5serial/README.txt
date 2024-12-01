GidQt5Serial
============

G. van der Kolf, March 2017

GidQt5Serial is a QMainWindow class that contains a QSerialPort object and
provides a dialog for a user to choose, configure and open a serial port. This
allows a serial port GUI to be easily incorporated into any Qt application with
direct access to the QSerialPort object.


Usage
=====

 - QSerialPort was introduced in Qt 5.1, so this won't work with Qt 4.

 - Copy the files to your project directory, add them to your .pro file and
   #include the gidqt5serial.h file where you want to use it.

 - Add the serialport dependency to your .pro file: QT += serialport

 - Create an object instance e.g. GidQt5Serial mySerial;

 - The QSerialPort member "s" is publicly accessible. Access this directly to
   work with the serial port.

 - Connect the appropriate signals from the serial port member (.s) to your
   slots, e.g. the readyRead() signal emits when data is received on the port:
   connect(&(mySerial.s), &QSerialPort::readyRead, ... );

 - The portOpened() and dialogCancelled() signals are emitted when the dialog
   is closed by the user opening a port or cancelling, respectively.

 - In your readyRead slot, use the read functions of the serial port member,
   e.g. QByteArray rxData = mySerial.s.readAll();

 - To write to the serial port: mySerial.s.write(myData);

 - Use mySerial.show() to display a dialog for the user to configure and open
   a serial port.

 - Some informational messages are sent via the print signal.

 - GidQt5Serial is based on QMainWindow. Remember to call the close() method
   when quitting the application to ensure the window doesn't prevent the app
   from quitting.
   e.g. if using a QMainWindow you can override the QMainWindow's
   closeEvent(QCloseEvent *event) and in that put mySerial.close().
   ( Remember to also call event->accept() in your closeEvent. )

 - Other MainWindow related properties can also be set, such as modality,
   window title, etc., e.g.:
   mySerial.setWindowTitle("My Qt5 Serial App");
   mySerial.setWindowModality(Qt::ApplicationModal);
   
   
