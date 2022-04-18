#include <cstdint>

#include <X11/Xatom.h>
#include <X11/X.h>

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>

#include <FL/Fl_Roller.H>

class TransparentWidget : virtual public Fl_Widget
{
public:
    TransparentWidget(int X, int Y, int W, int H, const char *L = 0):
        Fl_Widget(X, Y, W, H, L),
        widget_alpha_(1.0f),
        blend_label_(false),
        bg_image_(0),
        fg_image_(0),
        images_sz_(0)
    {
        box(FL_UP_BOX);
        if (W < 0)
        {
            W = 0;
        }
        if (H < 0)
        {
            H = 0;
        }
        resize_images(W * H * 3);
    }

    virtual ~TransparentWidget()
    {
        resize_images(0);
    }

    float get_alpha()
    {
        return widget_alpha_;
    }

    void set_alpha(float f_new_alpha)
    {
        if (f_new_alpha < 0.0f)
        {
            widget_alpha_ = 0.0f;
        }
        else
        if (f_new_alpha > 1.0f)
        {
            widget_alpha_ = 1.0f;
        }
        else
        {
            widget_alpha_ = f_new_alpha;
        }
    }

    bool blend_label()
    {
        return blend_label_;
    }

    void blend_label(bool a)
    {
        blend_label_ = a;
    }

    virtual void resize(int X, int Y, int W, int H)
    {
        if ((W != w()) || (H != h()))
        {
            resize_images(W * H * 3);
        }
        Fl_Widget::resize(X, Y, W, H);
    }

    virtual void draw()
    {
        // FIXME : if bounds of this widget are outside top window,
        // fl_read_image may return an image with black rectangle
        if ((widget_alpha_ != 1.0f) && (bg_image_))
        {
            fl_read_image(bg_image_, x(), y(), w(), h());
        }

        fl_push_clip(x(), y(), w(), h());

        draw_box(box(), x(), y(), w(), h(), color());
        if (blend_label_)
        {
            draw_label();
        }

        if ((bg_image_) && (fg_image_))
        {
            fl_read_image(fg_image_, x(), y(), w(), h());
            float f_inv_alpha = 1.0f - widget_alpha_;
            unsigned int max = w() * h() * 3;
            for (unsigned int _i = 0; _i < max; _i += 3)
            {
                for (unsigned int _d = 0; _d < 3; _d++)
                {
                    float fg = float(fg_image_[_i + _d]) * widget_alpha_;
                    float bg = float(bg_image_[_i + _d]) * f_inv_alpha;
                    float fc = fg + bg + 0.5f;
                    if (fc > 255.0f)
                    {
                        fg_image_[_i + _d] = 255;
                    }
                    else
                    {
                        fg_image_[_i + _d] = (unsigned char)fc;
                    }
                }
            }

            fl_draw_image(fg_image_, x(), y(), w(), h());
        }

        if (!blend_label_)
        {
            draw_label();
        }

        fl_pop_clip();
    }

    void redraw()
    {
        // We need to redraw the parent first to get the blending in our
        // draw() method to work... This is not ideal...
        this->parent()->redraw();
    }

protected:
    float widget_alpha_;
    bool blend_label_;
    unsigned char *bg_image_;
    unsigned char *fg_image_;
    int images_sz_;

    void resize_images(int nsz)
    {
        if ((images_sz_ == nsz) || (nsz < 0))
        {
            return;
        }
        delete[] bg_image_;
        bg_image_ = 0;
        delete[] fg_image_;
        fg_image_ = 0;
        images_sz_ = 0;
        if (!nsz)
        {
            return ;
        }
        if ((bg_image_ = new unsigned char[nsz + 1]))
        {
            if ((fg_image_ = new unsigned char[nsz + 1]))
            {
                images_sz_ = nsz;
            }
            else
            {
                delete[] bg_image_;
                bg_image_ = 0;
            }
        }
    } // resize_images

}; // TransparentWidget

#pragma GCC diagnostic ignored "-Winaccessible-base"
#pragma GCC diagnostic ignored "-Wreorder"

