// =============================================================
// WattControl v2 - Gestor de energia de un taller de mecanizado (El Nino 2026)
// Evidencia EA2: herencia, polimorfismo, composicion, agregacion y asociacion
// =============================================================
// --- Librerias --------------------------------------------------------------
// iostream: cout (escribir en pantalla) y cin (leer del teclado)
#include <iostream>
// iomanip: setw (ancho de columna) y setprecision (cantidad de decimales)
#include <iomanip>
// string: el tipo string, para guardar textos
#include <string>
// cstdlib: rand() y srand(), para generar numeros al azar
#include <cstdlib>

using namespace std;

// --- Directivas del preprocesador ------------------------------------------
// Horas de la simulacion: un dia
#define HORAS_DIA 24
// Cuantas cargas puede tener el negocio, como maximo (tamano del arreglo)
#define MAX_CARGAS 5
// Semilla de rand(): con la misma semilla, todos obtienen los mismos precios al azar
#define SEMILLA 2026

// --- Constantes: todos los valores del taller en un solo lugar ---------
// kW de la iluminacion y los computadores (carga critica)
const float POTENCIA_ILUMINACION = 1.5;
// kW del torno CNC (aplazable 1)
const float POTENCIA_TORNO = 7.5;
// kW del compresor (aplazable 2)
const float POTENCIA_COMPRESOR = 3.0;
// kW contratados con la empresa de energia
const float POTENCIA_CONTRATADA = 10.0;
// COP/kWh: si el precio pasa de aqui, la energia esta cara
const float PRECIO_CARO = 750.0;
// COP/kWh: con este precio y embalses bajos hay emergencia de racionamiento
const float PRECIO_ESCASEZ = 1500.0;
// La hora pico empieza a las 18 h...
const int PICO_INICIO = 18;
// ...y termina a las 20 h (la hora 20 esta incluida)
const int PICO_FIN = 20;
// %: si los embalses estan por debajo, hay riesgo de racionamiento
const float EMBALSE_ALERTA = 40.0;
// % de los embalses al empezar (septiembre 2026: 78 % de volumen util)
const float EMBALSE_INICIAL = 78.0;
// % que bajan los embalses cada hora de simulacion (El Nino: llueve menos)
const float BAJADA_EMBALSE_HORA = 0.5;
// Horas minimas de funcionamiento al dia (justificacion en el PDF)
const int HORAS_MIN_TORNO = 4;
const int HORAS_MIN_COMPRESOR = 3;
// 1 de cada 20 lecturas del precio falla
const int FALLA_CADA = 20;

// =============================================================
// Clase base Carga: lo que tienen en comun todas las cargas
// =============================================================
class Carga {
protected:
    // El nombre de la carga: "Iluminacion", "Torno CNC"
    string nombre;
    // La potencia en kW
    float potenciaKW;
    // true si esta encendida en esta hora; false si esta apagada
    bool encendida;
    // Cuantas horas ha estado encendida (para el resumen)
    int horasUso;
    // Horas minimas que debe funcionar en el dia (0 si no tiene minimo)
    int horasMinimas;

public:
    // Constructor: recibe nombre, potencia y horas minimas (por defecto 0)
    Carga(string n, float p, int hMin = 0) {
        nombre = n;
        potenciaKW = p;
        horasMinimas = hMin;
        // Toda carga empieza apagada y sin horas de uso
        encendida = false;
        horasUso = 0;
    }

    // Destructor virtual: obligatorio en una clase base con metodos virtuales
    virtual ~Carga() {}

    // Polimorfismo: cada clase hija decide a su manera si se enciende.
    // "= 0" = metodo virtual puro: Carga es ABSTRACTA, cada hija debe implementarlo
    virtual bool debeEncender(int hora, float precio, float embalses) = 0;

    // Dice si es carga critica (nunca se apaga). Por defecto no lo es;
    // CargaCritica lo cambia. Lo usa el Negocio en la regla 6 y en la falla de lectura
    virtual bool esCritica() { return false; }

    // Enciende la carga durante esta hora
    void encender() {
        encendida = true;
        horasUso++;
    }

    // Apaga la carga durante esta hora
    void apagar() {
        encendida = false;
    }

    // Deja la carga como nueva: se usa al empezar cada simulacion
    void reiniciar() {
        encendida = false;
        horasUso = 0;
    }

    // Costo de esta hora: kW x precio si esta encendida; 0 si esta apagada
    float costoHora(float precio) {
        if (encendida) {
            return potenciaKW * precio;
        }
        return 0;
    }

