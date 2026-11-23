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
#include <QPushButton>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusReply>
#include <KSharedConfig>
#include <KConfigGroup>
#include <cmath>
#include "liquidbody.h"
#include "liquidrenderer.h"

// ---------------------------------------------------------------------------
// 1. KWin Controller (D-Bus & kwinrc configuration)
// ---------------------------------------------------------------------------
class KWinController {
public:
    static QString activeEffectName() {
        QStringList candidates = {
            QStringLiteral("kwin_liquidmouse_v9"),
            QStringLiteral("kwin_liquidmouse_v8"),
            QStringLiteral("kwin_liquidmouse_v7"),
            QStringLiteral("kwin_liquidmouse_v6"),
            QStringLiteral("kwin_liquidmouse_v5"),
            QStringLiteral("kwin_liquidmouse_v4"),
            QStringLiteral("kwin_liquidmouse_v3"),
            QStringLiteral("kwin_liquidmouse_v2"),
            QStringLiteral("kwin_liquidmouse"),
            QStringLiteral("kwin6_effect_liquidmouse"),
            QStringLiteral("kwin4_effect_liquidmouse"),
            QStringLiteral("liquidmouse")
        };
        for (const auto &cand : candidates) {
            QDBusMessage msg = QDBusMessage::createMethodCall(
                QStringLiteral("org.kde.KWin"),
                QStringLiteral("/Effects"),
                QStringLiteral("org.kde.kwin.Effects"),
                QStringLiteral("isEffectLoaded")
            );
            msg << cand;
            QDBusReply<bool> reply = QDBusConnection::sessionBus().call(msg);
            if (reply.isValid() && reply.value()) {
                return cand;
            }
        }
        return QStringLiteral("kwin_liquidmouse_v2");
    }

    static bool isEffectLoaded() {
        QStringList candidates = {
            QStringLiteral("kwin_liquidmouse_v9"),
            QStringLiteral("kwin_liquidmouse_v8"),
            QStringLiteral("kwin_liquidmouse_v7"),
            QStringLiteral("kwin_liquidmouse_v6"),
            QStringLiteral("kwin_liquidmouse_v5"),
            QStringLiteral("kwin_liquidmouse_v4"),
            QStringLiteral("kwin_liquidmouse_v3"),
            QStringLiteral("kwin_liquidmouse_v2"),
            QStringLiteral("kwin_liquidmouse"),
            QStringLiteral("kwin6_effect_liquidmouse"),
            QStringLiteral("kwin4_effect_liquidmouse"),
            QStringLiteral("liquidmouse")
        };
        for (const auto &cand : candidates) {
            QDBusMessage msg = QDBusMessage::createMethodCall(
                QStringLiteral("org.kde.KWin"),
                QStringLiteral("/Effects"),
                QStringLiteral("org.kde.kwin.Effects"),
                QStringLiteral("isEffectLoaded")
            );
            msg << cand;
            QDBusReply<bool> reply = QDBusConnection::sessionBus().call(msg);
            if (reply.isValid() && reply.value()) {
                return true;
            }
        }

        auto config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
        config->reparseConfiguration();
        return config->group(QStringLiteral("Plugins")).readEntry("kwin_liquidmouse_v9Enabled", false)
               || config->group(QStringLiteral("Plugins")).readEntry("kwin_liquidmouse_v8Enabled", false)
               || config->group(QStringLiteral("Plugins")).readEntry("kwin_liquidmouse_v7Enabled", false)
               || config->group(QStringLiteral("Plugins")).readEntry("kwin_liquidmouse_v6Enabled", false)
               || config->group(QStringLiteral("Plugins")).readEntry("kwin_liquidmouse_v5Enabled", false)
               || config->group(QStringLiteral("Plugins")).readEntry("kwin_liquidmouse_v4Enabled", false)
               || config->group(QStringLiteral("Plugins")).readEntry("kwin_liquidmouse_v3Enabled", false)
               || config->group(QStringLiteral("Plugins")).readEntry("kwin_liquidmouse_v2Enabled", false)
               || config->group(QStringLiteral("Plugins")).readEntry("kwin_liquidmouseEnabled", false)
               || config->group(QStringLiteral("Plugins")).readEntry("kwin6_effect_liquidmouseEnabled", false)
               || config->group(QStringLiteral("Plugins")).readEntry("kwin4_effect_liquidmouseEnabled", false)
               || config->group(QStringLiteral("Plugins")).readEntry("liquidmouseEnabled", false);
    }

