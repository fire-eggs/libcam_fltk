//
// Created by kevin on 1/6/22.
//

#ifndef CROPPER_PREFS_H
#define CROPPER_PREFS_H

#define APPLICATION "libcam_fltk"
#define ORGANIZATION "fire-eggs"

#include <FL/Fl_Preferences.H>
#include <string>

class Prefs
{
public:
    Fl_Preferences* _prefs;

    Prefs()
    {
        _prefs = new Fl_Preferences(Fl_Preferences::USER, ORGANIZATION, APPLICATION);
    }

    ~Prefs()
    {
        delete _prefs;
        _prefs = NULL;
    }

    Prefs(Fl_Preferences* group)
    {
        _prefs = group;
    }

    Prefs* getSubPrefs(const char* group)
    {
        if (!_prefs)
            return NULL;
        return new Prefs(new Fl_Preferences(_prefs, group));
    }

    void getWinRect(const char* prefix, int& x, int& y, int& w, int& h)
    {
        std::string n = prefix;
        _prefs->get((n + "_x").c_str(), x,  50);
        _prefs->get((n + "_y").c_str(), y,  50);
        _prefs->get((n + "_w").c_str(), w, 500);
        _prefs->get((n + "_h").c_str(), h, 500);
    }

    void setWinRect(const char* prefix, int x, int y, int w, int h)
    {
        std::string n = prefix;
        _prefs->set((n + "_x").c_str(), x);
        _prefs->set((n + "_y").c_str(), y);
        _prefs->set((n + "_w").c_str(), w);
        _prefs->set((n + "_h").c_str(), h);
        _prefs->flush();
    }

    void set(const char* n, int val)
    {
        _prefs->set(n, val);
        _prefs->flush();
    }

    void set(const char* n, double val)
    {
        _prefs->set(n, val);
        _prefs->flush();
    }

    int get(const char * n, int def)
    {
        int val;
        _prefs->get(n, val, def);
        return val;
    }
    double get(const char * n, double def)
    {
        double val;
        _prefs->get(n, val, def);
        return val;
    }
    bool get(const char *n, bool def)
    {
        return (bool)get(n,(int)def);
    }
};

#endif //CROPPER_PREFS_H
