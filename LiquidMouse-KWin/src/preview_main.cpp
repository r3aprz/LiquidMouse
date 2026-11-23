#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <QElapsedTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QGroupBox>
#include "liquidbody.h"
#include "liquidrenderer.h"

class CanvasWidget : public QWidget {
    Q_OBJECT
public:
    explicit CanvasWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMouseTracking(true);
        m_timer.start();
        m_body = LiquidBody(QPointF(200.0, 200.0));

        connect(&m_animTimer, &QTimer::timeout, this, [this]() {
            double dt = m_timer.restart() / 1000.0;
            if (dt <= 0.0 || dt > 0.1) dt = 0.016;
            m_body.step(m_mousePos, dt, m_context, m_pressed, true);
            update();
        });
        m_animTimer.start(16); // ~60 FPS
    }

    LiquidBody& body() { return m_body; }
    void setContext(const CursorContext &ctx) { m_context = ctx; }
    void setSystemIsDark(bool dark) { m_systemIsDark = dark; }

protected:
    void mouseMoveEvent(QMouseEvent *event) override {
        m_mousePos = event->position();
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            m_pressed = true;
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            m_pressed = false;
        }
    }

    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Background
        QColor bg = m_systemIsDark ? QColor(32, 33, 36) : QColor(245, 246, 248);
        painter.fillRect(rect(), bg);

        // Draw helper instructions
        painter.setPen(m_systemIsDark ? QColor(140, 140, 140) : QColor(120, 120, 120));
        QFont font = painter.font();
        font.setPointSize(11);
        painter.setFont(font);
        painter.drawText(rect().adjusted(20, 20, -20, -20), Qt::AlignTop | Qt::AlignLeft,
                         QStringLiteral("Muovi il cursore all'interno dell'area per vedere la fisica liquida.\n"
                                        "Fai clic con il tasto sinistro per generare lo splash di gocce!"));

        // Render the LiquidMouse body
        LiquidBody::Shape shape = m_body.shape();
        LiquidRenderer::Colors colors = LiquidRenderer::resolveColors(m_body.tuning.appearance, m_systemIsDark);

        painter.save();
        painter.translate(m_mousePos);
        LiquidRenderer::render(painter, shape, colors);
        painter.restore();
    }