    static void setEffectEnabled(bool enabled) {
        auto config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
        auto plugins = config->group(QStringLiteral("Plugins"));
        plugins.writeEntry("kwin_liquidmouse_v9Enabled", enabled);
        plugins.writeEntry("kwin_liquidmouse_v8Enabled", enabled);
        plugins.writeEntry("kwin_liquidmouse_v7Enabled", enabled);
        plugins.writeEntry("kwin_liquidmouse_v6Enabled", enabled);
        plugins.writeEntry("kwin_liquidmouse_v5Enabled", enabled);
        plugins.writeEntry("kwin_liquidmouse_v4Enabled", enabled);
        plugins.writeEntry("kwin_liquidmouse_v3Enabled", enabled);
        plugins.writeEntry("kwin_liquidmouse_v2Enabled", enabled);
        plugins.writeEntry("kwin_liquidmouseEnabled", enabled);
        plugins.writeEntry("kwin6_effect_liquidmouseEnabled", enabled);
        plugins.writeEntry("kwin4_effect_liquidmouseEnabled", enabled);
        plugins.writeEntry("liquidmouseEnabled", enabled);
        plugins.sync();

        QDBusMessage msg = QDBusMessage::createMethodCall(
            QStringLiteral("org.kde.KWin"),
            QStringLiteral("/Effects"),
            QStringLiteral("org.kde.kwin.Effects"),
            enabled ? QStringLiteral("loadEffect") : QStringLiteral("unloadEffect")
        );
        msg << activeEffectName();
        QDBusConnection::sessionBus().send(msg);

        // Notifica KWin di ricaricare le impostazioni
        notifyKWin();
    }

    static void saveSettings(double scale, const LiquidBody::Tuning &tuning) {
        auto config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
        auto group = config->group(QStringLiteral("Effect-LiquidMouse"));

        group.writeEntry("Scale", scale);
        group.writeEntry("DotRadius", tuning.dotRadius);
        group.writeEntry("TrailLength", tuning.trailLength);
        group.writeEntry("TrailResponse", tuning.trailResponse);
        group.writeEntry("Bounce", tuning.bounce);
        group.writeEntry("Splashes", tuning.splashes);
        group.writeEntry("Form", (tuning.form == CursorForm::Arrow) ? 1 : 0);

        int look = 0; // Predefinito: 0 = Light (bianco con bordo nero)
        if (tuning.appearance == BodyLook::Dark) look = 1;
        else if (tuning.appearance == BodyLook::Automatic) look = 2;
        group.writeEntry("Appearance", look);

        group.sync();
        notifyKWin();
    }

    static void loadSettings(double &scale, LiquidBody::Tuning &tuning) {
        auto config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
        config->reparseConfiguration();
        auto group = config->group(QStringLiteral("Effect-LiquidMouse"));

        // Fallback su impostazioni cursore globale di KDE
        double defaultScale = 1.0;
        auto mouseConfig = KSharedConfig::openConfig(QStringLiteral("kcminputrc"));
        int sysCursor = mouseConfig->group(QStringLiteral("Mouse")).readEntry("cursorSize", 24);
        if (sysCursor > 0) {
            defaultScale = static_cast<double>(sysCursor) / 24.0;
        }

        scale = group.readEntry("Scale", defaultScale);
        tuning.dotRadius = group.readEntry("DotRadius", Glyph::designRadius * scale);
        tuning.trailLength = group.readEntry("TrailLength", 160.0 * scale);
        tuning.trailResponse = group.readEntry("TrailResponse", 80.0);
        tuning.bounce = group.readEntry("Bounce", 0.5);
        tuning.splashes = group.readEntry("Splashes", true);

        int form = group.readEntry("Form", 1);
        tuning.form = (form == 0) ? CursorForm::Dot : CursorForm::Arrow;

        int look = group.readEntry("Appearance", 0); // Predefinito 0 = Light (bianco con bordo nero)
        if (look == 1) tuning.appearance = BodyLook::Dark;
        else if (look == 2) tuning.appearance = BodyLook::Automatic;
        else tuning.appearance = BodyLook::Light;
    }

    static void notifyKWin() {
        QStringList effects = {
            QStringLiteral("kwin_liquidmouse_v4"),
            QStringLiteral("kwin_liquidmouse_v3"),
            QStringLiteral("kwin_liquidmouse_v2"),
            QStringLiteral("kwin_liquidmouse"),
            QStringLiteral("kwin6_effect_liquidmouse"),
            QStringLiteral("kwin4_effect_liquidmouse"),
            QStringLiteral("liquidmouse")
        };
        for (const auto &eff : effects) {
            QDBusMessage msg = QDBusMessage::createMethodCall(
                QStringLiteral("org.kde.KWin"),
                QStringLiteral("/Effects"),
                QStringLiteral("org.kde.kwin.Effects"),
                QStringLiteral("reconfigureEffect")
            );
            msg << eff;
            QDBusConnection::sessionBus().send(msg);
        }

        // Notifica generale del compositore KWin
        QDBusMessage msg2 = QDBusMessage::createMethodCall(
            QStringLiteral("org.kde.KWin"),
            QStringLiteral("/KWin"),
            QStringLiteral("org.kde.KWin"),
            QStringLiteral("reconfigure")
        );
        QDBusConnection::sessionBus().send(msg2);
    }