    // Getters
    string getNombre() { return nombre; }
    float getPotencia() { return potenciaKW; }
    bool estaEncendida() { return encendida; }
    int getHorasUso() { return horasUso; }
    int getHorasMinimas() { return horasMinimas; }
};

// =============================================================
// Herencia: CargaCritica ES UNA Carga (nunca se apaga)
// =============================================================
class CargaCritica : public Carga {
public:
    CargaCritica(string n, float p) : Carga(n, p) {}

    // Regla 1: la carga critica siempre permanece encendida.
    // Recibe hora, precio y embalses aunque no los use, porque debe tener
    // la misma forma que debeEncender de Carga
    bool debeEncender(int hora, float precio, float embalses) override {
        return true;
    }

    // Esta carga si es critica
    bool esCritica() override { return true; }
};

// =============================================================
// Herencia: CargaAplazable ES UNA Carga (se aplaza si la energia no conviene)
// =============================================================
class CargaAplazable : public Carga {
public:
    CargaAplazable(string n, float p, int hMin) : Carga(n, p, hMin) {}

    // Reglas 2 a 5 como una sola condicion compuesta
    bool debeEncender(int hora, float precio, float embalses) override {
        // Regla 3: hora pico si la hora esta entre 18 y 20 (&& = "y")
        bool horaPico = hora >= PICO_INICIO && hora <= PICO_FIN;
        // Regla 2: embalses por debajo del 40 %
        bool embalsesBajos = embalses < EMBALSE_ALERTA;
        // Regla 4: el precio supera el precio "caro" del taller
        bool energiaCara = precio > PRECIO_CARO;
        // Regla 5: en otro caso se enciende. Se aplaza si ocurre cualquiera de las
        // tres (||); el ! lo niega: enciende solo cuando no ocurre ninguna
        return !(embalsesBajos || horaPico || energiaCara);
    }
};

// =============================================================
// Medidor: parte del Negocio (composicion). Acumula kWh y costo
// =============================================================
class Medidor {
private:
    // Energia acumulada en kWh
    float kWh;
    // Costo acumulado en pesos
    float costo;

public:
    // El medidor empieza en cero
    Medidor() {
        kWh = 0;
        costo = 0;
    }

    // Registra una hora de una carga encendida: kW durante 1 hora = kW en kWh
    void registrar(float kW, float precio) {
        kWh = kWh + kW;
        costo = costo + kW * precio;
    }

    // Vuelve a cero (al empezar una simulacion)
    void reiniciar() {
        kWh = 0;
        costo = 0;
    }

    float getKWh() { return kWh; }
    float getCosto() { return costo; }
};

// =============================================================
// Mercado: da el precio de cada hora y el nivel de los embalses (asociacion)
// =============================================================
class Mercado {
public:
    // Precio al azar dentro del rango real de la franja (XM, 21-27 sep 2026)
    float precioHora(int hora) {
        int minimo, maximo;
        // De 0 a 6 h: madrugada
        if (hora <= 6) {
            minimo = 398; maximo = 640;
        // De 7 a 14 h: manana y mediodia
        } else if (hora <= 14) {
            minimo = 456; maximo = 970;
        // De 15 a 17 h: tarde
        } else if (hora <= 17) {
            minimo = 596; maximo = 1040;
        // De 18 a 20 h: hora pico
        } else if (hora <= 20) {
            minimo = 705; maximo = 1612;
        // De 21 a 23 h: noche
        } else {
            minimo = 565; maximo = 970;
        }
        // rand() % (maximo - minimo + 1) da de 0 a la diferencia; sumarle el minimo
        // deja el precio entre minimo y maximo
        return minimo + rand() % (maximo - minimo + 1);
    }

    // Nivel de los embalses (%) en una hora: baja un poco cada hora por El Nino
    float nivelEmbalses(float nivelInicial, int hora) {
        float nivel = nivelInicial - BAJADA_EMBALSE_HORA * hora;
        // El nivel nunca queda por debajo de 0
        if (nivel < 0) nivel = 0;
        return nivel;
    }
};

