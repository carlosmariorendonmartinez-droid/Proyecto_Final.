#include "portada.h"               // Incluye la definición de la clase Portada
#include "ui_mainwindow.h"         // Incluye la interfaz generada por Qt Designer
#include "nivel1.h"                // Clase del nivel 1
#include "nivel2.h"                // Clase del nivel 2
#include "nivel3.h"                // Clase del nivel 3

/*
 * RELACIÓN ENTRE CLASES:
 * - Portada depende de Nivel1, Nivel2, Nivel3 → Composición dinámica.
 * - Portada crea niveles, escucha señales y los destruye.
 * - Ui::MainWindow administra la interfaz → patrón MVC simple.
 */

#include <QVBoxLayout>  // QVBoxLayout: organiza widgets verticalmente
#include <QListWidget>  // QListWidget: lista visual de elementos
#include <QPushButton>  // QPushButton: botón clickeable
#include <QMessageBox>  // QMessageBox: muestra alertas y mensajes
#include <QFont>        // QFont: manipulación de fuentes y tamaños de letra
#include <QLabel>       // QLabel: texto estático o imágenes
#include <QDialog>      // QDialog: ventana modal para diálogos

// Constructor de Portada (se ejecuta al iniciar el juego)
// parent → puntero al "padre" en la jerarquía de Qt. Aquí casi siempre es nullptr.
Portada::Portada(QWidget *parent)
    : QMainWindow(parent),          // Llama al constructor de QMainWindow con el padre
    ui(new Ui::MainWindow),         // Reserva memoria para la interfaz gráfica
    nivelActual(0),                 // 0 significa “ningún nivel cargado aún”
    nivelActivo(nullptr)            // Apunta al nivel actual (N1, N2 o N3). nullptr = no hay nivel
{
    ui->setupUi(this); // Carga la interfaz diseñada en Qt Designer dentro de esta ventana

    showFullScreen();  // Muestra esta ventana ocupando TODA la pantalla del PC

    // --- AUDIO ---
    audioOutput = new QAudioOutput(this); // Crea el sistema de salida de audio
    player = new QMediaPlayer(this);      // Reproductor de audio
    player->setAudioOutput(audioOutput);  // Conecta el reproductor al sistema de salida
    player->setSource(QUrl("qrc:/sounds/batalla.mp3")); // Carga música desde recursos (qrc)
    audioOutput->setVolume(0.5); // Volumen entre 0.0 (silencio) y 1.0 (máximo)
    player->setLoops(QMediaPlayer::Infinite); // Reproduce en bucle infinito
    player->play(); // Inicia la música de fondo

    setMenuAsCentral(); // Crea y coloca el menú principal como pantalla central
}

Portada::~Portada()
{
    // Si existe un nivel activo, lo cerramos y lo marcamos para destrucción
    if (nivelActivo) {
        nivelActivo->close();        // Cierra la ventana del nivel
        nivelActivo->deleteLater();  // Qt lo elimina más adelante de forma segura
        nivelActivo = nullptr;       // Limpia el puntero para evitar errores
    }
    delete ui; // Libera la memoria asignada al sistema de interfaz
}


// Crea y configura el menú principal como contenido central
void Portada::setMenuAsCentral()
{
    menuWidget_ = new QWidget(this); // Widget que será el contenedor principal del menú
    menuWidget_->setObjectName("centralWidget"); // Nombre para usar en el stylesheet

    QVBoxLayout *layout = new QVBoxLayout(menuWidget_); // Layout vertical para el menú
    layout->setContentsMargins(0, 0, 0, 0); // Márgenes internos del layout (izq, arriba, der, abajo)

    QLabel *titulo = new QLabel(tr("IMPERIO ROMANO"), menuWidget_);
    titulo->setAlignment(Qt::AlignCenter); // Centra el texto horizontalmente

    QFont f = titulo->font(); // Obtiene la fuente actual del título
    f.setPointSize(36);       // Aumenta tamaño de letra
    f.setBold(true);          // Activa negrita
    titulo->setFont(f);       // Aplica la fuente modificada

    // Cambia color y agrega sombra al texto
    titulo->setStyleSheet("color: rgb(255,215,0); text-shadow: 2px 2px 4px black;");

    // Creación de botones del menú
    QPushButton *btnJugar = new QPushButton(tr("Jugar"), menuWidget_);
    QPushButton *btnNiveles = new QPushButton(tr("Seleccionar Nivel"), menuWidget_);
    QPushButton *btnSalir = new QPushButton(tr("Salir"), menuWidget_);

    // Contenedor que agrupa los botones
    QList<QPushButton*> botones = {btnJugar, btnNiveles, btnSalir};

    // Aplica estilo a todos los botones
    for (auto *btn : botones) {
        btn->setFixedSize(250, 60); // Tamaño fijo del botón... 250 es ancho y 60 es largo.
        btn->setStyleSheet(
            "QPushButton {"
            " background-color: rgba(0,0,0,170);"
            " color: #FFFFFF;"
            " border-radius: 12px;"
            " font-size: 20px;"
            " font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: rgba(255,255,255,40); }"
        );
        layout->addWidget(btn, 0, Qt::AlignHCenter); // Agrega botón al layout
    }

    layout->setSpacing(25); // Espaciado vertical entre widgets
    layout->insertWidget(0, titulo, 0, Qt::AlignHCenter); // Inserta el título arriba

    // Fondo del menú usando border-image
    menuWidget_->setStyleSheet(
        "QWidget#centralWidget {"
        " border-image: url(:/images/portada.jpeg) 0 0 0 0 stretch stretch;"
        "}"
    );

    // Conexión de señales (patrón OBSERVER de Qt)
    connect(btnJugar, &QPushButton::clicked, this, &Portada::onBtnJugarClicked);
    connect(btnNiveles, &QPushButton::clicked, this, &Portada::onBtnNivelesClicked);
    connect(btnSalir, &QPushButton::clicked, this, &Portada::close);

    setCentralWidget(menuWidget_); // Coloca el menú como "pantalla principal"
}


