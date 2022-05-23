#include <iostream>
#include "mainwin.h"
#include <FL/Fl_Float_Input.H>

double servoTDegree = 90.0;
double servoBDegree = 90.0;

Fl_Roller *rollT;
Fl_Roller *rollB;
Fl_Input *currentT;
Fl_Input *currentB;

static void cbStep(Fl_Widget *w, void *d)
{
    Fl_Spinner *ww = (Fl_Spinner *)w;
    Fl_Roller *wd  = (Fl_Roller *)d;
    double val = ww->value();
    wd->step(val);
}

static void setValue(Fl_Input *w, double val)
{
    char buffer[100];
    sprintf(buffer, "%g", val);
    w->value(buffer);
}

static void cbRollT(Fl_Widget *w, void *d)
{
    auto ww = (Fl_Roller *)w;
    double val = ww->value();
        
    setValue(currentT, val);
    servoTDegree = val;
    
    //std::cerr << "T: " << val << std::endl;
    
    // TODO need a delay    ??
}

static void cbRollB(Fl_Widget *w, void *d)
{
    auto ww = (Fl_Roller *)w;
    double val = ww->value();
        
    setValue(currentB, val);
    servoBDegree = val;
    
    //std::cerr << "B: " << val << std::endl; 
    
    // TODO need a delay ??
}

static void cbReset(Fl_Widget *w, void *d)
{
    servoBDegree = 90.0;
    servoTDegree = 90.0;
    
    rollT->value(90); // TODO why isn't the callback invoked??
    rollB->value(90);
    
    setValue(currentT, 90);
    setValue(currentB, 90);
    
    std::cerr << "T: " << servoTDegree << std::endl;
    std::cerr << "B: " << servoBDegree << std::endl;   
}


Fl_Group *MainWin::makeServoTab(int w, int h)
{
    Fl_Group *o = new Fl_Group(10,MAGIC_Y+25,w,h, "Servo");

    int Y = MAGIC_Y + 60;
    int X = 75;
    int W = 60;
    
    currentT = new Fl_Float_Input(X, Y, W, 25, "Position:");
    setValue(currentT, servoTDegree);
    currentT->readonly(1);
    
    int rollH = 100;
    rollT = new Fl_Roller(X+70, Y, 25, rollH);
    rollT->type(FL_VERTICAL);
    rollT->bounds(15,180); // TODO get actual value
    rollT->callback(cbRollT);
    rollT->value(servoTDegree);
    rollT->step(0.25);
    
    Fl_Spinner *stepT = new Fl_Spinner(X, Y+35, W, 25, "Step:");
    stepT->value(0.25);
    stepT->minimum(0.25);
    stepT->maximum(5);
    stepT->step(0.25);
    stepT->callback(cbStep, rollT);

    {
        Fl_Box* o = new Fl_Box(15, Y+rollH+5, w-30, 3);
        o->box(FL_BORDER_BOX);
        o->color(FL_FOREGROUND_COLOR);
    }
    
    Y+=rollH+15;

    currentB = new Fl_Float_Input(X, Y, W, 25, "Position:");
    setValue(currentB, servoBDegree);
    currentB->readonly(1);
    
    rollB = new Fl_Roller(X, Y+30, rollH, 25);
    rollB->type(FL_HORIZONTAL);
    rollB->bounds(0,180);
    rollB->callback(cbRollB);
    rollB->value(servoBDegree);
    rollB->step(0.25);

    Fl_Spinner *stepB = new Fl_Spinner(X, Y+60, W, 25, "Step:");
    stepB->value(0.25);
    stepB->minimum(0.25);
    stepB->maximum(5);
    stepB->step(0.25);
    stepB->callback(cbStep, rollB);

    Y+=100;
    
    {
        Fl_Box* o = new Fl_Box(15, Y, w-30, 3);
        o->box(FL_BORDER_BOX);
        o->color(FL_FOREGROUND_COLOR);
    }
    
    Fl_Button *bReset = new Fl_Button(w-125, Y+30, 100, 25, "Reset");
    bReset->callback(cbReset);
    
    o->end();
    return o;
}