// =============================================================
// Negocio: junta todo
// =============================================================
class Negocio {
private:
    // El nombre del negocio
    string nombre;
    // COMPOSICION: los medidores son atributos objeto. Nacen y mueren con el negocio.
    // medidor: lo que se gasta con gestor; medidorSin: lo que se gastaria sin gestor
    Medidor medidor;
    Medidor medidorSin;
    // AGREGACION: arreglo de punteros (*) a Carga. Las cargas se crean afuera
    // y aqui solo se guarda su direccion
    Carga* cargas[MAX_CARGAS];
    // Cuantas cargas se han agregado
    int numCargas;
    // ASOCIACION: puntero al mercado. El negocio no lo crea: solo lo conoce
    // y le pide los datos
    Mercado* mercado;

    // Resultados de la ultima simulacion, para el resumen (opcion 4)
    // horasSimuladas: horas que alcanzo a simular; fallas: lecturas que fallaron
    int horasSimuladas, fallas;
    // true si hubo emergencia de racionamiento (break)
    bool emergencia;
    // true si alguna aplazable no cumplio sus horas minimas
    bool produccionIncompleta;

    // Decide que cargas se encienden en una hora y lo deja en on[].
    // Pregunta debeEncender() a todas con el mismo ciclo (polimorfismo) y aplica
    // la regla 6. Devuelve true si se supero la potencia contratada
    bool decidirCargas(int hora, float precio, float embalses, bool on[]) {
        // Suma de kW de las cargas que quieren encenderse
        float potenciaEncendida = 0;
        for (int i = 0; i < numCargas; i++) {
            // Polimorfismo: la misma llamada; cada carga responde con su regla
            on[i] = cargas[i]->debeEncender(hora, precio, embalses);
            if (on[i]) potenciaEncendida += cargas[i]->getPotencia();
        }
        // Regla 6: si la potencia encendida supera la contratada...
        bool alerta = potenciaEncendida > POTENCIA_CONTRATADA;
        if (alerta) {
            // ...se busca la aplazable encendida de mayor potencia (-1 = ninguna todavia)...
            int mayor = -1;
            for (int i = 0; i < numCargas; i++) {
                if (on[i] && !cargas[i]->esCritica() &&
                    (mayor == -1 || cargas[i]->getPotencia() > cargas[mayor]->getPotencia())) {
                    mayor = i;
                }
            }
            // ...y se apaga
            if (mayor != -1) on[mayor] = false;
        }
        return alerta;
    }

public:
    // Constructor: recibe el nombre y la direccion del mercado (Mercado* m)
    Negocio(string n, Mercado* m) {
        nombre = n;
        mercado = m;
        numCargas = 0;
        horasSimuladas = 0; fallas = 0;
        emergencia = false;
        produccionIncompleta = false;
    }

    // Agrega una carga al negocio. Recibe su direccion (Carga* c)
    void agregarCarga(Carga* c) {
        if (numCargas < MAX_CARGAS) {
            cargas[numCargas] = c;
            numCargas++;
        }
    }

    // Muestra el negocio y sus cargas
    void mostrar() {
        cout << "Negocio " << nombre << " | potencia contratada " << POTENCIA_CONTRATADA << " kW" << endl;
        for (int i = 0; i < numCargas; i++) {
            cout << "  " << cargas[i]->getNombre() << " (" << cargas[i]->getPotencia() << " kW)" << endl;
        }
    }

    // Que haria cada carga con una lectura (hora, precio, embalses)
    void evaluar(int hora, float precio, float embalses) {
        bool on[MAX_CARGAS];
        bool alerta = decidirCargas(hora, precio, embalses, on);
        // Regla 2: alerta de riesgo de racionamiento
        if (embalses < EMBALSE_ALERTA) {
            cout << "  ALERTA: riesgo de racionamiento, solo funciona la carga critica" << endl;
        }
        for (int i = 0; i < numCargas; i++) {
            cout << "  " << setw(12) << left << cargas[i]->getNombre() << right
                 << (on[i] ? "ENCENDER" : "APAGAR / APLAZAR") << endl;
        }
        // Regla 6: alerta si se supero la potencia contratada
        if (alerta) {
            cout << "  ALERTA: se supera la potencia contratada de " << POTENCIA_CONTRATADA
                 << " kW; se apaga la aplazable de mayor potencia" << endl;
        }
    }

