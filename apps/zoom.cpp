//
// Created by kevin on 2/13/22.
//

#ifdef NOISY
#include <iostream>
#endif

#include <FL/fl_draw.H>
#include "settings.h"
#include "prefs.h"
#include "mainwin.h"
#include "zoom.h"

// Zoom settings: implementation
double _zoom;
double _panH;
double _panV;
int _zoomChoice;
bool _lever;

extern Prefs *_prefs;
extern MainWin* _window;

static double zoomVals[] = {1.0, 0.8, 2.0 / 3.0, 0.5, 0.4, 1.0 / 3.0, 0.25, 0.2 };
Fl_Menu_Item menu_cmbZoom[] =
        {
                {"100%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"125%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"150%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"200%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"250%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"300%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"400%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {"500%", 0, 0, 0, 0, (uchar)FL_NORMAL_LABEL, 0, 14, 0},
                {0,     0, 0, 0, 0,                      0, 0,  0, 0}
        };

class DrawZoom : public Fl_Widget
{
public:
    DrawZoom(int X, int Y, int W, int H, const char *L= nullptr) : Fl_Widget(X,Y,W,H,L) {}
    void draw()
    {
        fl_draw_box(FL_BORDER_BOX, x(), y(), w(), h(), FL_BACKGROUND_COLOR);
        fl_draw_box(FL_BORDER_FRAME, x(), y(), w(), h(), FL_BLACK);

        // TODO common code w/ camThread.h
        int dx = w() * (_panH + (1.0 - _zoom) / 2.0);
        int dy = h() * (_panV + (1.0 - _zoom) / 2.0);
        int dh = h() * _zoom;
        int dw = w() * _zoom;
        fl_draw_box(FL_BORDER_FRAME, x()+dx, y()+dy, dw, dh, FL_BLUE);
    }
};

DrawZoom *dZoom;

static void commonZoom(MainWin *mw, int zoomdex);

static void onZoomChange(Fl_Widget *w, void *d)
{
    MainWin *mw = static_cast<MainWin *>(d);
    _zoomChoice = ((Fl_Choice *)w)->value();
    commonZoom(mw, _zoomChoice);

    if (_zoomChoice && mw->m_chkLever->value())
    {
        _panV = -_panV;
        _panH = -_panH;
    }

    _lever = mw->m_chkLever->value(); // HACK no callback for lever change so update frequently

    dZoom->redraw();

    onStateChange();
}

static void onPanH(Fl_Widget *w, void *d)
{
    _panH = ((Fl_Roller *)w)->value();

#ifdef NOISY
    std::cerr << "panH: " << _panH << std::endl;
#endif

    MainWin *mw = (MainWin *)d;
    mw->m_rlPanH->value(_panH);

    _lever = mw->m_chkLever->value(); // HACK no callback for lever change so update frequently
    if (_lever)
    {
        _panH = -_panH;
    }

    onZoomChange(mw->cmbZoom, d);
    //onStateChange();
}

static void onPanV(Fl_Widget *w, void *d)
{
    _panV = ((Fl_Roller *)w)->value();

#ifdef NOISY
    std::cerr << "panV: " << _panV << std::endl;
#endif

    MainWin *mw = (MainWin *)d;
    mw->m_rlPanV->value(_panV);

    _lever = mw->m_chkLever->value(); // HACK no callback for lever change so update frequently
    if (_lever)
    {
        _panV = -_panV;
    }

    onZoomChange(mw->cmbZoom, d);
    //onStateChange();
}

static void onZoomReset(Fl_Widget *, void *d)
{
    MainWin *mw = (MainWin *)d;
    _zoom = 1.0;
    _zoomChoice = 0;
    _panH = +0.0;
    _panV = +0.0;
    _lever = false;

    mw->m_chkLever->value(_lever);
    mw->cmbZoom->value(_zoomChoice);
    mw->m_rlPanH->value(_panH);
    mw->m_rlPanV->value(_panV);
    mw->m_rlPanH ->deactivate();
    mw->m_rlPanV ->deactivate();

    onStateChange();
}

Fl_Group *MainWin::makeZoomTab(int w, int h)
{
    // Restore saved values
    Prefs *setP = _prefs->getSubPrefs("camera");
    _zoomChoice  = setP->get("zoom", 0);
    double panHval  = setP->get("panh",   0.0);
    double panVval  = setP->get("panv",   0.0);
    _lever = (bool)setP->get("lever", false);

    Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Zoom");
    o->tooltip("Adjust dynamic zoom");

    int slidX = 40;
    int slidY = MAGIC_Y+75;

    m_rlPanH = new Fl_Roller(slidX, slidY, 200, 25, "Pan horizontally");
    m_rlPanH->when(FL_WHEN_CHANGED);
    m_rlPanH->callback(onPanH, this);
    m_rlPanH->type(FL_HORIZONTAL);
    m_rlPanH->tooltip("Move the zoom center left or right");

    m_rlPanV = new Fl_Roller(slidX + 220, slidY, 25, 200, "Pan vertically");
    m_rlPanV->when(FL_WHEN_CHANGED);
    m_rlPanV->callback(onPanV, this);
    m_rlPanV->tooltip("Move the zoom center up or down");
    slidY += 70;

    cmbZoom = new Fl_Choice(slidX, slidY, 150, 25, "Zoom:");
    cmbZoom->down_box(FL_BORDER_BOX);
    cmbZoom->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    cmbZoom->menu(menu_cmbZoom);
    cmbZoom->value(_zoomChoice); // NOTE: do before establish callback, don't fire it
    cmbZoom->callback(onZoomChange, this);
    slidY += 50;

    m_chkLever = new Fl_Check_Button(slidX, slidY, 150, 25, "Tripod panning");
    m_chkLever->tooltip("Pan as if using a tripod lever to move the camera");
    m_chkLever->value(_lever);
    slidY += 50;

    Fl_Button *bReset = new Fl_Button(slidX, slidY, 100, 25, "Reset");
    bReset->tooltip("Reset all zoom settings to default");
    bReset->callback(onZoomReset, this);

    slidY += 50;

    dZoom = new DrawZoom(slidX, slidY, 150, 150);

    _panH = panHval;
    m_rlPanH->value(_panH); // TODO confirm is not corrected
    _panV = panVval;
    m_rlPanV->value(_panV); // TODO confirm is not corrected

    _zoom = zoomVals[_zoomChoice];
    commonZoom(this, _zoomChoice);

    // TODO fine control?

    o->end();
    return o;
}

void MainWin::loadZoomSettings()
{
    Prefs *setP = _prefs->getSubPrefs("camera");

    _zoomChoice = setP->get("zoom", 0);
    _lever      = (bool)setP->get("lever", (int)false);
    _panH       = setP->get("panh", 0.0);
    _panV       = setP->get("panv", 0.0);

    commonZoom(this, _zoomChoice);
}

/* Update zoom controls on zoom choice change.
 * Used during init for saved value. Used when user changes zoom level.
 */
static void commonZoom(MainWin *mw, int zoomdex)
{
    _zoom = zoomVals[zoomdex];

    bool oldLever = _lever; // TODO onZoomReset clobbers

    if (zoomdex == 0)
        onZoomReset(nullptr, mw);
    else
    {
        double pHval = mw->m_rlPanH->value();
        double pVval = mw->m_rlPanV->value();

        double range = (1.0 - _zoom) / 2.0;

        mw->m_rlPanH->bounds(-range, range);
        mw->m_rlPanV->bounds(-range, range);

        _panV = mw->m_rlPanV->clamp(pVval);
        mw->m_rlPanV->value(_panV);
        _panH = mw->m_rlPanH->clamp(pHval);
        mw->m_rlPanH->value(_panH);

        mw->m_rlPanH->activate();
        mw->m_rlPanV->activate();
    }

    mw->m_chkLever->value(oldLever);
    _lever = oldLever;
}