    static bool isAutostart() {
        QString autostartPath = QDir::homePath() + QStringLiteral("/.config/autostart/liquidmouse.desktop");
        return QFile::exists(autostartPath);
    }

    static void setAutostart(bool enable) {
        QString autostartDir = QDir::homePath() + QStringLiteral("/.config/autostart");
        QString autostartFile = autostartDir + QStringLiteral("/liquidmouse.desktop");
        if (enable) {
            QDir().mkpath(autostartDir);
            QFile file(autostartFile);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QString execPath = QCoreApplication::applicationFilePath();
                if (execPath.isEmpty()) {
                    execPath = QStringLiteral("/usr/local/bin/liquidmouse");
                }
                QByteArray content = "[Desktop Entry]\n"
                                     "Type=Application\n"
                                     "Name=LiquidMouse\n"
                                     "GenericName=Cursore Liquido\n"
                                     "Comment=Cursore fluido con fisica liquida per KDE Plasma\n"
                                     "Exec=" + execPath.toUtf8() + "\n"
                                     "Icon=liquidmouse\n"
                                     "Terminal=false\n"
                                     "Categories=Utility;\n"
                                     "StartupNotify=false\n"
                                     "X-KDE-autostart-phase=2\n"
                                     "X-KDE-autostart-after=kwin\n";
                file.write(content);
            }
        } else {
            QFile::remove(autostartFile);
        }
    }
};

// ---------------------------------------------------------------------------
// 2. Icon Generator (renders a crisp gradient droplet arrow)
// ---------------------------------------------------------------------------
static QIcon createLiquidMouseIcon() {
    if (QIcon::hasThemeIcon(QStringLiteral("liquidmouse"))) {
        return QIcon::fromTheme(QStringLiteral("liquidmouse"));
    }

    QStringList searchPaths = {
        QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("icons/hicolor/scalable/apps/liquidmouse.svg")),
        QDir::homePath() + QStringLiteral("/.local/share/icons/hicolor/scalable/apps/liquidmouse.svg"),
        QStringLiteral("/usr/share/icons/hicolor/scalable/apps/liquidmouse.svg"),
        QStringLiteral("resources/liquidmouse.svg"),
        QStringLiteral("../resources/liquidmouse.svg")
    };
    for (const auto &p : searchPaths) {
        if (!p.isEmpty() && QFile::exists(p)) {
            return QIcon(p);
        }
    }

    // Fallback: pixmap ad alta risoluzione
    QPixmap pix(128, 128);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient grad(10, 10, 110, 110);
    grad.setColorAt(0.0, QColor(0, 210, 255));
    grad.setColorAt(1.0, QColor(58, 123, 213));

    QPainterPath arrow;
    arrow.moveTo(24, 16);
    arrow.lineTo(76, 68);
    arrow.lineTo(54, 72);
    arrow.lineTo(78, 110);
    arrow.lineTo(60, 118);
    arrow.lineTo(36, 80);
    arrow.lineTo(16, 96);
    arrow.closeSubpath();

    p.setPen(QPen(QColor(0, 0, 0, 90), 8.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(arrow.translated(0, 4));

    p.setPen(QPen(Qt::white, 6.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(grad);
    p.drawPath(arrow);

    p.setPen(QPen(Qt::white, 3.0));
    p.setBrush(QColor(0, 210, 255));
    p.drawEllipse(QPointF(96, 92), 12, 12);
    p.drawEllipse(QPointF(112, 68), 7, 7);
    p.drawEllipse(QPointF(84, 116), 5, 5);

    p.end();
    return QIcon::fromTheme(QStringLiteral("liquidmouse"), QIcon(pix));
}

// ---------------------------------------------------------------------------
// 3. Canvas Widget for Interactive Physics Preview
// ---------------------------------------------------------------------------
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
        if (event->button() == Qt::LeftButton) m_pressed = true;
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) m_pressed = false;
    }
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Tela divisa a metà: sinistra bianco puro, destra scuro per verificare il bordino di contrasto
        QRect r = rect();
        int halfW = r.width() / 2;
        QRect leftRect(0, 0, halfW, r.height());
        QRect rightRect(halfW, 0, r.width() - halfW, r.height());

        painter.fillRect(leftRect, QColor(255, 255, 255)); // Bianco puro
        painter.fillRect(rightRect, QColor(28, 30, 34));   // Scuro

        painter.setPen(QColor(130, 130, 130));
        QFont font = painter.font();
        font.setPointSize(9);
        painter.setFont(font);
        painter.drawText(leftRect.adjusted(14, 14, -14, -14), Qt::AlignTop | Qt::AlignLeft,
                         QStringLiteral("Sfondo Bianco\n(prova il contrasto del bordino)"));
        painter.drawText(rightRect.adjusted(14, 14, -14, -14), Qt::AlignTop | Qt::AlignRight,
                         QStringLiteral("Sfondo Scuro\n(prova il rimbalzo e la scia)"));

        // Linea divisoria tratteggiata
        painter.setPen(QPen(QColor(160, 160, 160, 100), 1.0, Qt::DashLine));
        painter.drawLine(halfW, 0, halfW, r.height());

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
    QPointF m_mousePos = QPointF(150.0, 150.0);
    bool m_pressed = false;
    bool m_systemIsDark = true;
    QElapsedTimer m_timer;
    QTimer m_animTimer;
};

