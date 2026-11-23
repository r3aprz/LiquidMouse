#include "liquidmouse_effect.h"
#include <kwin/effect/effecthandler.h>
#include <kwin/cursor.h>
#include <kwin/cursorsource.h>
#include <kwin/core/rendertarget.h>
#include <kwin/core/renderviewport.h>
#include <kwin/opengl/gltexture.h>
#include <kwin/opengl/glshader.h>
#include <kwin/opengl/glshadermanager.h>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QPainter>
#include <QDBusConnection>
#include <QDBusMessage>
#include <cmath>
#include <GL/gl.h>

namespace KWin {

LiquidMouseEffect::LiquidMouseEffect() {
    m_body = LiquidBody(effects->cursorPos());
    m_lastPos = effects->cursorPos();
    m_frameTimer.start();

    // Mouse movement and cursor changes
    connect(effects, &EffectsHandler::mouseChanged, this, &LiquidMouseEffect::slotMouseChanged);
    connect(effects, &EffectsHandler::cursorShapeChanged, this, &LiquidMouseEffect::slotCursorShapeChanged);
    if (Cursors::self() && Cursors::self()->mouse()) {
        connect(Cursors::self()->mouse(), &Cursor::cursorChanged, this, &LiquidMouseEffect::slotCursorShapeChanged);
    }

    // Hide system hardware cursor while effect is active
    if (Cursors::self()) {
        Cursors::self()->hideCursor();
        m_cursorHidden = true;
    }

    // Disabilita shakecursor di KWin per evitare che il cursore statico di sistema appaia sotto al nostro
    if (effects) {
        QDBusMessage msg = QDBusMessage::createMethodCall(
            QStringLiteral("org.kde.KWin"),
            QStringLiteral("/Effects"),
            QStringLiteral("org.kde.kwin.Effects"),
            QStringLiteral("unloadEffect")
        );
        msg << QStringLiteral("shakecursor");
        QDBusConnection::sessionBus().send(msg);
    }

    // High frequency timer to advance physics when mouse is still but springs/splash are moving
    connect(&m_animTimer, &QTimer::timeout, this, &LiquidMouseEffect::onTick);
    m_animTimer.setInterval(8); // ~120 Hz update
    m_animTimer.start();

    reconfigure(ReconfigureAll);
}

LiquidMouseEffect::~LiquidMouseEffect() {
    if (m_cursorHidden && Cursors::self()) {
        Cursors::self()->showCursor();
        m_cursorHidden = false;
    }
}

bool LiquidMouseEffect::supported() {
    return !effects || effects->isOpenGLCompositing();
}

bool LiquidMouseEffect::isActive() const {
    return m_enabled;
}

void LiquidMouseEffect::reconfigure(ReconfigureFlags) {
    auto kwinConfig = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
    kwinConfig->reparseConfiguration();
    KConfigGroup conf = kwinConfig->group(QStringLiteral("Effect-LiquidMouse"));

    // Controlla la dimensione del cursore di sistema da kcminputrc come scala di base
    double defaultScale = 1.0;
    auto mouseConfig = KSharedConfig::openConfig(QStringLiteral("kcminputrc"));
    KConfigGroup mouseGroup = mouseConfig->group(QStringLiteral("Mouse"));
    int cursorSize = mouseGroup.readEntry("cursorSize", 24);
    if (cursorSize > 0) {
        defaultScale = static_cast<double>(cursorSize) / 24.0;
    }

    double scale = conf.readEntry("Scale", defaultScale);
    double baseRadius = conf.readEntry("DotRadius", Glyph::designRadius * scale);
    m_body.tuning.dotRadius = std::clamp(baseRadius, 3.0, 48.0);

    m_body.tuning.trailLength = conf.readEntry("TrailLength", 160.0 * scale);
    m_body.tuning.trailResponse = conf.readEntry("TrailResponse", 80.0);
    m_body.tuning.bounce = conf.readEntry("Bounce", 0.5);
    m_body.tuning.splashes = conf.readEntry("Splashes", true);

    int form = conf.readEntry("Form", 1); // 0 = Dot, 1 = Arrow
    m_body.tuning.form = (form == 0) ? CursorForm::Dot : CursorForm::Arrow;

    int look = conf.readEntry("Appearance", 2); // 0 = Light, 1 = Dark, 2 = Automatic
    if (look == 0) m_body.tuning.appearance = BodyLook::Light;
    else if (look == 1) m_body.tuning.appearance = BodyLook::Dark;
    else m_body.tuning.appearance = BodyLook::Automatic;

    auto globals = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    globals->reparseConfiguration();
    QString scheme = globals->group(QStringLiteral("General")).readEntry("ColorScheme", QStringLiteral("BreezeDark"));
    m_systemIsDark = scheme.contains(QStringLiteral("Dark"), Qt::CaseInsensitive);
}

void LiquidMouseEffect::slotMouseChanged(const QPointF &pos, const QPointF &,
                                         Qt::MouseButtons buttons, Qt::MouseButtons,
                                         Qt::KeyboardModifiers, Qt::KeyboardModifiers) {
    m_lastPos = pos;
    m_mousePressed = (buttons != Qt::NoButton);
    updateCursorContext();
    onTick();
}

void LiquidMouseEffect::slotCursorShapeChanged() {
    updateCursorContext();
    onTick();
}

void LiquidMouseEffect::updateCursorContext() {
    if (Cursors::self() && Cursors::self()->mouse()) {
        if (auto shapeSource = qobject_cast<ShapeCursorSource*>(Cursors::self()->mouse()->source())) {
            QByteArray shapeName = shapeSource->shape().toLower();
            if (shapeName.isEmpty()) {
                m_context = CursorContext::pointer();
                return;
            }

            if (shapeName == "text" || shapeName == "xterm" || shapeName == "ibeam") {
                m_context = CursorContext::text();
                return;
            } else if (shapeName == "pointer" || shapeName == "pointing_hand" || shapeName == "hand2" || shapeName == "hand1" || shapeName == "link") {
                m_context = CursorContext::link();
                return;
            } else if (shapeName == "grab" || shapeName == "openhand") {
                m_context = CursorContext::grab();
                return;
            } else if (shapeName == "grabbing" || shapeName == "closedhand") {
                m_context = CursorContext::grabbing();
                return;
            } else if (shapeName == "crosshair") {
                m_context = CursorContext::crosshair();
                return;
            } else if (shapeName == "not-allowed" || shapeName == "forbidden" || shapeName == "circle" || shapeName == "no-drop") {
                m_context = CursorContext::notAllowed();
                return;
            }

            // 1. Ridimensionamento Diagonale (\ Alto-SX <-> Basso-DX)
            // DEVE precedere i controlli orizzontali/verticali perché "nw-resize" e "se-resize"
            // contengono le sottostringhe "w-resize" ed "e-resize"!
            if (shapeName.contains("nwse") || shapeName.contains("nw")
                || shapeName.contains("se-resize") || shapeName == "se"
                || shapeName.contains("top_left") || shapeName.contains("bottom_right")
                || shapeName.contains("top-left") || shapeName.contains("bottom-right")
                || shapeName.contains("fdiag") || shapeName.contains("bd_double_arrow")) {
                double invSqrt2 = 1.0 / std::sqrt(2.0);
                m_context = CursorContext::resize(QPointF(invSqrt2, invSqrt2), true, false);
                return;
            }

            // 2. Ridimensionamento Diagonale (/ Alto-DX <-> Basso-SX)
            if (shapeName.contains("nesw") || shapeName.contains("ne")
                || shapeName.contains("sw-resize") || shapeName == "sw"
                || shapeName.contains("top_right") || shapeName.contains("bottom_left")
                || shapeName.contains("top-right") || shapeName.contains("bottom-left")
                || shapeName.contains("bdiag") || shapeName.contains("fd_double_arrow")) {
                double invSqrt2 = 1.0 / std::sqrt(2.0);
                m_context = CursorContext::resize(QPointF(invSqrt2, -invSqrt2), true, false);
                return;
            }

            // 3. Ridimensionamento Verticale (↕)
            if (shapeName.contains("row-resize") || shapeName.contains("row_resize")
                || shapeName.contains("ns-resize") || shapeName.contains("ns_resize")
                || shapeName == "n-resize" || shapeName == "s-resize"
                || shapeName.contains("size_ver") || shapeName.contains("split_v")
                || shapeName.contains("top_side") || shapeName.contains("bottom_side")
                || shapeName.contains("v_double_arrow")) {
                m_context = CursorContext::resize(QPointF(0.0, 1.0), true, false);
                return;
            }

            // 4. Ridimensionamento Orizzontale (↔)
            if (shapeName.contains("col-resize") || shapeName.contains("col_resize")
                || shapeName.contains("ew-resize") || shapeName.contains("ew_resize")
                || shapeName == "e-resize" || shapeName == "w-resize"
                || shapeName.contains("size_hor") || shapeName.contains("split_h")
                || shapeName.contains("right_side") || shapeName.contains("left_side")
                || shapeName.contains("h_double_arrow")) {
                m_context = CursorContext::resize(QPointF(1.0, 0.0), true, false);
                return;
            }
        }
    }
    m_context = CursorContext::pointer();
}

void LiquidMouseEffect::onTick() {
    double dt = m_frameTimer.restart() / 1000.0;
    if (dt <= 0.0 || dt > 0.1) dt = 0.016;

    m_body.step(m_lastPos, dt, m_context, m_mousePressed, true);

    // Calculate dirty region to repaint
    double shakeFactor = std::max(1.0, m_body.shakeScale.value);
    double reach = (m_body.tuning.reach() + 32.0) * shakeFactor;
    Rect newDamage(static_cast<int>(m_lastPos.x() - reach),
                   static_cast<int>(m_lastPos.y() - reach),
                   static_cast<int>(2 * reach),
                   static_cast<int>(2 * reach));

    Rect totalDamage = m_lastDamage.united(newDamage);
    m_lastDamage = newDamage;

    if (!totalDamage.isEmpty()) {
        effects->addRepaint(totalDamage);
    }
}

void LiquidMouseEffect::prePaintScreen(ScreenPrePaintData &data) {
    effects->prePaintScreen(data);
}

bool LiquidMouseEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport,
                                    int mask, const Region &deviceRegion, LogicalOutput *screen) {
    // 1. Paint underlying desktop windows
    bool ok = effects->paintScreen(renderTarget, viewport, mask, deviceRegion, screen);
    if (!ok || !m_enabled) {
        return ok;
    }

    // 2. Render the liquid cursor
    LiquidBody::Shape shape = m_body.shape();
    if (shape.isEmpty()) {
        return true;
    }

    LiquidRenderer::Colors colors = LiquidRenderer::resolveColors(m_body.tuning.appearance, m_systemIsDark);

    if (effects->isOpenGLCompositing()) {
        QPointF offset;
        QImage cursorImg = LiquidRenderer::renderToImage(shape, colors, offset);
        if (!cursorImg.isNull()) {
            auto texture = GLTexture::upload(cursorImg);
            if (texture) {
                ShaderBinder binder(ShaderTrait::MapTexture);
                QMatrix4x4 mvp = viewport.projectionMatrix();

                QPointF renderPos = m_lastPos + offset;
                mvp.translate(renderPos.x(), renderPos.y());
                binder.shader()->setUniform(GLShader::Mat4Uniform::ModelViewProjectionMatrix, mvp);

                texture->bind();
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                texture->render(QSizeF(cursorImg.width(), cursorImg.height()));
                texture->unbind();
            }
        }
    }
    return true;
}

void LiquidMouseEffect::postPaintScreen() {
    effects->postPaintScreen();
    if (!m_body.isSettled()) {
        effects->addRepaint(m_lastDamage);
    }
}

#ifdef OVERRIDE_KWIN_VERSION
#undef EffectPluginFactory_iid
#define EffectPluginFactory_iid "org.kde.kwin.EffectPluginFactory" OVERRIDE_KWIN_VERSION
#endif

class LiquidMouseEffectFactory : public EffectPluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID EffectPluginFactory_iid FILE "../metadata.json")
    Q_INTERFACES(KPluginFactory)

public:
    LiquidMouseEffectFactory() {}
    ~LiquidMouseEffectFactory() override {}

    bool isSupported() const override {
        return LiquidMouseEffect::supported();
    }
    bool enabledByDefault() const override {
        return false;
    }
    Effect *createEffect() const override {
        return new LiquidMouseEffect();
    }
};

} // namespace KWin

#include "liquidmouse_effect.moc"
