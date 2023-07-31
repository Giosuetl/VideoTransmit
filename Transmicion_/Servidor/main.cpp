//autor Lagunez Caramon Nestor
#include <iostream>
#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>
#include "opencv2/aruco.hpp"
#include "opencv2/calib3d.hpp"
#include "vision.h"

#include <vector>
using namespace boost::asio;
using ip::tcp;
using namespace std;

const int CHUNK_SIZE = 65536; // Tamaño del fragmento (64 KB)
const int DETECTION_INTERVAL = 10; // Realizar detección cada 5 cuadros

struct dat {
    uint32_t clientID = 0;
    double x, y, z;
};

int main() {
    try {
        io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));

        cout << "Esperando conexión..." << endl;
        tcp::socket socket(io_context);
        acceptor.accept(socket);

        cout << "Conexión establecida. Recibiendo imágenes..." << endl;

        cv::namedWindow("Server", cv::WINDOW_AUTOSIZE);

        size_t rows, cols;
        socket.read_some(buffer(&rows, sizeof(rows)));
        socket.read_some(buffer(&cols, sizeof(cols)));

        cv::Mat frame(rows, cols, CV_8UC3);

        dat nu = { 0 };
        cv::Mat DCoeficientes;
        cv::Mat matriz_calibrada = cv::Mat::eye(3, 3, CV_64F);
        Cargar_Calibracion("calibrado", matriz_calibrada, DCoeficientes);
        vector<int> aruco_IDs;
        vector<vector<cv::Point2f>> marcador_Esquinas, rejectedCandidates;
        cv::Ptr<cv::aruco::Dictionary> DiccionarioMarcadores = cv::aruco::getPredefinedDictionary(cv::aruco::PREDEFINED_DICTIONARY_NAME::DICT_4X4_50);
        vector<cv::Vec3d> vectores_rotacion, vectores_translacion;

        int frameCount = 0;
        while (true) {
            size_t totalBytesReceived = 0;

            while (totalBytesReceived < frame.total() * frame.elemSize()) {
                size_t remainingBytes = frame.total() * frame.elemSize() - totalBytesReceived;
                size_t bytesToReceive = min(remainingBytes, static_cast<size_t>(CHUNK_SIZE));

                totalBytesReceived += socket.read_some(buffer(frame.data + totalBytesReceived, bytesToReceive));
            }

            if (frameCount % DETECTION_INTERVAL == 0) {
                aruco::detectMarkers(frame, DiccionarioMarcadores, marcador_Esquinas, aruco_IDs);
                aruco::estimatePoseSingleMarkers(marcador_Esquinas, Tamaño_Aruco_cuadrado, matriz_calibrada, DCoeficientes, vectores_rotacion, vectores_translacion);
               
                if (!vectores_translacion.empty()) {
                    const cv::Vec3d& c = vectores_translacion[0];
                    nu.x = c[0];
                    nu.y = c[1];
                    nu.z = c[2];
                }
                std::cout << "x: " << nu.x << " y: " << nu.y << " z: " << nu.z << "\n";
            }
            aruco::drawDetectedMarkers(frame, marcador_Esquinas, aruco_IDs);

            frameCount++;

            if (!frame.empty()) {
                cv::imshow("Server", frame);
                cv::waitKey(1);
            }
            else {
                cout << "Conexión terminada por el cliente." << endl;
                break;
            }
        }
    }
    catch (exception& e) {
        cerr << "Excepción: " << e.what() << endl;
    }

    return 0;
}