template<class T> class xparent : public TransparentWidget, public T
{
public:
    xparent(int X, int Y, int W, int H, const char *L=0) : TransparentWidget(X,Y,W,H,L), T(X,Y,W,H,L), Fl_Widget(X,Y,W,H,L)
    {
        set_alpha(0.35f);
        blend_label(true);
        T::box(FL_THIN_UP_BOX);
        T::color(0xE8E8E800);
    }

    virtual void draw()
    {
        int xx = T::x();
        int yy = T::y();
        int ww = T::w();
        int hh = T::h();

        // FIXME : if bounds of this widget are outside top window,
        // fl_read_image may return an image with black rectangle
        if ((widget_alpha_ != 1.0f) && (bg_image_))
        {
            fl_read_image(bg_image_, xx, yy, ww, hh);
        }

        fl_push_clip(xx, yy, ww, hh);

        T::draw();

        if ((bg_image_) && (fg_image_))
        {
            fl_read_image(fg_image_, xx, yy, ww, hh);
            float f_inv_alpha = 1.0f - widget_alpha_;
            unsigned int max = ww * hh * 3;
            for (unsigned int _i = 0; _i < max; _i += 3)
            {
                for (unsigned int _d = 0; _d < 3; _d++)
                {
                    float fg = float(fg_image_[_i + _d]) * widget_alpha_;
                    float bg = float(bg_image_[_i + _d]) * f_inv_alpha;
                    float fc = fg + bg + 0.5f;
                    if (fc > 255.0f)
                    {
                        fg_image_[_i + _d] = 255;
                    }
                    else
                    {
                        fg_image_[_i + _d] = (unsigned char)fc;
                    }
                }
            }

            fl_draw_image(fg_image_, xx, yy, ww, hh);
        }

        fl_pop_clip();
    }
};

class TransparentRoller : public Fl_Roller
{
public:
    TransparentRoller(int X, int Y, int W, int H, const char *L = 0):
            Fl_Roller(X, Y, W, H, L),
            widget_alpha_(1.0f),
            blend_label_(false),
            bg_image_(0),
            fg_image_(0),
            images_sz_(0)
    {
        box(FL_UP_BOX);
        if (W < 0)
        {
            W = 0;
        }
        if (H < 0)
        {
            H = 0;
        }
        resize_images(W * H * 3);
    }

    virtual ~TransparentRoller()
    {
        resize_images(0);
    }

    float get_alpha()
    {
        return widget_alpha_;
    }

    void set_alpha(float f_new_alpha)
    {
        if (f_new_alpha < 0.0f)
        {
            widget_alpha_ = 0.0f;
        }
        else
        if (f_new_alpha > 1.0f)
        {
            widget_alpha_ = 1.0f;
        }
        else
        {
            widget_alpha_ = f_new_alpha;
        }
    }

    bool blend_label()
    {
        return blend_label_;
    }

    void blend_label(bool a)
    {
        blend_label_ = a;
    }

    virtual void resize(int X, int Y, int W, int H)
    {
        if ((W != w()) || (H != h()))
        {
            resize_images(W * H * 3);
        }
        Fl_Widget::resize(X, Y, W, H);
    }

    virtual void draw()
    {
        // FIXME : if bounds of this widget are outside top window,
        // fl_read_image may return an image with black rectangle
        if ((widget_alpha_ != 1.0f) && (bg_image_))
        {
            fl_read_image(bg_image_, x(), y(), w(), h());
        }

        fl_push_clip(x(), y(), w(), h());

        Fl_Roller::draw();

        //draw_box(box(), x(), y(), w(), h(), color());
//        if (blend_label_)
//        {
//            draw_label();
//        }

        if ((bg_image_) && (fg_image_))
        {
            fl_read_image(fg_image_, x(), y(), w(), h());
            float f_inv_alpha = 1.0f - widget_alpha_;
            unsigned int max = w() * h() * 3;
            for (unsigned int _i = 0; _i < max; _i += 3)
            {
                for (unsigned int _d = 0; _d < 3; _d++)
                {
                    float fg = float(fg_image_[_i + _d]) * widget_alpha_;
                    float bg = float(bg_image_[_i + _d]) * f_inv_alpha;
                    float fc = fg + bg + 0.5f;
                    if (fc > 255.0f)
                    {
                        fg_image_[_i + _d] = 255;
                    }
                    else
                    {
                        fg_image_[_i + _d] = (unsigned char)fc;
                    }
                }
            }

            fl_draw_image(fg_image_, x(), y(), w(), h());
        }

//        if (!blend_label_)
//        {
//            draw_label();
//        }

        fl_pop_clip();
    }

    void redraw()
    {
        // We need to redraw the parent first to get the blending in our
        // draw() method to work... This is not ideal...
        this->parent()->redraw();
    }

private:
    float widget_alpha_;
    bool blend_label_;
    int images_sz_;
    unsigned char *fg_image_;
    unsigned char *bg_image_;

    void resize_images(int nsz)
    {
        if ((images_sz_ == nsz) || (nsz < 0))
        {
            return;
        }
        delete[] bg_image_;
        bg_image_ = 0;
        delete[] fg_image_;
        fg_image_ = 0;
        images_sz_ = 0;
        if (!nsz)
        {
            return ;
        }
        if ((bg_image_ = new unsigned char[nsz + 1]))
        {
            if ((fg_image_ = new unsigned char[nsz + 1]))
            {
                images_sz_ = nsz;
            }
            else
            {
                delete[] bg_image_;
                bg_image_ = 0;
            }
        }
    } // resize_images

}; // TransparentRoller
