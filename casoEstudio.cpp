#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <string>
#include <cmath>

const double INF = 1e9;

struct Comunidad {
    int id;
    double recursosNecesarios; 
    int prioridad;             
    double distanciaAlmacen;   
    double ratio = 0.0;        
};

struct EntregaViaje {
    int idComunidad;
    double cantidadEntregada;
    double distanciaRecorridaParaLlegar; // Distancia desde el punto anterior
};

struct Ubicacion {
    int idOriginal;
    std::string nombre;
};

// --- Prototipos ---
void imprimirEstadoDiario(const std::vector<Comunidad>& comunidades);
void calcularRutasOptimasFloydWarshall(const std::vector<EntregaViaje>& entregas, const std::vector<std::vector<double>>& distanciasCompletas);

// NUEVA función que recibe la matriz completa para calcular distancias reales
std::vector<EntregaViaje> planificarRutaDinamica(
    std::vector<Comunidad>& comunidades, 
    const std::vector<std::vector<double>>& matrizDistancias,
    double capacidadVehiculo, 
    double tiempoMaximo,
    double tiempoPorEntrega
);

int main() {
    // --- PARÁMETROS ---
    const double CAPACIDAD_VEHICULO = 11.0;     
    const double TIEMPO_MAXIMO_OPERACION = 8.0; 
    const double TIEMPO_POR_ENTREGA = 0.5;      

    // Datos Iniciales
    std::vector<Comunidad> todasLasComunidades = {
        {1, 8.0, 80, 2.5},
        {2, 3.0, 40, 3.0}, 
        {3, 6.0, 95, 1.5},
        {4, 4.0, 70, 4.0}, // Antes era imposible (8.5h), ahora será posible si se llega desde C1 o C5
        {5, 5.0, 85, 1.0},
        {6, 15.0, 50, 4.0} 
    };
    
    // Matriz: 0=Alm, 1=C1, 2=C2, 3=C3, 4=C4, 5=C5, 6=C6
    std::vector<std::vector<double>> distanciasCompletas = {
        {0,  2.5, 3.0, 1.5, 4.0, 1.0, 4.0}, // 0: Almacén
        {2.5, 0,  1.2, 1.8, 1.0, 2.0, 3.5}, // 1: C1
        {3.0, 1.2, 0,   2.5, 1.5, 2.8, 2.0}, // 2: C2
        {1.5, 1.8, 2.5, 0,   1.2, 0.8, 3.0}, // 3: C3
        {4.0, 1.0, 1.5, 1.2, 0,   1.5, 2.8}, // 4: C4 (¡Ojo! C4->C1 es 1.0, mucho mejor que ir desde Almacén)
        {1.0, 2.0, 2.8, 0.8, 1.5, 0,   3.2}, // 5: C5
        {4.0, 3.5, 2.0, 3.0, 2.8, 3.2, 0  }  // 6: C6
    };

    std::cout << "--- SIMULACION LOGISTICA: RUTA DINAMICA ---\n";
    
    int dia = 1;
    bool quedanRecursosPendientes = true;

    while (quedanRecursosPendientes) {
        std::cout << "\n=======================================================\n";
        std::cout << "                 INICIO DEL DIA " << dia << "\n";
        std::cout << "=======================================================\n";
        
        imprimirEstadoDiario(todasLasComunidades);

        // 1. Planificar Ruta usando Distancias Dinámicas (Matriz)
        std::vector<EntregaViaje> entregasHoy = planificarRutaDinamica(
            todasLasComunidades, 
            distanciasCompletas,
            CAPACIDAD_VEHICULO, 
            TIEMPO_MAXIMO_OPERACION, 
            TIEMPO_POR_ENTREGA
        );

        if (entregasHoy.empty()) {
            std::cout << "\n[!] ALERTA: Quedan comunidades pero no se pueden alcanzar en el tiempo limite (8h).\n";
            break;
        }

        // 2. Resumen y actualización
        std::cout << "\n---> RUTA EJECUTADA DIA " << dia << ":\n";
        std::cout << "+----+-----------+------------+\n";
        std::cout << "| ID | Carga (T) | Dist. Viaje|\n";
        std::cout << "+----+-----------+------------+\n";
        
        for (const auto& entrega : entregasHoy) {
            auto it = std::find_if(todasLasComunidades.begin(), todasLasComunidades.end(), 
                                   [&](const Comunidad& c){ return c.id == entrega.idComunidad; });
            
            if (it != todasLasComunidades.end()) {
                it->recursosNecesarios -= entrega.cantidadEntregada;
                if (it->recursosNecesarios < 0.001) it->recursosNecesarios = 0;

                std::cout << "| " << std::setw(2) << entrega.idComunidad 
                          << " | " << std::setw(9) << std::fixed << std::setprecision(2) << entrega.cantidadEntregada 
                          << " | " << std::setw(8) << entrega.distanciaRecorridaParaLlegar << "km |\n";
            }
        }
        std::cout << "+----+-----------+------------+\n";

        // 3. Limpieza
        todasLasComunidades.erase(
            std::remove_if(todasLasComunidades.begin(), todasLasComunidades.end(), 
                [](const Comunidad& c) { return c.recursosNecesarios <= 0; }
            ), 
            todasLasComunidades.end()
        );

        if (todasLasComunidades.empty()) {
            quedanRecursosPendientes = false;
            std::cout << "\n*** MISION CUMPLIDA ***\n";
        } else {
            dia++;
        }
    }

    return 0;
}

// --- IMPLEMENTACIÓN MEJORADA ---