    // Simula 24 horas, con gestor y sin gestor. Los embalses empiezan en embalsesIniciales
    void simularDia(float embalsesIniciales) {
        // Los medidores y los resultados empiezan de nuevo en 0
        medidor.reiniciar();
        medidorSin.reiniciar();
        fallas = 0; horasSimuladas = 0;
        emergencia = false;
        produccionIncompleta = false;
        // Todas las cargas empiezan el dia apagadas y sin horas de uso
        for (int i = 0; i < numCargas; i++) cargas[i]->reiniciar();

        cout << fixed << setprecision(0);
        // Encabezado de la tabla
        cout << setw(5) << "Hora" << setw(8) << "Precio" << setw(10) << "Embalses";
        for (int i = 0; i < numCargas; i++) cout << setw(12) << cargas[i]->getNombre();
        cout << setw(9) << "Costo" << endl;

        // Repite para cada hora del dia: h = 0, 1, ..., 23
        for (int h = 0; h < HORAS_DIA; h++) {
            // ASOCIACION: el negocio le pide al mercado el precio y el nivel de embalses
            float precio = mercado->precioHora(h);
            float embalses = mercado->nivelEmbalses(embalsesIniciales, h);

            // break: precio de escasez con embalses bajos -> emergencia de racionamiento
            if (precio > PRECIO_ESCASEZ && embalses < EMBALSE_ALERTA) {
                cout << setw(5) << h << setw(8) << precio << setw(10) << embalses
                     << "  EMERGENCIA DE RACIONAMIENTO: se termina la simulacion" << endl;
                emergencia = true;
                // break SALE del for: no se simulan las horas que faltan
                break;
            }
            // Si llego aqui, esta hora si se simula
            horasSimuladas++;

            // Sin gestor: las tres cargas encendidas toda la hora
            for (int i = 0; i < numCargas; i++) {
                medidorSin.registrar(cargas[i]->getPotencia(), precio);
            }

            // continue: falla la lectura del precio -> esa hora solo funciona la critica
            if (rand() % FALLA_CADA == 0) {
                float costoFalla = 0;
                for (int i = 0; i < numCargas; i++) {
                    if (cargas[i]->esCritica()) {
                        cargas[i]->encender();
                        medidor.registrar(cargas[i]->getPotencia(), precio);
                        costoFalla += cargas[i]->costoHora(precio);
                    } else {
                        cargas[i]->apagar();
                    }
                }
                fallas++;
                cout << setw(5) << h << setw(8) << "--" << setw(10) << embalses
                     << "  falla de lectura: solo la carga critica ($" << costoFalla << ")" << endl;
                // continue SALTA a la siguiente hora
                continue;
            }

            // Con gestor: cada carga decide (polimorfismo) y se aplica la regla 6
            bool on[MAX_CARGAS];
            bool alerta = decidirCargas(h, precio, embalses, on);
            float costoHora = 0;
            cout << setw(5) << h << setw(8) << precio << setw(10) << embalses;
            for (int i = 0; i < numCargas; i++) {
                if (on[i]) {
                    cargas[i]->encender();
                    medidor.registrar(cargas[i]->getPotencia(), precio);
                } else {
                    cargas[i]->apagar();
                }
                cout << setw(12) << (cargas[i]->estaEncendida() ? "ON" : "off");
                costoHora += cargas[i]->costoHora(precio);
            }
            cout << setw(9) << costoHora;
            // Avisos de la hora
            if (embalses < EMBALSE_ALERTA) cout << "  RIESGO DE RACIONAMIENTO";
            if (alerta) cout << "  ALERTA: potencia contratada superada";
            cout << endl;
        }

        // Horas minimas: cada carga con minimo debe haberlas cumplido
        for (int i = 0; i < numCargas; i++) {
            if (cargas[i]->getHorasUso() < cargas[i]->getHorasMinimas()) {
                produccionIncompleta = true;
            }
        }
        if (produccionIncompleta) cout << "ALERTA: produccion incompleta" << endl;
        cout << "Simulacion terminada. Vea el resumen con la opcion 4." << endl;
    }