// ---------------------------------------------------------------------------
// 4. Settings Window with Live Sliders & Real-Time Sync
// ---------------------------------------------------------------------------
class SettingsDialog : public QMainWindow {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle(QStringLiteral("LiquidMouse - Impostazioni & Controllo"));
        setWindowIcon(createLiquidMouseIcon());
        resize(940, 680);

        // Timer di debounce per evitare spam D-Bus durante il trascinamento dello slider
        m_debounceTimer.setSingleShot(true);
        m_debounceTimer.setInterval(40);
        connect(&m_debounceTimer, &QTimer::timeout, this, [this]() {
            KWinController::saveSettings(m_scale, m_tuning);
        });

        // Carica impostazioni attuali
        KWinController::loadSettings(m_scale, m_tuning);

        auto *central = new QWidget(this);
        auto *mainLayout = new QHBoxLayout(central);

        // Colonna Sinistra: Canvas di anteprima e pulsanti di test forme
        auto *leftCol = new QWidget(central);
        auto *leftLayout = new QVBoxLayout(leftCol);
        leftLayout->setContentsMargins(0, 0, 0, 0);

        m_canvas = new CanvasWidget(leftCol);
        m_canvas->body().tuning = m_tuning;
        leftLayout->addWidget(m_canvas, 1);

        // Barra pulsanti per testare forme di ridimensionamento
        auto *shapeTestGroup = new QGroupBox(QStringLiteral("Simula forma cursore nell'area di prova:"), leftCol);
        auto *stLayout = new QHBoxLayout(shapeTestGroup);

        auto addTestBtn = [this, stLayout, shapeTestGroup](const QString &text, const CursorContext &ctx) {
            auto *b = new QPushButton(text, shapeTestGroup);
            connect(b, &QPushButton::clicked, this, [this, ctx]() {
                m_canvas->setContext(ctx);
            });
            stLayout->addWidget(b);
        };

        double invSqrt2 = 1.0 / std::sqrt(2.0);
        addTestBtn(QStringLiteral("Normale"), CursorContext::pointer());
        addTestBtn(QStringLiteral("👆 Link"), CursorContext::link());
        addTestBtn(QStringLiteral("↔ Orizzontale"), CursorContext::resize(QPointF(1.0, 0.0), true, false));
        addTestBtn(QStringLiteral("↕ Verticale"), CursorContext::resize(QPointF(0.0, 1.0), true, false));
        addTestBtn(QStringLiteral("↖↘ Diag NW-SE"), CursorContext::resize(QPointF(invSqrt2, invSqrt2), true, false));
        addTestBtn(QStringLiteral("↗↙ Diag NE-SW"), CursorContext::resize(QPointF(invSqrt2, -invSqrt2), true, false));
        addTestBtn(QStringLiteral("I-Beam Testo"), CursorContext::text(9.0));

        leftLayout->addWidget(shapeTestGroup);
        mainLayout->addWidget(leftCol, 1);

        // Pannello comandi a destra
        auto *sidebar = new QWidget(central);
        sidebar->setFixedWidth(360);
        auto *sideLayout = new QVBoxLayout(sidebar);

        // Switch Attivazione Principale
        auto *mainGroup = new QGroupBox(QStringLiteral("Stato Generale"), sidebar);
        auto *mgLayout = new QVBoxLayout(mainGroup);
        m_enableCheck = new QCheckBox(QStringLiteral("Abilita LiquidMouse a livello globale"), mainGroup);
        m_enableCheck->setChecked(KWinController::isEffectLoaded());
        mgLayout->addWidget(m_enableCheck);
        sideLayout->addWidget(mainGroup);

        connect(m_enableCheck, &QCheckBox::toggled, this, [this](bool checked) {
            KWinController::setEffectEnabled(checked);
            Q_EMIT effectStateChanged(checked);
        });

