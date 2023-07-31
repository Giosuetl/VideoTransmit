//autor Lagunez Caramon Nestor
#pragma once
#include "opencv2\core.hpp"
#include "opencv2\imgcodecs.hpp"
#include "opencv2\imgproc.hpp"
#include "opencv2\highgui.hpp"
#include "opencv2\aruco.hpp"
#include "opencv2\calib3d.hpp"
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;
using namespace cv;
const float Tamaño_Cuadrado_Calibracion = 0.0415f; //metros
const float Tamaño_Aruco_cuadrado = .04f; //metros
const Size Dimensiones_Chess = Size(8, 5);
const int im = 13, imt = 27;

void GenerarMarcadores_Aruco() {
	Mat marcadorD_salida;// se crea un objeto del tipo matt para almacenar los marcadores aruco
	Ptr<aruco::Dictionary> Diccionario_marcadores =
		aruco::getPredefinedDictionary(aruco::PREDEFINED_DICTIONARY_NAME::DICT_4X4_50);//se crea un apuntador al objeto aruco::Dictionary el
		//cual es un diccionario de marcadores aruco, el apuntador se movera entre los marcadores contenidos en el objeto para poder extraerlos uno por uno

	for (int i = 0; i < 50; i++) {
		aruco::drawMarker(Diccionario_marcadores, i, 500, marcadorD_salida, 1);
		ostringstream StringNombre;
		string Nombre_img = "marcador4x4_";
		StringNombre << Nombre_img << i << ".jpg";//Se genera una cadena con los parametros correspondientas a el nombre numero y tipo de imagen
		cv::imwrite(StringNombre.str(), marcadorD_salida);//se envian los parametro de la imagen y el marcador a imwrite para generar el arcivo
	}
}

void Crear_Posiciones_Tablero(Size Tamaño_tablero, float Tamaño_Cuadrado_Calibracion, vector<Point3f>& esquinas)
{
	for (int i = 0; i < Tamaño_tablero.height; i++)
	{
		for (int j = 0; j < Tamaño_tablero.width; j++)
		{
			esquinas.push_back(Point3f(j * Tamaño_Cuadrado_Calibracion, i * Tamaño_Cuadrado_Calibracion, 0.0f));//(x,y,z=0)//se trabaja con & para actualizar directamente el valor de esquinas
		}//se obtienen las coorrdenadas de los puntos correspondientes a las esquinas de un tablero ideal, con valors xy y z=0
	}
}

void Esquinas_Tablero(vector<Mat> Imagenes_guardadas, vector<vector<Point2f>>& AllEsquina_detectadas)
{

	for (vector<Mat>::iterator iter = Imagenes_guardadas.begin(); iter != Imagenes_guardadas.end(); iter++)
	{//se recorrre el vector Imagenes_guardadas contenedor de imagenes
		vector<Point2f> contenedor;//se crea para almacenar los puntos encontrados en la imagen temporalmente
		bool Encontrado = findChessboardCorners(*iter, Dimensiones_Chess, contenedor, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE);
		//se envia el iterator, las dimensiones del tablero, el buffer para los puntos y el algortimo a utilizar adaptative thresh
		//para y normalizar la imagen
		if (Encontrado) {
			AllEsquina_detectadas.push_back(contenedor);// la informacion del contenedor se almacena en AllEsquina_detectadas
		}
	}
}

void Calibracion_Camara(vector<Mat> Imagenes_guardadas, Size Tamaño_tablero, float Tamaño_Cuadrado_Calibracion, Mat& matriz_camara, Mat& coeficientesD_distancia)
{
	vector<vector<Point2f>> AllEsquina_detectadas;
	Esquinas_Tablero(Imagenes_guardadas, AllEsquina_detectadas);

	vector<vector<Point3f>> PuntosD_patron_Calibracion(1);
	Crear_Posiciones_Tablero(Tamaño_tablero, Tamaño_Cuadrado_Calibracion, PuntosD_patron_Calibracion[0]);
	PuntosD_patron_Calibracion.resize(AllEsquina_detectadas.size(), PuntosD_patron_Calibracion[0]);

	vector<Mat> VectoresR, VectorsT;
	coeficientesD_distancia = Mat::zeros(8, 1, CV_64F);
	cv::calibrateCamera(PuntosD_patron_Calibracion, AllEsquina_detectadas, Tamaño_tablero, matriz_camara, coeficientesD_distancia, VectoresR, VectorsT);
}
bool Guardar_Calibracion(string nombre, Mat matriz_camara, Mat coeficientesD_distancia)
{
	ofstream outStream(nombre);//recibe el nombre del archivo y por medio de una variable del tipo ofstream abre el fichero con los datos de calibracion
	if (outStream)//abre siempre que se el fichero sea abierto
	{
		uint16_t filas = matriz_camara.rows;
		uint16_t columnas = matriz_camara.cols;
		outStream << filas << endl;//almacena el nu mro de filas en outStream: 3
		outStream << columnas << endl;//almacena el nu mro de columnas en outStream: 3

		for (int r = 0; r < filas; r++)
		{
			for (int c = 0; c < columnas; c++)
			{
				double value = matriz_camara.at<double>(r, c);
				outStream << value << endl;//almacena los datos de matriz_camara en outStream
			}
		}
		filas = coeficientesD_distancia.rows;//5 mariz de coeficientes 5x1
		columnas = coeficientesD_distancia.cols;//1
		outStream << filas << endl;
		outStream << columnas << endl;

		for (int r = 0; r < filas; r++)
		{
			for (int c = 0; c < columnas; c++)
			{
				double value = coeficientesD_distancia.at<double>(r, c);
				outStream << value << endl;
			}
		}
		outStream.close();
		return true;
	}
	return false;
}