void imprimirEstadoDiario(const std::vector<Comunidad>& comunidades) {
    std::cout << "Recursos pendientes (Ordenados por ID):\n";
    std::cout << "+----+----------+-----------+-------+---------+\n";
    std::cout << "| ID | Falta(T) | Prioridad | Dist. | Ratio   |\n";
    std::cout << "+----+----------+-----------+-------+---------+\n";
    for (const auto& c : comunidades) {
        // Calculamos el ratio aquí solo para visualizar, aunque se recalcula dentro de la función
        double r = (c.distanciaAlmacen > 0) ? c.prioridad / c.distanciaAlmacen : 0;
        std::cout << "| " << std::setw(2) << c.id
                  << " | " << std::setw(8) << std::fixed << std::setprecision(1) << c.recursosNecesarios
                  << " | " << std::setw(9) << c.prioridad
                  << " | " << std::setw(5) << c.distanciaAlmacen 
                  << " | " << std::setw(7) << std::fixed << std::setprecision(1) << r << " |\n";
    }
    std::cout << "+----+----------+-----------+-------+---------+\n";
}

std::vector<EntregaViaje> planificarRutaDinamica(
    std::vector<Comunidad>& comunidades, 
    const std::vector<std::vector<double>>& matrizDistancias,
    double capacidadVehiculo, 
    double tiempoMaximo,
    double tiempoPorEntrega) 
{
    std::vector<EntregaViaje> entregas;
    double capacidadDisponible = capacidadVehiculo;
    double tiempoConsumido = 0.0;
    int ubicacionActual = 0; // Empezamos en el Almacén (ID 0)

    // 1. Calcular ratios
    for (auto& c : comunidades) {
        if (c.distanciaAlmacen > 0) c.ratio = c.prioridad / c.distanciaAlmacen;
        else c.ratio = INF;
    }

    // 2. Copia temporal para ordenar e ir marcando visitados en este día
    std::vector<Comunidad> candidatos = comunidades;
    std::vector<bool> visitadoHoy(50, false); // Asumiendo max ID < 50

    // Ordenar por Ratio (Mayor prioridad/distancia primero)
    std::sort(candidatos.begin(), candidatos.end(), [](const Comunidad& a, const Comunidad& b) {
        return a.ratio > b.ratio;
    });

    bool seAgregoAlgo = true;
    
    // Bucle greedy: Mientras quepa algo y tengamos tiempo
    while (seAgregoAlgo && capacidadDisponible > 0) {
        seAgregoAlgo = false;

        // Recorremos los candidatos en orden de Ratio
        for (auto& cand : candidatos) {
            if (visitadoHoy[cand.id]) continue; // Ya lo visitamos hoy

            // --- AQUI ESTA EL CAMBIO CLAVE ---
            // Distancia desde donde estoy AHORA (ubicacionActual) hacia el candidato
            double distIr = matrizDistancias[ubicacionActual][cand.id];
            
            // Distancia para volver al almacén desde el candidato (por seguridad)
            double distVolverAlmacen = matrizDistancias[cand.id][0];

            double tiempoOperacion = distIr + tiempoPorEntrega + distVolverAlmacen; // Tiempo total si fuera la última parada
            
            // Chequeo: Tiempo actual + lo que tardo en ir y descargar + volver a casa <= Maximo
            // Nota: 'tiempoConsumido' ya incluye el trayecto hasta 'ubicacionActual', pero NO la vuelta desde ahí.
            // Por eso sumamos ir + volver desde el nuevo punto.
            
            // Ajuste fino: Si ya estamos en ruta, restamos la "vuelta virtual" anterior, pero simplifiquemos:
            // Vamos a sumar incrementalmente.
            
            double tiempoNuevoAcumulado = tiempoConsumido + distIr + tiempoPorEntrega;
            
            // ¿Podemos volver a casa después de esta entrega?
            if (tiempoNuevoAcumulado + distVolverAlmacen <= tiempoMaximo) {
                
                // SI CABE EN TIEMPO: CALCULAMOS CARGA
                double llevar = std::min(cand.recursosNecesarios, capacidadDisponible);
                
                if (llevar > 0) {
                    entregas.push_back({cand.id, llevar, distIr});
                    capacidadDisponible -= llevar;
                    
                    // Actualizamos estado
                    tiempoConsumido += (distIr + tiempoPorEntrega);
                    ubicacionActual = cand.id; // El camión se mueve aquí
                    visitadoHoy[cand.id] = true;
                    seAgregoAlgo = true;
                    
                    std::cout << "   -> Agregado C" << cand.id << ". Ruta: ... -> " << distIr << "km -> C" << cand.id 
                              << ". T.Acum: " << tiempoConsumido << "h\n";
                    
                    // IMPORTANTE: Al ser greedy, al encontrar el mejor ratio disponible que cabe, 
                    // lo tomamos y reiniciamos el bucle para re-evaluar distancias desde el NUEVO punto?
                    // En este enfoque simple, seguimos la lista de ratios originales. 
                    // Para ser más preciso (TSP greedy), deberíamos reordenar por cercanía, 
                    // pero tu pregunta era sobre el RATIO. Así que respetamos el Ratio.
                    break; // Rompemos el for para buscar el siguiente desde la nueva ubicación (opcional, o seguir lista)
                }
            }
        }
    }

    // Sumar el tiempo de retorno final real
    if (ubicacionActual != 0) {
        tiempoConsumido += matrizDistancias[ubicacionActual][0];
        std::cout << "   -> Regreso a Almacen: " << matrizDistancias[ubicacionActual][0] << "km. Tiempo Total: " << tiempoConsumido << "h\n";
    }

    return entregas;
}