        // Aspetto & Bordo di Contrasto
        auto *lookGroup = new QGroupBox(QStringLiteral("Aspetto & Bordino di Contrasto"), sidebar);
        auto *lLayout = new QVBoxLayout(lookGroup);
        m_lightRadio = new QRadioButton(QStringLiteral("Bianco con bordo nero (Consigliato, visibile ovunque)"), lookGroup);
        m_darkRadio = new QRadioButton(QStringLiteral("Nero con bordo bianco"), lookGroup);
        m_autoRadio = new QRadioButton(QStringLiteral("Automatico (in base al tema desktop)"), lookGroup);

        if (m_tuning.appearance == BodyLook::Dark) m_darkRadio->setChecked(true);
        else if (m_tuning.appearance == BodyLook::Automatic) m_autoRadio->setChecked(true);
        else m_lightRadio->setChecked(true);

        lLayout->addWidget(m_lightRadio);
        lLayout->addWidget(m_darkRadio);
        lLayout->addWidget(m_autoRadio);
        sideLayout->addWidget(lookGroup);

        connect(m_lightRadio, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                setAppearance(BodyLook::Light);
            }
        });
        connect(m_darkRadio, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                setAppearance(BodyLook::Dark);
            }
        });
        connect(m_autoRadio, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                setAppearance(BodyLook::Automatic);
            }
        });

        // Controllo Grandezza Cursore (Scale)
        auto *sizeGroup = new QGroupBox(QStringLiteral("Dimensioni Cursore (Grandezza)"), sidebar);
        auto *szLayout = new QVBoxLayout(sizeGroup);

        m_sizeLabel = new QLabel(QStringLiteral("Grandezza: %1% (Scala %2x)")
                                     .arg(std::round(m_scale * 100.0))
                                     .arg(m_scale, 0, 'f', 2), sizeGroup);
        QFont f = m_sizeLabel->font();
        f.setBold(true);
        m_sizeLabel->setFont(f);
        szLayout->addWidget(m_sizeLabel);

        m_sizeSlider = new QSlider(Qt::Horizontal, sizeGroup);
        m_sizeSlider->setRange(50, 300); // 50% fino a 300%
        m_sizeSlider->setValue(static_cast<int>(std::round(m_scale * 100.0)));
        szLayout->addWidget(m_sizeSlider);

        // Pulsanti rapidi percentuali
        auto *presetLayout = new QHBoxLayout();
        auto addQuickBtn = [this, presetLayout, sizeGroup](const QString &text, double s) {
            auto *b = new QPushButton(text, sizeGroup);
            connect(b, &QPushButton::clicked, this, [this, s]() {
                setScaleValue(s);
            });
            presetLayout->addWidget(b);
        };
        addQuickBtn(QStringLiteral("100%"), 1.0);
        addQuickBtn(QStringLiteral("125%"), 1.25);
        addQuickBtn(QStringLiteral("150%"), 1.5);
        addQuickBtn(QStringLiteral("200%"), 2.0);
        szLayout->addLayout(presetLayout);

        sideLayout->addWidget(sizeGroup);

        connect(m_sizeSlider, &QSlider::valueChanged, this, [this](int val) {
            m_scale = val / 100.0;
            m_sizeLabel->setText(QStringLiteral("Grandezza: %1% (Scala %2x)").arg(val).arg(m_scale, 0, 'f', 2));
            m_tuning.dotRadius = Glyph::designRadius * m_scale;
            m_tuning.trailLength = 160.0 * m_scale;
            m_canvas->body().tuning.dotRadius = m_tuning.dotRadius;
            m_canvas->body().tuning.trailLength = m_tuning.trailLength;
            applySettings();
        });
        connect(m_sizeSlider, &QSlider::sliderReleased, this, &SettingsDialog::applySettingsImmediately);

        // Controlli Fisica (Scia, Reattività, Rimbalzo)
        auto *physGroup = new QGroupBox(QStringLiteral("Fisica Fluida & Scia"), sidebar);
        auto *pLayout = new QVBoxLayout(physGroup);

        auto addPhysSlider = [pLayout, sidebar, this](const QString &title, int min, int max, int cur, auto callback) {
            auto *lbl = new QLabel(QStringLiteral("%1: %2").arg(title).arg(cur), sidebar);
            auto *sld = new QSlider(Qt::Horizontal, sidebar);
            sld->setRange(min, max);
            sld->setValue(cur);
            pLayout->addWidget(lbl);
            pLayout->addWidget(sld);
            connect(sld, &QSlider::valueChanged, [lbl, title, callback, this](int v) {
                lbl->setText(QStringLiteral("%1: %2").arg(title).arg(v));
                callback(v);
                applySettings();
            });
            connect(sld, &QSlider::sliderReleased, this, &SettingsDialog::applySettingsImmediately);
        };

        addPhysSlider(QStringLiteral("Lunghezza scia"), 0, 400, static_cast<int>(m_tuning.trailLength), [this](int v) {
            m_tuning.trailLength = v;
            m_canvas->body().tuning.trailLength = v;
        });

        addPhysSlider(QStringLiteral("Reattività scia"), 20, 150, static_cast<int>(m_tuning.trailResponse), [this](int v) {
            m_tuning.trailResponse = v;
            m_canvas->body().tuning.trailResponse = v;
        });

        addPhysSlider(QStringLiteral("Elasticità / Rimbalzo (%)"), 0, 100, static_cast<int>(m_tuning.bounce * 100.0), [this](int v) {
            m_tuning.bounce = v / 100.0;
            m_canvas->body().tuning.bounce = m_tuning.bounce;
        });

        m_splashCheck = new QCheckBox(QStringLiteral("Schizzo gocce al clic (Splash)"), physGroup);
        m_splashCheck->setChecked(m_tuning.splashes);
        pLayout->addWidget(m_splashCheck);
        connect(m_splashCheck, &QCheckBox::toggled, this, [this](bool checked) {
            m_tuning.splashes = checked;
            m_canvas->body().tuning.splashes = checked;
            applySettingsImmediately();
        });

        sideLayout->addWidget(physGroup);

        // Selezione Forma Base (Freccia / Punto)
        auto *formGroup = new QGroupBox(QStringLiteral("Forma Base del Puntatore"), sidebar);
        auto *fLayout = new QHBoxLayout(formGroup);
        auto *arrowRadio = new QRadioButton(QStringLiteral("Freccia"), formGroup);
        auto *dotRadio = new QRadioButton(QStringLiteral("Punto"), formGroup);
        if (m_tuning.form == CursorForm::Arrow) arrowRadio->setChecked(true);
        else dotRadio->setChecked(true);
        fLayout->addWidget(arrowRadio);
        fLayout->addWidget(dotRadio);
        sideLayout->addWidget(formGroup);

        connect(arrowRadio, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                m_tuning.form = CursorForm::Arrow;
                m_canvas->body().tuning.form = CursorForm::Arrow;
                applySettingsImmediately();
            }
        });
        connect(dotRadio, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                m_tuning.form = CursorForm::Dot;
                m_canvas->body().tuning.form = CursorForm::Dot;
                applySettingsImmediately();
            }
        });

        // Pulsanti fondo finestra
        sideLayout->addStretch();
        auto *btnLayout = new QHBoxLayout();
        auto *resetBtn = new QPushButton(QStringLiteral("Predefiniti"), sidebar);
        auto *hideBtn = new QPushButton(QStringLiteral("Chiudi / Riduci a icona"), sidebar);
        hideBtn->setDefault(true);
        btnLayout->addWidget(resetBtn);
        btnLayout->addWidget(hideBtn);
        sideLayout->addLayout(btnLayout);

        connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::resetDefaults);
        connect(hideBtn, &QPushButton::clicked, this, &QWidget::hide);

        mainLayout->addWidget(sidebar);
        setCentralWidget(central);
    }

    double scale() const { return m_scale; }
    BodyLook appearance() const { return m_tuning.appearance; }

    void setAppearance(BodyLook look) {
        m_tuning.appearance = look;
        m_canvas->body().tuning.appearance = look;
        m_canvas->update();
        if (look == BodyLook::Dark && m_darkRadio && !m_darkRadio->isChecked()) m_darkRadio->setChecked(true);
        else if (look == BodyLook::Automatic && m_autoRadio && !m_autoRadio->isChecked()) m_autoRadio->setChecked(true);
        else if (look == BodyLook::Light && m_lightRadio && !m_lightRadio->isChecked()) m_lightRadio->setChecked(true);
        applySettingsImmediately();
    }

    void setScaleValue(double scale) {
        m_scale = scale;
        int sliderVal = static_cast<int>(std::round(scale * 100.0));
        m_sizeSlider->setValue(sliderVal);
        m_sizeLabel->setText(QStringLiteral("Grandezza: %1% (Scala %2x)").arg(sliderVal).arg(m_scale, 0, 'f', 2));
        m_tuning.dotRadius = Glyph::designRadius * m_scale;
        m_tuning.trailLength = 160.0 * m_scale;
        m_canvas->body().tuning.dotRadius = m_tuning.dotRadius;
        m_canvas->body().tuning.trailLength = m_tuning.trailLength;
        applySettingsImmediately();
    }

    void refreshStatus() {
        m_enableCheck->setChecked(KWinController::isEffectLoaded());
        KWinController::loadSettings(m_scale, m_tuning);
        m_sizeSlider->blockSignals(true);
        m_sizeSlider->setValue(static_cast<int>(std::round(m_scale * 100.0)));
        m_sizeSlider->blockSignals(false);
        m_sizeLabel->setText(QStringLiteral("Grandezza: %1% (Scala %2x)")
                                 .arg(std::round(m_scale * 100.0))
                                 .arg(m_scale, 0, 'f', 2));
        m_canvas->body().tuning = m_tuning;
        if (m_tuning.appearance == BodyLook::Dark && m_darkRadio) m_darkRadio->setChecked(true);
        else if (m_tuning.appearance == BodyLook::Automatic && m_autoRadio) m_autoRadio->setChecked(true);
        else if (m_lightRadio) m_lightRadio->setChecked(true);
    }

    void applySettings() {
        if (!m_debounceTimer.isActive()) {
            m_debounceTimer.start();
        }
    }

    void applySettingsImmediately() {
        m_debounceTimer.stop();
        KWinController::saveSettings(m_scale, m_tuning);
    }