private:
    LiquidBody m_body;
    CursorContext m_context = CursorContext::pointer();
    QPointF m_mousePos = QPointF(250.0, 250.0);
    bool m_pressed = false;
    bool m_systemIsDark = false;
    QElapsedTimer m_timer;
    QTimer m_animTimer;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow() {
        setWindowTitle(QStringLiteral("LiquidMouse - Linux / KDE Preview & Test Harness"));
        resize(950, 650);

        auto *central = new QWidget(this);
        auto *mainLayout = new QHBoxLayout(central);

        auto *canvas = new CanvasWidget(this);
        mainLayout->addWidget(canvas, 1);

        // Sidebar controls
        auto *sidebar = new QWidget(this);
        sidebar->setFixedWidth(300);
        auto *sideLayout = new QVBoxLayout(sidebar);

        // Form selector
        auto *formGroup = new QGroupBox(QStringLiteral("Forma cursore"), sidebar);
        auto *formLayout = new QVBoxLayout(formGroup);
        auto *arrowRadio = new QRadioButton(QStringLiteral("Freccia (Arrow)"), formGroup);
        auto *dotRadio = new QRadioButton(QStringLiteral("Punto (Dot)"), formGroup);
        arrowRadio->setChecked(true);
        formLayout->addWidget(arrowRadio);
        formLayout->addWidget(dotRadio);
        sideLayout->addWidget(formGroup);

        connect(arrowRadio, &QRadioButton::toggled, [canvas](bool checked) {
            if (checked) canvas->body().tuning.form = CursorForm::Arrow;
        });
        connect(dotRadio, &QRadioButton::toggled, [canvas](bool checked) {
            if (checked) canvas->body().tuning.form = CursorForm::Dot;
        });

        // Context selector
        auto *contextGroup = new QGroupBox(QStringLiteral("Simula forma contestuale"), sidebar);
        auto *ctxLayout = new QVBoxLayout(contextGroup);
        auto *ctxCombo = new QComboBox(contextGroup);
        ctxCombo->addItem(QStringLiteral("Normale (Pointer)"), 0);
        ctxCombo->addItem(QStringLiteral("Link / Cliccabile"), 1);
        ctxCombo->addItem(QStringLiteral("Testo / I-Beam"), 2);
        ctxCombo->addItem(QStringLiteral("Testo Verticale"), 3);
        ctxCombo->addItem(QStringLiteral("Afferra (Grab)"), 4);
        ctxCombo->addItem(QStringLiteral("Afferrato (Grabbing)"), 5);
        ctxCombo->addItem(QStringLiteral("Ridimensiona Orizzontale (↔)"), 6);
        ctxCombo->addItem(QStringLiteral("Ridimensiona Verticale (↕)"), 7);
        ctxCombo->addItem(QStringLiteral("Ridimensiona Diagonale NW-SE (↖↘)"), 8);
        ctxCombo->addItem(QStringLiteral("Ridimensiona Diagonale NE-SW (↗↙)"), 9);
        ctxCombo->addItem(QStringLiteral("Mirino (Crosshair)"), 10);
        ctxCombo->addItem(QStringLiteral("Vietato (Forbidden)"), 11);
        ctxLayout->addWidget(ctxCombo);
        sideLayout->addWidget(contextGroup);

        connect(ctxCombo, &QComboBox::currentIndexChanged, [canvas](int idx) {
            double invSqrt2 = 1.0 / std::sqrt(2.0);
            switch (idx) {
            case 0: canvas->setContext(CursorContext::pointer()); break;
            case 1: canvas->setContext(CursorContext::link()); break;
            case 2: canvas->setContext(CursorContext::text(9.0)); break;
            case 3: canvas->setContext(CursorContext::verticalText()); break;
            case 4: canvas->setContext(CursorContext::grab()); break;
            case 5: canvas->setContext(CursorContext::grabbing()); break;
            case 6: canvas->setContext(CursorContext::resize(QPointF(1.0, 0.0), true, false)); break;
            case 7: canvas->setContext(CursorContext::resize(QPointF(0.0, 1.0), true, false)); break;
            case 8: canvas->setContext(CursorContext::resize(QPointF(invSqrt2, invSqrt2), true, false)); break;
            case 9: canvas->setContext(CursorContext::resize(QPointF(invSqrt2, -invSqrt2), true, false)); break;
            case 10: canvas->setContext(CursorContext::crosshair()); break;
            case 11: canvas->setContext(CursorContext::notAllowed()); break;
            }
        });

        // Sliders
        auto addSlider = [sidebar, sideLayout](const QString &label, int min, int max, int val, auto callback) {
            auto *lbl = new QLabel(QStringLiteral("%1: %2").arg(label).arg(val), sidebar);
            auto *slider = new QSlider(Qt::Horizontal, sidebar);
            slider->setRange(min, max);
            slider->setValue(val);
            sideLayout->addWidget(lbl);
            sideLayout->addWidget(slider);
            QObject::connect(slider, &QSlider::valueChanged, [lbl, label, callback](int v) {
                lbl->setText(QStringLiteral("%1: %2").arg(label).arg(v));
                callback(v);
            });
        };

        addSlider(QStringLiteral("Lunghezza scia (Trail)"), 0, 300, 160, [canvas](int v) {
            canvas->body().tuning.trailLength = v;
        });

        addSlider(QStringLiteral("Reattività scia (Speed)"), 20, 150, 80, [canvas](int v) {
            canvas->body().tuning.trailResponse = v;
        });

        addSlider(QStringLiteral("Elasticità / Rimbalzo (%)"), 0, 100, 50, [canvas](int v) {
            canvas->body().tuning.bounce = v / 100.0;
        });

        addSlider(QStringLiteral("Raggio base punto"), 3, 16, 6, [canvas](int v) {
            canvas->body().tuning.dotRadius = v;
        });

        // Splashes toggle
        auto *splashCheck = new QCheckBox(QStringLiteral("Schizzo gocce al clic (Splash)"), sidebar);
        splashCheck->setChecked(true);
        sideLayout->addWidget(splashCheck);
        connect(splashCheck, &QCheckBox::toggled, [canvas](bool checked) {
            canvas->body().tuning.splashes = checked;
        });

        // Dark mode toggle
        auto *darkCheck = new QCheckBox(QStringLiteral("Sfondo scuro (Dark Theme)"), sidebar);
        darkCheck->setChecked(false);
        sideLayout->addWidget(darkCheck);
        connect(darkCheck, &QCheckBox::toggled, [canvas](bool checked) {
            canvas->setSystemIsDark(checked);
        });

        sideLayout->addStretch();
        mainLayout->addWidget(sidebar);

        setCentralWidget(central);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow win;
    win.show();
    return app.exec();
}

#include "preview_main.moc"
