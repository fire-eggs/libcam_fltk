// TODO disable tab tooltips inside the tab [inner group with no tooltip?]

#ifdef __CLION_IDE__
#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-auto"
#endif

#include <iostream>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Value_Slider.H>

Fl_Menu_Bar *_menu;
char *_loadfile;

// Camera settings
bool hflip;
bool vflip;
double bright;
double saturate;
double sharp;
double contrast;
double evComp;

// Inter-thread communication
bool stateChange;
extern void fire_proc_thread(int argc, char ** argv);

static void onReset(Fl_Widget *, void *);

static Fl_Value_Slider *makeSlider(int x, int y, const char *label, int min, int max, int def)
{
    // TODO how to change the size of the "value" label?

    Fl_Value_Slider* o = new Fl_Value_Slider(x, y, 200, 20, label);
    o->type(5);
    o->box(FL_DOWN_BOX);
    o->bounds(min, max);
    o->value(def);
    o->labelsize(14);
    o->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    return o;
}

static void onBright(Fl_Widget *w, void *)
{
    bright = ((Fl_Value_Slider *)w)->value();
    std::cerr << "brightness: " << bright << std::endl;
    stateChange = true;
}
static void onContrast(Fl_Widget *w, void *)
{
    contrast = ((Fl_Value_Slider *)w)->value();
    std::cerr << "contrast: " << contrast << std::endl;
    stateChange = true;
}
static void onSharp(Fl_Widget *w, void *)
{
    sharp = ((Fl_Value_Slider *)w)->value();
    std::cerr << "sharpness: " << sharp << std::endl;
    stateChange = true;
}
static void onSaturate(Fl_Widget *w, void *)
{
    saturate = ((Fl_Value_Slider *)w)->value();
    std::cerr << "saturation: " << saturate << std::endl;
    stateChange = true;
}
static void onEvComp(Fl_Widget *w, void *)
{
    evComp = ((Fl_Value_Slider *)w)->value();
    std::cerr << "Ev comp: " << evComp << std::endl;
    stateChange = true;
}
static void onHFlip(Fl_Widget*w, void *)
{
    hflip = ((Fl_Check_Button *)w)->value();
    std::cerr << "H-Flip: " << hflip << std::endl;
    stateChange = true;
}
static void onVFlip(Fl_Widget*w, void *)
{
    vflip = ((Fl_Check_Button *)w)->value();
    std::cerr << "V-Flip: " << vflip << std::endl;
    stateChange = true;
}

#define KBR_UPD_PREVIEW 1001

class MainWin : public Fl_Double_Window
{
public:

    Fl_Value_Slider *m_slBright;
    Fl_Value_Slider *m_slContrast;
    Fl_Value_Slider *m_slSaturate;
    Fl_Value_Slider *m_slSharp;
    Fl_Value_Slider *m_slevComp;
    Fl_Check_Button *m_chkHflip;
    Fl_Check_Button *m_chkVflip;

    static const int MAGIC_Y = 30;
    // TODO "25" is font height? label height?

    MainWin(int x, int y, int w, int h) : Fl_Double_Window(x, y, w,h)
    {
        //grp = new Fl_Group(0, 25, w, h-25);

        int magicW = w - 20;
        int magicH = h - 50;

//        Fl_Box *box1 = new Fl_Box(50, 50, magicW, magicH, "Box 1");
//        box1->type(FL_BORDER_BOX);
//        box1->labelfont(1);

        Fl_Tabs *tabs = new Fl_Tabs(10, MAGIC_Y, magicW, magicH);
        magicH -= 25;
        Fl_Group *t1 = makeSettingsTab(magicW, magicH);
        Fl_Group *t5 = makeZoomTab(magicW, magicH);
        Fl_Group *t2 = makeCaptureTab(magicW, magicH);
        Fl_Group *t3 = makeVideoTab(magicW, magicH);
        Fl_Group *t4 = makeTimelapseTab(magicW, magicH);
        tabs->end();
        //Fl_Group::current()->resizable(tabs);

/*
        _crop = new cropBox(_img, 0, 25, w/2, h-25);
        _preview = new Fl_Box(w/2, 25, w/2, h-25);
        _preview->box(FL_DOWN_FRAME);
*/
        //grp->end();
        resizable(this);
    }

    Fl_Group *makeSettingsTab(int w, int h)
    {
        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Settings");
        o->tooltip("Configure camera settings");

        m_chkHflip = new Fl_Check_Button(270, MAGIC_Y+60, 150, 20, "Horizontal Flip");
        m_chkHflip->callback(onHFlip);
        m_chkHflip->tooltip("Flip camera image horizontally");
        m_chkVflip = new Fl_Check_Button(270, MAGIC_Y+90, 150, 20, "Vertical Flip");
        m_chkVflip->callback(onVFlip);
        m_chkVflip->tooltip("Flip camera image vertically");

        Fl_Button *bReset = new Fl_Button(270, MAGIC_Y+200, 100, 25, "Reset");
        bReset->tooltip("Reset all settings to default");
        bReset->callback(onReset, this);

        int slidX = 40;
        int slidY = MAGIC_Y + 60;
        int yStep = 50;
        m_slBright = makeSlider(slidX, slidY, "Brightness", -1, 1, 0);
        m_slBright->callback(onBright);
        m_slBright->tooltip("-1.0: black; 1.0: white; 0.0: standard");
        slidY += yStep;
        m_slContrast = makeSlider(slidX, slidY, "Contrast", 0, 5, 1);
        m_slContrast->callback(onContrast);
        m_slContrast->tooltip("0: minimum; 1: default; 1+: extra contrast");
        slidY += yStep;
        m_slSaturate = makeSlider(slidX, slidY, "Saturation", 0, 5, 1);
        m_slSaturate->callback(onSaturate);
        m_slSaturate->tooltip("0: minimum; 1: default; 1+: extra color saturation");
        slidY += yStep;
        m_slSharp = makeSlider(slidX, slidY, "Sharpness", 0, 5, 1);
        m_slSharp->callback(onSharp);
        m_slSharp->tooltip("0: minimum; 1: default; 1+: extra sharp");
        slidY += yStep;
        m_slevComp = makeSlider(slidX, slidY, "EV Compensation", -10, 10, 0);
        m_slevComp->callback(onEvComp);
        m_slevComp->tooltip("-10 to 10 stops");
        slidY += yStep;

        o->end();
        //Fl_Group::current()->resizable(o);
        return o;
    }