Q_SIGNALS:
    void effectStateChanged(bool active);

protected:
    void closeEvent(QCloseEvent *event) override {
        // Non terminare l'applicazione alla chiusura della finestra: minimizza nella tray
        event->ignore();
        hide();
    }

private:
    void resetDefaults() {
        m_scale = 1.0;
        m_tuning.dotRadius = Glyph::designRadius;
        m_tuning.trailLength = 160.0;
        m_tuning.trailResponse = 80.0;
        m_tuning.bounce = 0.5;
        m_tuning.splashes = true;
        m_tuning.form = CursorForm::Arrow;
        m_tuning.appearance = BodyLook::Light;

        m_sizeSlider->setValue(100);
        m_canvas->body().tuning = m_tuning;
        if (m_lightRadio) m_lightRadio->setChecked(true);
        applySettingsImmediately();
    }

    CanvasWidget *m_canvas = nullptr;
    QCheckBox *m_enableCheck = nullptr;
    QLabel *m_sizeLabel = nullptr;
    QSlider *m_sizeSlider = nullptr;
    QCheckBox *m_splashCheck = nullptr;
    QRadioButton *m_lightRadio = nullptr;
    QRadioButton *m_darkRadio = nullptr;
    QRadioButton *m_autoRadio = nullptr;
    QTimer m_debounceTimer;
    double m_scale = 1.0;
    LiquidBody::Tuning m_tuning;
};