// Se ejecuta al presionar el botón JUGAR
void Portada::onBtnJugarClicked()
{
    nivelActual = 1;             // Comienza en el nivel 1
    mostrarNivel(nivelActual);   // Llama a la función que abre el nivel
}


// Se ejecuta al presionar SELECCIONAR NIVEL
void Portada::onBtnNivelesClicked()
{
    int seleccionado = mostrarDialogoSeleccionNivel(); // Abre ventana y devuelve nivel elegido

    if (seleccionado >= 1 && seleccionado <= 3) { // Solo niveles válidos
        nivelActual = seleccionado; // Guarda el nivel seleccionado
        mostrarNivel(nivelActual);  // Abre ese nivel
    }
}


// Crea un diálogo con lista de niveles disponible
int Portada::mostrarDialogoSeleccionNivel()
{
    QDialog dlg(this); // Ventana modal hija de Portada
    dlg.setWindowTitle(tr("Seleccionar nivel"));
    dlg.setMinimumSize(350, 260);

    QVBoxLayout *v = new QVBoxLayout(&dlg);

    QListWidget *list = new QListWidget(&dlg); // Lista visual
    list->addItem("Nivel 1 - Coliseo Romano");
    list->addItem("Nivel 2 - Escape del Templo");
    list->addItem("Nivel 3 - Defensa de Roma");

    QPushButton *btnAceptar = new QPushButton(tr("Seleccionar"), &dlg);
    QPushButton *btnCancelar = new QPushButton(tr("Cancelar"), &dlg);

    v->addWidget(list);
    v->addStretch();
    v->addWidget(btnAceptar);
    v->addWidget(btnCancelar);

    // Validación → si no selecciona nivel, no deja avanzar
    QObject::connect(btnAceptar, &QPushButton::clicked, [&]() {
        if (!list->currentItem()) {
            QMessageBox::warning(&dlg, tr("Atención"), tr("Selecciona un nivel primero."));
            return;
        }
        dlg.accept(); // Cierra el diálogo con resultado "ACEPTADO"
    });

    QObject::connect(btnCancelar, &QPushButton::clicked, &dlg, &QDialog::reject);
    QObject::connect(list, &QListWidget::itemActivated, [&](QListWidgetItem*) {
        dlg.accept();
    });

    int result = dlg.exec(); // Muestra el diálogo

    if (result == QDialog::Accepted)
        return list->currentRow() + 1; // Convierte posición (0,1,2 → nivel 1,2,3)

    return 0; // 0 significa cancelar
}


// Abre el nivel correspondiente
void Portada::mostrarNivel(int numero)
{
    player->pause(); // Pausa la música del menú

    // Limpia el nivel anterior si lo había
    if (nivelActivo) {
        nivelActivo->close();
        nivelActivo->deleteLater();
        nivelActivo = nullptr;
    }

    Nivel *nivel = nullptr; // Puntero temporal al nuevo nivel

    // Decide qué nivel crear
    switch (numero) {
    case 1: nivel = new Nivel1(nullptr); break;
    case 2: nivel = new Nivel2(nullptr); break;
    case 3: nivel = new Nivel3(nullptr); break;
    default: return;
    }

    if (nivel) {
        nivelActivo = nivel; // Guarda referencia global al nivel

        // Conexión de señales del nivel
        connect(nivel, &Nivel::nivelCompletado, this, &Portada::avanzarNivel);
        connect(nivel, &Nivel::nivelFallido, this, &Portada::volverAlMenu);

        nivel->setAttribute(Qt::WA_DeleteOnClose); // Elimina nivel al cerrarse

        this->hide();           // Oculta el menú
        nivel->showFullScreen(); // Muestra el nivel en pantalla completa
    }
}


// Lógica para avanzar al siguiente nivel
void Portada::avanzarNivel(int numero)
{
    Nivel *nivelAnterior = qobject_cast<Nivel*>(sender());

    if (numero < 3) {
        if (nivelAnterior) nivelAnterior->close();

        nivelActivo = nullptr;
        nivelActual = numero + 1;
        mostrarNivel(nivelActual);
    } else {
        if (nivelAnterior) nivelAnterior->close();
        nivelActivo = nullptr;

        QMessageBox::information(this, tr("Juego completado"),
                                 tr("¡Has completado todos los niveles!\n\nVolviendo al menú..."));
        volverAlMenu();
    }
}


// Devuelve al menú principal
void Portada::volverAlMenu()
{
    Nivel *nivelAnterior = qobject_cast<Nivel*>(sender());
    if (nivelAnterior) {
        nivelAnterior->close();
    } else if (nivelActivo) {
        nivelActivo->close();
    }

    nivelActivo = nullptr; // Limpia puntero
    showFullScreen();      // Muestra la Portada nuevamente
    player->play();        // Reproduce música del menú
}

