#pragma once

#include <kwin/effect/effect.h>
#include <kwin/core/rect.h>
#include <QObject>
#include <QPointF>
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
    [[nodiscard]] bool paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport,
                                  int mask, const Region &deviceRegion, LogicalOutput *screen) override;
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
    Rect m_lastDamage;
    QElapsedTimer m_frameTimer;
    QTimer m_animTimer;
};

} // namespace KWin
