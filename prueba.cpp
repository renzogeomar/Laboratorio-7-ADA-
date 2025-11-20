#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <string>

using namespace std;

const double INF = 1e9;

// ==========================================
// ESTRUCTURAS DE DATOS
// ==========================================

struct Coordenada {
    double x, y;
};

struct Comunidad {
    int id;
    string nombre;
    double demandaTotal;     // Lo que pidieron originalmente
    double demandaPendiente; // Lo que falta por entregar
    double cargaEnEsteViaje; // Lo que el camión lleva AHORA
    int prioridad;
    Coordenada ubicacion;
    double distAlmacen;
    double ratio;            
};

// Distancia Euclidiana
double calcularDistancia(Coordenada a, Coordenada b) {
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

// ==========================================
// FUNCIONES AUXILIARES
// ==========================================

bool compararPorRatio(const Comunidad &a, const Comunidad &b) {
    return a.ratio > b.ratio;
}

// Verifica si queda trabajo por hacer
bool hayPendientes(const vector<Comunidad>& comunidades) {
    for(const auto& c : comunidades) {
        if(c.demandaPendiente > 0.001) return true; // Umbral pequeño por float
    }
    return false;
}

// ==========================================
// ETAPA 1: MOCHILA FRACCIONAL (POR VIAJE)
// ==========================================

void planificarViaje(
    vector<Comunidad>& todasComunidades, 
    Coordenada almacen, 
    double capacidadMax, 
    vector<Comunidad*>& seleccionadasParaViaje,
    int numeroViaje) {
    
    cout << "\n======================================================================\n";
    cout << "                    PLANIFICACION VIAJE #" << numeroViaje << "\n";
    cout << "======================================================================\n";

    // 1. Recalcular ratios (solo necesario una vez, pero lo mantenemos por si cambia la lógica)
    for (auto &com : todasComunidades) {
        com.cargaEnEsteViaje = 0; // Reset para este viaje
        com.distAlmacen = calcularDistancia(almacen, com.ubicacion);
        double distSegura = (com.distAlmacen == 0) ? 0.1 : com.distAlmacen;
        com.ratio = (double)com.prioridad / distSegura;
    }

    // 2. Ordenar (Las de mayor prioridad/cercanía van primero)
    sort(todasComunidades.begin(), todasComunidades.end(), compararPorRatio);

    // 3. Llenado del Camión
    double espacioDisponible = capacidadMax;
    
    cout << left;
    cout << setw(4) << "ID" << " | " 
         << setw(9) << "Prioridad" << " | " 
         << setw(10) << "Dist. Alm" << " | " 
         << setw(8) << "Ratio" << " | " 
         << setw(9) << "Pendiente" << " | "  // Cambiado de 'Demanda' a 'Pendiente'
         << "Estado (Accion)" << endl;
    cout << string(75, '-') << endl;

    for (auto &com : todasComunidades) {
        string estadoStr;
        
        if (com.demandaPendiente <= 0.001) {
            // Ya se entregó todo en viajes anteriores
            estadoStr = "COMPLETADO (Previamente)";
        }
        else if (espacioDisponible <= 0.001) {
            // Camión lleno, queda para el siguiente viaje
            estadoStr = "EN ESPERA (Capacidad llena)";
        } 
        else {
            // Intentamos cargar
            if (com.demandaPendiente <= espacioDisponible) {
                // Carga completa de lo que falta
                com.cargaEnEsteViaje = com.demandaPendiente;
                espacioDisponible -= com.demandaPendiente;
                com.demandaPendiente = 0; // Ya no deberá nada
                
                seleccionadasParaViaje.push_back(&com); // Guardamos puntero para modificar original
                estadoStr = "SELECCIONADA (Finaliza)";
            } 
            else {
                // Carga parcial (Mochila Fraccional)
                com.cargaEnEsteViaje = espacioDisponible;
                com.demandaPendiente -= espacioDisponible; // Queda debiendo para el prox viaje
                espacioDisponible = 0;
                
                seleccionadasParaViaje.push_back(&com);
                estadoStr = "PARCIAL (" + to_string((int)com.cargaEnEsteViaje) + " ton)";
            }
        }

        // Imprimir fila
        cout << setw(4) << com.id << " | " 
             << setw(9) << com.prioridad << " | " 
             << setw(10) << fixed << setprecision(2) << com.distAlmacen << " | " 
             << setw(8) << com.ratio << " | " 
             << setw(9) << (com.demandaPendiente + com.cargaEnEsteViaje) << " | " // Mostramos lo que había antes de cargar
             << estadoStr << endl;
    }
    
    cout << string(75, '-') << endl;
    cout << "Capacidad libre al salir: " << espacioDisponible << " toneladas." << endl;
}

// ==========================================
// ETAPA 2: FLOYD-WARSHALL
// ==========================================

vector<vector<double>> calcularMatrizFloyd(const vector<Comunidad*>& seleccionadas, Coordenada almacen) {
    int N = seleccionadas.size() + 1; 
    vector<Coordenada> nodos(N);
    
    nodos[0] = almacen;
    for(size_t i=0; i<seleccionadas.size(); i++) {
        nodos[i+1] = seleccionadas[i]->ubicacion;
    }

    vector<vector<double>> dist(N, vector<double>(N));
    for(int i=0; i<N; i++) {
        for(int j=0; j<N; j++) {
            if (i==j) dist[i][j] = 0;
            else dist[i][j] = calcularDistancia(nodos[i], nodos[j]);
        }
    }

    for (int k = 0; k < N; k++) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (dist[i][k] + dist[k][j] < dist[i][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                }
            }
        }
    }
    return dist;
}