    Fl_Group *makeCaptureTab(int w, int h)
    {
        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Capture");
        o->tooltip("Image Capture");

        Fl_Button *bStill = new Fl_Button(50, MAGIC_Y+60, 150, 30, "Still Capture");
        bStill->tooltip("Single still image");
        Fl_Button *bBurst = new Fl_Button(50, MAGIC_Y+100, 150, 30, "Burst Capture");
        bStill->tooltip("Capture multiple still images");

        o->end();
        Fl_Group::current()->resizable(o);
        return o;
    }

    Fl_Group *makeVideoTab(int w, int h)
    {
        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Video");
        o->tooltip("Video Capture");

        o->end();
        return o;
    }

    Fl_Group *makeTimelapseTab(int w, int h)
    {
        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "TimeLapse");
        o->tooltip("Make timelapse");

        o->end();
        return o;
    }

    Fl_Group *makeZoomTab(int w, int h)
    {
        Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Zoom");
        o->tooltip("Adjust dynamic zoom");

        o->end();
        return o;
    }

    void resize(int, int, int, int) override;
};

static void onReset(Fl_Widget *w, void *d)
{
    // TODO constants for defaults

    MainWin *mw = (MainWin *)d;
    mw->m_slBright->value(0.0);
    mw->m_slContrast->value(1.0);
    mw->m_slSaturate->value(1.0);
    mw->m_slSharp->value(1.0);
    mw->m_slevComp->value(0.0);
    mw->m_chkHflip->value(false);
    mw->m_chkVflip->value(false);

    bright = 0.0;
    contrast = 1.0;
    sharp = 1.0;
    saturate = 1.0;
    evComp = 0.0;
    hflip = false;
    vflip = false;

    stateChange = true;
}

MainWin* _window;

void MainWin::resize(int x, int y, int w, int h)
{
    int oldw = this->w();
    int oldh = this->h();
    Fl_Double_Window::resize(x, y, w, h);
    if (w == oldw && h == oldh)
        return; // merely moving window

    _menu->size(w, 25);
    //grp->size(w, h-25);
    //_crop->size(w/2,h-25);
    //_preview->size(w/2,h-25);
}

static void popup(Fl_File_Chooser* filechooser) // TODO stolen from FLTK source... genericize?
{
    filechooser->show();

    // deactivate Fl::grab(), because it is incompatible with modal windows
    Fl_Window* g = Fl::grab();
    if (g) Fl::grab(0);

    while (filechooser->shown())
        Fl::wait();

    if (g) // regrab the previous popup menu, if there was one
        Fl::grab(g);
}

void load_cb(Fl_Widget* , void* )
{
    char filename[1024] = "";
    Fl_File_Chooser* choose = new Fl_File_Chooser(filename, "*.*", 0, "Open image file");
    choose->preview(false); // force preview off
    popup(choose);
    _loadfile = (char*)choose->value();
    if (!_loadfile)
        return;

  //Fl_Image *i = loadFile(_loadfile, nullptr);
  //_window->setImage(i);
}

void quit_cb(Fl_Widget* , void* )
{
    exit(0);
}

void save_cb(Fl_Widget*, void*) {
//    _window->savecrop();
}

Fl_Menu_Item mainmenuItems[] =
{
    {"&File", 0, 0, 0, FL_SUBMENU},
    {"&Load...", 0, load_cb},
    {"Save", 0, save_cb},
    {"E&xit", 0, quit_cb},
    {0},
    {0},
};

int handleSpecial(int event)
{
    switch (event)
    {
    case KBR_UPD_PREVIEW:
//        _window->updatePreview();
        return 1;
    default:
        return 0;
    }
    return 1;
}

int main(int argc, char** argv)
{
#if 0
    libcamera::logSetTarget(libcamera::LoggingTargetNone);
#endif

    Fl::lock();

    // TODO : use actual size when building controls?
    MainWin window(100, 100, 500, 500);

    _menu = new Fl_Menu_Bar(0, 0, window.w(), 25);
    _menu->copy(mainmenuItems);

    window.end();

    _window = &window;
    Fl::add_handler(handleSpecial); // handle internal events

    window.show();

#if 0
    fire_proc_thread(argc, argv);
#endif

    return Fl::run();   
}

#ifdef __CLION_IDE__
#pragma clang diagnostic pop
#endif