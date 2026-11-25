#pragma once

#include <kwin/effect/effect.h>
#ifdef LIQUIDMOUSE_KWIN_HAS_CORE_RECT
#include <kwin/core/rect.h>
#endif
#include <QObject>
#include <QPointF>
#include <QRect>
#include <QElapsedTimer>
#include <QTimer>
#include "liquidbody.h"
#include "liquidrenderer.h"

namespace KWin {

class LiquidMouseEffect : public Effect {
    Q_OBJECT

public:
    LiquidMouseEffect();
    ~LiquidMouseEffect() override;

    static bool supported();

    void reconfigure(ReconfigureFlags flags) override;
    void prePaintScreen(ScreenPrePaintData &data) override;
#if defined(LIQUIDMOUSE_KWIN_PAINTSCREEN_BOOL_REGION_LOGICALOUTPUT)
    [[nodiscard]] bool paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport,
                                  int mask, const Region &deviceRegion, LogicalOutput *screen) override;
#elif defined(LIQUIDMOUSE_KWIN_PAINTSCREEN_VOID_QREGION_LOGICALOUTPUT)
    void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport,
                     int mask, const QRegion &region, LogicalOutput *screen) override;
#else
    void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport,
                     int mask, const QRegion &region, Output *screen) override;
#endif
    void postPaintScreen() override;

    bool isActive() const override;

private Q_SLOTS:
    void slotMouseChanged(const QPointF &pos, const QPointF &oldpos,
                          Qt::MouseButtons buttons, Qt::MouseButtons oldbuttons,
                          Qt::KeyboardModifiers modifiers, Qt::KeyboardModifiers oldmodifiers);
    void slotCursorShapeChanged();
    void onTick();

private:
    void updateCursorContext();
    CursorContext mapCursorShape(Qt::CursorShape shape);

    LiquidBody m_body;
    CursorContext m_context = CursorContext::pointer();
    bool m_mousePressed = false;
    bool m_cursorHidden = false;
    bool m_enabled = true;
    bool m_systemIsDark = true;

    QPointF m_lastPos;
#ifdef LIQUIDMOUSE_KWIN_HAS_CORE_RECT
    Rect m_lastDamage;
#else
    QRect m_lastDamage;
#endif
    QElapsedTimer m_frameTimer;
    QTimer m_animTimer;
};

} // namespace KWin
