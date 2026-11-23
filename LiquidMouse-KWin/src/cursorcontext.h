#pragma once

#include <QPointF>

enum class CursorForm {
    Dot,
    Arrow
};

enum class BodyLook {
    Automatic,
    Dark,
    Light
};

enum class CursorContextKind {
    Pointer,
    Text,
    VerticalText,
    Link,
    Grab,
    Grabbing,
    Resize,
    Crosshair,
    NotAllowed,
    Copy,
    Alias,
    Menu,
    Zoom,
    Other,
    Hidden
};

struct CursorContext {
    CursorContextKind kind = CursorContextKind::Pointer;
    double halfHeight = 8.0;              // For Text (half height of I-beam)
    QPointF direction = QPointF(1.0, 0.0); // For Resize
    bool bothWays = true;
    bool divider = false;
    bool magnifies = true;                 // For Zoom

    static CursorContext pointer() { return {CursorContextKind::Pointer}; }
    static CursorContext link() { return {CursorContextKind::Link}; }
    static CursorContext text(double halfH = 8.0) {
        CursorContext c;
        c.kind = CursorContextKind::Text;
        c.halfHeight = halfH;
        return c;
    }
    static CursorContext verticalText() {
        CursorContext c;
        c.kind = CursorContextKind::VerticalText;
        return c;
    }
    static CursorContext grab() { return {CursorContextKind::Grab}; }
    static CursorContext grabbing() { return {CursorContextKind::Grabbing}; }
    static CursorContext crosshair() { return {CursorContextKind::Crosshair}; }
    static CursorContext notAllowed() { return {CursorContextKind::NotAllowed}; }
    static CursorContext copy() { return {CursorContextKind::Copy}; }
    static CursorContext alias() { return {CursorContextKind::Alias}; }
    static CursorContext menu() { return {CursorContextKind::Menu}; }
    static CursorContext zoom(bool mag = true) {
        CursorContext c;
        c.kind = CursorContextKind::Zoom;
        c.magnifies = mag;
        return c;
    }
    static CursorContext resize(const QPointF &dir, bool both = true, bool div = false) {
        CursorContext c;
        c.kind = CursorContextKind::Resize;
        c.direction = dir;
        c.bothWays = both;
        c.divider = div;
        return c;
    }
};