// ==========================================
// ETAPA 3: EJECUCIÓN DE RUTA
// ==========================================

double ejecutarRuta(const vector<vector<double>>& matrizDist, const vector<Comunidad*>& seleccionadas) {
    cout << "\n   >>> EJECUCION DE RUTA (Vecino Mas Cercano) >>>\n";
    
    int N = matrizDist.size();
    vector<bool> visitado(N, false);
    int nodoActual = 0; 
    visitado[0] = true;
    double distanciaViaje = 0;

    cout << "   [Salida] Almacen Central" << endl;

    for (int paso = 0; paso < N - 1; paso++) {
        int siguienteNodo = -1;
        double minDistancia = INF;

        for (int i = 0; i < N; i++) {
            if (!visitado[i] && matrizDist[nodoActual][i] < minDistancia) {
                minDistancia = matrizDist[nodoActual][i];
                siguienteNodo = i;
            }
        }

        if (siguienteNodo != -1) {
            visitado[siguienteNodo] = true;
            distanciaViaje += minDistancia;
            Comunidad* c = seleccionadas[siguienteNodo - 1];
            
            cout << "   |--> Ir a ID " << c->id << " (" << c->nombre << ") "
                 << "[Dist: " << minDistancia << "] "
                 << "Entregar: " << c->cargaEnEsteViaje << " ton.";
            
            if (c->demandaPendiente > 0) cout << " (Queda pendiente: " << c->demandaPendiente << ")";
            else cout << " (Pedido Completado)";
            
            cout << endl;
            nodoActual = siguienteNodo;
        }
    }

    double distRegreso = matrizDist[nodoActual][0];
    distanciaViaje += distRegreso;
    cout << "   |--> Regreso a Almacen [Dist: " << distRegreso << "]" << endl;
    
    return distanciaViaje;
}

// ==========================================
// MAIN
// ==========================================

int main() {
    Coordenada almacen = {0, 0};
    
    // CAPACIDAD REDUCIDA para forzar múltiples viajes con los datos dados
    double capacidadVehiculo = 40.0; 
    
    vector<Comunidad> comunidades = {
        // Inicializamos demandaPendiente igual a demandaTotal
        {1, "Zona A", 15.0, 15.0, 0, 90, {10, 5}, 0, 0},
        {2, "Zona B", 20.0, 20.0, 0, 60, {2, 3}, 0, 0},
        {3, "Zona C", 10.0, 10.0, 0, 80, {50, 50}, 0, 0},
        {4, "Zona D", 25.0, 25.0, 0, 40, {5, 5}, 0, 0},
        {5, "Zona E", 10.0, 10.0, 0, 95, {12, 8}, 0, 0} 
    };

    int numeroViaje = 1;
    double distanciaTotalOperacion = 0;

    // BUCLE PRINCIPAL: MIENTRAS HAYA NECESIDAD
    while(hayPendientes(comunidades)) {
        
        // Vector de PUNTEROS para trabajar sobre las comunidades originales
        vector<Comunidad*> seleccionadasParaViaje;

        // 1. Planificar carga (Etapa Mochila)
        planificarViaje(comunidades, almacen, capacidadVehiculo, seleccionadasParaViaje, numeroViaje);

        if (!seleccionadasParaViaje.empty()) {
            // 2. Calcular distancias (Floyd)
            vector<vector<double>> matriz = calcularMatrizFloyd(seleccionadasParaViaje, almacen);
            
            // 3. Ejecutar ruta
            double distEsteViaje = ejecutarRuta(matriz, seleccionadasParaViaje);
            distanciaTotalOperacion += distEsteViaje;
            
            cout << "   [Fin Viaje " << numeroViaje << "] Distancia recorrida: " << distEsteViaje << "\n";
        } else {
            // Seguridad para evitar bucles infinitos si algo falla
            cout << "Error: Hay pendientes pero no se selecciono nada (revisar logica)." << endl;
            break;
        }

        numeroViaje++;
    }

    cout << "\n======================================================================\n";
    cout << "OPERACION FINALIZADA. TODAS LAS COMUNIDADES ATENDIDAS.\n";
    cout << "Distancia Total Acumulada: " << distanciaTotalOperacion << " km." << endl;
    cout << "Viajes realizados: " << numeroViaje - 1 << endl;
    cout << "======================================================================\n";

    return 0;
}