bool Cargar_Calibracion(string nombre, Mat& matriz_camara, Mat& coeficientesD_distancia)
{
	ifstream inStream(nombre);
	if (inStream)
	{
		uint16_t filas;
		uint16_t columnas;
		inStream >> filas;
		inStream >> columnas;
		matriz_camara = Mat(Size(columnas, filas), CV_64F);///(r,c)

		for (int r = 0; r < filas; r++)
		{
			for (int c = 0; c < columnas; c++)
			{
				double read = 0.0f;
				inStream >> read;
				matriz_camara.at<double>(r, c) = read;
				cout << matriz_camara.at<double>(r, c) << "\n";
			}
		}
		inStream >> filas;
		inStream >> columnas;
		coeficientesD_distancia = Mat::zeros(filas, columnas, CV_64F);
		for (int r = 0; r < filas; r++)
		{
			for (int c = 0; c < columnas; c++)
			{
				double read = 0.0f;
				inStream >> read;
				coeficientesD_distancia.at<double>(r, c) = read;
				cout << coeficientesD_distancia.at<double>(r, c) << "\n";
			}
		}
		inStream.close();
		return true;
	}
	return false;
}

void Calibrador(Mat& matriz_camara, Mat& coeficientesD_distancia)
{
	Mat cuadro_camara, cuadro_modificado;
	vector<Mat> imag_guardadas;
	vector<vector<Point2f>> Esquinas_marcdor, Candidatos_Naptos;//puntos con esquinas y candidados rechazados
	VideoCapture Vcapturador(0);//se inicia la camara y empieza a capturar

	if (!Vcapturador.isOpened())
	{
		return;
	}
	int cuadros_porSegundo = 20;
	namedWindow("webcam", CV_WINDOW_AUTOSIZE);//ventana 
	while (true)
	{
		if (!Vcapturador.read(cuadro_camara))
			break;
		vector<Vec2f> contenedor;
		bool detectado = false;

		detectado = findChessboardCorners(cuadro_camara, Dimensiones_Chess, contenedor, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE);//buscas un patron

		cuadro_camara.copyTo(cuadro_modificado);//copias la imagen que obtienes de la camara a otra variable en la que puedas hacer modificaciones libremente

		drawChessboardCorners(cuadro_modificado, Dimensiones_Chess, contenedor, detectado);

		if (detectado)
			imshow("webcam", cuadro_modificado);
		else
			imshow("webcam", cuadro_camara);

		char entrada = waitKey(1000 / cuadros_porSegundo);

		switch (entrada)
		{
		case ' '://espacio para tomar foto
			cout << "inicio imagen guardada";
			if (detectado)
			{
				Mat temp;
				cuadro_camara.copyTo(temp);
				imag_guardadas.push_back(temp);
				cout << "\nimagen: " << imag_guardadas.size();
			}
			break;
		case 'a':// enters
			if (imag_guardadas.size() > 15)
			{
				cout << "inicio_calibracion";
				Calibracion_Camara(imag_guardadas, Dimensiones_Chess, Tamaño_Cuadrado_Calibracion, matriz_camara, coeficientesD_distancia);
				Guardar_Calibracion("calibrados", matriz_camara, coeficientesD_distancia);
			}
			break;
		case 'b':
			cout << "inicio_break";
			return;
			break;
		}
	}
}
void inicioo(void)

{
	cout << "inicioa";
	Mat matriz_camara = Mat::eye(3, 3, CV_64F);//matriz identidad 3x3 del tipo CV_64F
	Mat coeficientesD_distancia;
	Calibrador(matriz_camara, coeficientesD_distancia);
	//Cargar_Calibracion("calibrado", matriz_camara, coeficientesD_distancia);
	//inicio_Monitoreo(matriz_camara, coeficientesD_distancia, 0.099f);
}


