
//autor Lagunez Caramon Nestor
#include <iostream>
#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>

using namespace boost::asio;
using ip::tcp;
using namespace std;

const int CHUNK_SIZE = 65536; // Tamaño del fragmento (64 KB)

int main() {
    try {
        io_context io_context;
        tcp::socket socket(io_context);
        socket.connect(tcp::endpoint(ip::address::from_string("192.168.1.67"), 12345));

        cout << "Conexión establecida. Iniciando video chat..." << endl;

        cv::VideoCapture cap(0);
        if (!cap.isOpened()) {
            cerr << "No se pudo abrir la cámara." << endl;
            return 1;
        }
        /*El uso de static_cast<size_t> es simplemente una forma de garantizar que los valores devueltos por 
        cv::VideoCapture::get() se conviertan correctamente al tipo size_t, ya que cv::VideoCapture::get() devuelve valores de tipo double.*/
        size_t rows = static_cast<size_t>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        size_t cols = static_cast<size_t>(cap.get(cv::CAP_PROP_FRAME_WIDTH));

        socket.write_some(buffer(&rows, sizeof(rows)));
        socket.write_some(buffer(&cols, sizeof(cols)));

        cv::Mat frame;
        while (true) {
            cap.read(frame);
            if (frame.empty()) {
                cerr << "No se pudo capturar el cuadro." << endl;
                break;
            }

            size_t totalBytesSent = 0;

            /*La condición totalBytesSent < frame.total() * frame.elemSize() se utiliza para asegurarse de que 
            estamos enviando todos los bytes de la imagen a través del socket.
            La función frame.total() devuelve el número total de elementos (píxeles) en la matriz frame. 
            Mientras que frame.elemSize() devuelve el tamaño en bytes de cada elemento de la matriz. Multiplicando 
            estos dos valores, obtenemos el tamaño total en bytes de la matriz frame.

            La variable totalBytesSent lleva un registro de la cantidad de bytes que ya hemos enviado 
            a través del socket. La condición totalBytesSent < frame.total() * frame.elemSize() asegura que 
            todavía hay bytes restantes por enviar, y el bucle continuará enviando fragmentos adicionales hasta que 
            se hayan enviado todos los bytes de la imagen.*/
            while (totalBytesSent < frame.total() * frame.elemSize()) {
                /*size_t remainingBytes = frame.total() * frame.elemSize() - totalBytesSent; 
                se utiliza para calcular la cantidad de bytes restantes que deben enviarse 
                a través del socket en un fragmento particular.*/
                size_t remainingBytes = frame.total() * frame.elemSize() - totalBytesSent;

                /*La función min(a, b) es una función de la biblioteca estándar de C++ que devuelve el valor más pequeño 
                entre a y b. Por lo tanto, min(remainingBytes, static_cast<size_t>(CHUNK_SIZE)) calculará el valor más pequeño 
                entre remainingBytes (los bytes restantes en la matriz frame) y CHUNK_SIZE (el tamaño máximo del fragmento).*/
                size_t bytesToSend = min(remainingBytes, static_cast<size_t>(CHUNK_SIZE));
                /*buffer(frame.data + totalBytesSent, bytesToSend): Esta función de Boost.Asio crea un búfer que representa el 
                fragmento de datos que deseamos enviar a través del socket. El búfer se construye a partir del puntero frame.
                data avanzado por totalBytesSent y tiene un tamaño de bytesToSend.
                
                totalBytesSent += ...: Actualizamos la variable totalBytesSent agregando la cantidad de bytes enviados en esta 
                iteración del bucle. Esto nos permite llevar un seguimiento de cuántos bytes se han enviado en total hasta el momento.*/
                totalBytesSent += socket.write_some(buffer(frame.data + totalBytesSent, bytesToSend));
            }

            if (cv::waitKey(1) == 27) // Presionar la tecla Esc para salir
                break;
        }

         rows = 0;
         cols = 0;
        socket.write_some(buffer(&rows, sizeof(rows)));
        socket.write_some(buffer(&cols, sizeof(cols)));
        cout << "Conexión terminada." << endl;

    }
    catch (exception& e) {
        cerr << "Excepción: " << e.what() << endl;
    }

    return 0;
}