    // Opcion 4: resumen y ahorro
    void resumen() {
        cout << fixed << setprecision(0);
        cout << "Negocio " << nombre << endl;
        cout << "Horas simuladas: " << horasSimuladas << " de " << HORAS_DIA
             << " | fallas de lectura: " << fallas << endl;
        if (emergencia) cout << "La simulacion termino por emergencia de racionamiento." << endl;
        // Una linea por carga: horas encendida y energia gastada (horas x kW = kWh)
        for (int i = 0; i < numCargas; i++) {
            cout << "  " << setw(12) << left << cargas[i]->getNombre() << right
                 << cargas[i]->getHorasUso() << " h, "
                 << cargas[i]->getHorasUso() * cargas[i]->getPotencia() << " kWh";
            // Las aplazables muestran su minimo de horas
            if (cargas[i]->getHorasMinimas() > 0) {
                cout << " (minimo " << cargas[i]->getHorasMinimas() << " h)";
            }
            cout << endl;
        }
        cout << "Energia total con gestor: " << medidor.getKWh() << " kWh" << endl;
        cout << "Costo con gestor: $" << medidor.getCosto() << endl;
        cout << "Costo sin gestor: $" << medidorSin.getCosto() << endl;
        // Ahorro: lo que se paga sin gestor menos lo que se paga con gestor
        float ahorro = medidorSin.getCosto() - medidor.getCosto();
        cout << "Ahorro:           $" << ahorro;
        // En porcentaje, con 1 decimal. Solo si el costo sin gestor > 0, para no dividir por 0
        if (medidorSin.getCosto() > 0) {
            cout << " (" << setprecision(1) << ahorro / medidorSin.getCosto() * 100 << " %)";
        }
        cout << endl;
        if (produccionIncompleta) cout << "ALERTA: produccion incompleta" << endl;
    }
};

// main: el programa empieza a ejecutarse aqui
int main() {
    srand(SEMILLA);

    // El mercado existe por su cuenta: no es parte del negocio
    Mercado mercado;
    // Crea las tres cargas del taller (fuera del negocio)
    CargaCritica iluminacion("Iluminacion", POTENCIA_ILUMINACION);
    CargaAplazable torno("Torno CNC", POTENCIA_TORNO, HORAS_MIN_TORNO);
    CargaAplazable compresor("Compresor", POTENCIA_COMPRESOR, HORAS_MIN_COMPRESOR);
    // Crea el negocio y le pasa la direccion del mercado: asociacion
    Negocio taller("Taller de mecanizado", &mercado);
    // Le agrega las cargas (agregacion)
    taller.agregarCarga(&iluminacion);
    taller.agregarCarga(&torno);
    taller.agregarCarga(&compresor);
    taller.mostrar();

    // Lectura manual: empieza con valores de ejemplo
    int hora = 12;
    float embalses = EMBALSE_INICIAL, precio = 500;
    bool haySimulacion = false;
    int opcion;

    do {
        cout << endl << "===== WattControl v2 =====" << endl;
        cout << "1. Ingresar lectura manual (hora, precio, embalses)" << endl;
        cout << "2. Evaluar decision de cada carga" << endl;
        cout << "3. Simular un dia completo (24 h)" << endl;
        cout << "4. Ver resumen de la ultima simulacion" << endl;
        cout << "0. Salir" << endl;
        cout << "Opcion: ";
        cin >> opcion;

        switch (opcion) {
        // Opcion 1: pedir la lectura, validando cada dato
        case 1:
            do {
                cout << "Hora (0 a 23): ";
                cin >> hora;
                if (hora < 0 || hora > 23) cout << "  Hora invalida." << endl;
            } while (hora < 0 || hora > 23);
            do {
                cout << "Precio de bolsa (COP/kWh): ";
                cin >> precio;
                if (precio <= 0) cout << "  El precio debe ser mayor que 0." << endl;
            } while (precio <= 0);
            do {
                cout << "Nivel de los embalses (0 a 100 %): ";
                cin >> embalses;
                if (embalses < 0 || embalses > 100) cout << "  Nivel invalido." << endl;
            } while (embalses < 0 || embalses > 100);
            cout << "Lectura guardada." << endl;
            break;
        // Opcion 2: que haria cada carga con la lectura guardada
        case 2:
            cout << "Hora " << hora << " | precio " << precio << " | embalses " << embalses << " %" << endl;
            taller.evaluar(hora, precio, embalses);
            break;
        // Opcion 3: simular las 24 horas. Los embalses empiezan en el nivel de la lectura
        case 3:
            cout << "Embalses iniciales: " << embalses << " %" << endl;
            taller.simularDia(embalses);
            haySimulacion = true;
            break;
        // Opcion 4: el resumen, solo si ya se simulo un dia
        case 4:
            if (haySimulacion) taller.resumen();
            else cout << "Primero simule un dia con la opcion 3." << endl;
            break;
        case 0:
            cout << "Hasta luego." << endl;
            break;
        default:
            cout << "Opcion invalida." << endl;
        }
        // Pausa: espera a que el usuario lea el resultado antes de volver a mostrar el menu
        if (opcion != 0) {
            cout << endl << "Presione Enter para continuar...";
            cin.ignore();
            cin.get();
        }
    } while (opcion != 0);

    return 0;
}