// ---------------------------------------------------------------------------
// 5. System Tray Application Wrapper
// ---------------------------------------------------------------------------
class LiquidMouseTrayApp : public QObject {
    Q_OBJECT
public:
    LiquidMouseTrayApp() {
        m_dialog = new SettingsDialog();

        createTrayIcon();

        // Collega eventi dello stato
        connect(m_dialog, &SettingsDialog::effectStateChanged, this, [this](bool active) {
            updateTrayStatus(active);
        });

        updateTrayStatus(KWinController::isEffectLoaded());
        m_trayIcon->show();
    }

    void showSettings() {
        m_dialog->refreshStatus();
        m_dialog->show();
        m_dialog->raise();
        m_dialog->activateWindow();
    }

private:
    void createTrayIcon() {
        m_trayIcon = new QSystemTrayIcon(createLiquidMouseIcon(), this);
        m_trayIcon->setToolTip(QStringLiteral("LiquidMouse - Cursore Fluido"));

        auto *menu = new QMenu();

        // Titolo / Intestazione
        m_titleAction = menu->addAction(QStringLiteral("LiquidMouse"));
        QFont f = m_titleAction->font();
        f.setBold(true);
        m_titleAction->setFont(f);
        m_titleAction->setEnabled(false);

        // Toggle attivo / inattivo
        m_toggleAction = menu->addAction(QStringLiteral("Attiva Cursore Liquido"));
        m_toggleAction->setCheckable(true);
        connect(m_toggleAction, &QAction::toggled, this, [](bool checked) {
            KWinController::setEffectEnabled(checked);
        });

        menu->addSeparator();

        // Sottomenu Grandezza Cursore Rapida
        auto *sizeMenu = menu->addMenu(QStringLiteral("Grandezza Cursore"));
        auto *sizeGroup = new QActionGroup(this);

        struct Preset { const char *label; double scale; };
        Preset presets[] = {
            {"Normale (100%)", 1.0},
            {"Media (125%)", 1.25},
            {"Grande (150%)", 1.5},
            {"Molto grande (175%)", 1.75},
            {"Extra large (200%)", 2.0}
        };

        for (const auto &p : presets) {
            auto *act = sizeMenu->addAction(QString::fromUtf8(p.label));
            act->setCheckable(true);
            sizeGroup->addAction(act);
            double sc = p.scale;
            if (std::abs(m_dialog->scale() - sc) < 0.05) {
                act->setChecked(true);
            }
            connect(act, &QAction::triggered, this, [this, sc]() {
                m_dialog->setScaleValue(sc);
            });
        }

        // Sottomenu Aspetto & Bordino di contrasto
        auto *themeMenu = menu->addMenu(QStringLiteral("Aspetto & Bordino"));
        auto *themeGroup = new QActionGroup(this);

        auto *lightAct = themeMenu->addAction(QStringLiteral("Bianco con bordo nero"));
        lightAct->setCheckable(true);
        themeGroup->addAction(lightAct);
        if (m_dialog->appearance() == BodyLook::Light) lightAct->setChecked(true);
        connect(lightAct, &QAction::triggered, this, [this]() {
            m_dialog->setAppearance(BodyLook::Light);
        });

        auto *darkAct = themeMenu->addAction(QStringLiteral("Nero con bordo bianco"));
        darkAct->setCheckable(true);
        themeGroup->addAction(darkAct);
        if (m_dialog->appearance() == BodyLook::Dark) darkAct->setChecked(true);
        connect(darkAct, &QAction::triggered, this, [this]() {
            m_dialog->setAppearance(BodyLook::Dark);
        });

        auto *autoAct = themeMenu->addAction(QStringLiteral("Automatico (tema desktop)"));
        autoAct->setCheckable(true);
        themeGroup->addAction(autoAct);
        if (m_dialog->appearance() == BodyLook::Automatic) autoAct->setChecked(true);
        connect(autoAct, &QAction::triggered, this, [this]() {
            m_dialog->setAppearance(BodyLook::Automatic);
        });

        menu->addSeparator();

        // Finestra Impostazioni
        auto *settingsAct = menu->addAction(QStringLiteral("Impostazioni & Anteprima..."));
        connect(settingsAct, &QAction::triggered, this, &LiquidMouseTrayApp::showSettings);

        // Autostart
        menu->addSeparator();
        auto *autostartAct = menu->addAction(QStringLiteral("Avvia automaticamente all'accesso"));
        autostartAct->setCheckable(true);
        autostartAct->setChecked(KWinController::isAutostart());
        connect(autostartAct, &QAction::toggled, [](bool checked) {
            KWinController::setAutostart(checked);
        });

        menu->addSeparator();

        // Esci
        auto *quitAct = menu->addAction(QStringLiteral("Esci da LiquidMouse"));
        connect(quitAct, &QAction::triggered, qApp, &QCoreApplication::quit);

        m_trayIcon->setContextMenu(menu);

        // Click sinistro o doppio click sull'icona della tray apre/chiude la finestra impostazioni
        connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                if (m_dialog->isVisible()) {
                    m_dialog->hide();
                } else {
                    showSettings();
                }
            }
        });
    }

    void updateTrayStatus(bool active) {
        m_toggleAction->setChecked(active);
        m_trayIcon->setToolTip(active ? QStringLiteral("LiquidMouse: Attivo") : QStringLiteral("LiquidMouse: Disattivato"));
    }

    QSystemTrayIcon *m_trayIcon = nullptr;
    SettingsDialog *m_dialog = nullptr;
    QAction *m_titleAction = nullptr;
    QAction *m_toggleAction = nullptr;
};

// ---------------------------------------------------------------------------
// 6. DBus Adaptor for Single Instance & Remote Control
// ---------------------------------------------------------------------------
class DBusAdaptor : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.LiquidMouse")
public:
    explicit DBusAdaptor(LiquidMouseTrayApp *tray, QObject *parent = nullptr)
        : QObject(parent), m_tray(tray) {}

public Q_SLOTS:
    void showSettings() {
        if (m_tray) {
            m_tray->showSettings();
        }
    }

private:
    LiquidMouseTrayApp *m_tray;
};

// ---------------------------------------------------------------------------
// 7. Main Entry Point
// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("liquidmouse"));
    app.setApplicationDisplayName(QStringLiteral("LiquidMouse"));
    app.setWindowIcon(createLiquidMouseIcon());

    // Non terminare il processo quando la finestra impostazioni viene chiusa
    app.setQuitOnLastWindowClosed(false);

    const QString serviceName = QStringLiteral("org.kde.LiquidMouse");
    auto bus = QDBusConnection::sessionBus();

    // Supporto istanza singola: se già in esecuzione, risveglia la finestra e termina
    if (!bus.registerService(serviceName)) {
        QDBusMessage msg = QDBusMessage::createMethodCall(
            serviceName,
            QStringLiteral("/App"),
            serviceName,
            QStringLiteral("showSettings")
        );
        bus.send(msg);
        return 0;
    }

    LiquidMouseTrayApp trayApp;
    bus.registerObject(QStringLiteral("/App"), new DBusAdaptor(&trayApp, &app), QDBusConnection::ExportAllSlots);

    // Se invocato con --settings o aperto da menu applicazioni, mostra subito la finestra
    bool openSettings = false;
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == QStringLiteral("--settings") || arg == QStringLiteral("--preview") || arg == QStringLiteral("-s")) {
            openSettings = true;
            break;
        }
    }

    if (openSettings) {
        trayApp.showSettings();
    }

    return app.exec();
}

#include "tray_app.moc"
