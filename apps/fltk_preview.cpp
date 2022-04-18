#include "fltk_preview.h"


#include "xparentWidget.cpp"
#include <FL/Fl_Roller.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>

class Fl_Horz_Roller : public Fl_Roller
{
public:
    Fl_Horz_Roller(int X,int Y,int W,int H,const char *l) : Fl_Roller(X,Y,W,H,l) 
    {
        type(FL_HORIZONTAL);
    }
};


//TransparentRoller *alphaRoller;
TransparentWidget *xHFlip;
TransparentWidget *xroller;
TransparentWidget *xVFlip;

#pragma GCC diagnostic ignored "-Wunused-variable"


FltkPreview::FltkPreview(int x, int y, int w, int h) : Preview(nullptr), last_fd_(-1), first_time_(true)
{
    max_image_height_ = 600; // TODO arbitrary
    max_image_width_ =  800;
    
    _win = new Fl_Double_Window(x,y,w,h,"libcam_fltk_preview");    
    _box = new Fl_Box(0,0,w,h);
    
//    Fl_Group *o = new Fl_Group(5, 5, 200, 100);
//    o->box(FL_BORDER_FRAME);
//    o->color(FL_BLUE);
    
    Fl_Button *btn = new Fl_Button(5, 5, 100, 20, "Blah");
/*    
    alphaRoller = new TransparentRoller(5, 30, 100, 20);
    alphaRoller->set_alpha(0.5f);
    alphaRoller->blend_label(1);
    alphaRoller->box(FL_THIN_UP_BOX);
    alphaRoller->color(0xE8E8E800);
    alphaRoller->type(1); // horizontal
*/    
    xroller = new xparent<Fl_Horz_Roller>(5, 30, 180, 20);
    //xRoller = &alphaRoller;
    
    xHFlip = new xparent<Fl_Button>(5, 55, 75, 20, "Flip H");
    //alphaBtn.Fl_Button::callback(onBtn);
    //xHFlip = &alphaBtn;

    xVFlip = new xparent<Fl_Check_Button>(5, 80, 75, 20, "Flip V");
    
//    o->end();
    
    _win->end();
}

FltkPreview::FltkPreview(Options const *options) : Preview(options), last_fd_(-1), first_time_(true)
{
/*    
    // controls the max viewfinder size [see ConfigureViewfinder]
    max_image_height_ = 768; // TODO arbitrary
    max_image_width_ = 1024;
    
	x_ = options_->preview_x;
	y_ = options_->preview_y;
	width_ = options_->preview_width;
	height_ = options_->preview_height;

    _win = new Fl_Double_Window(x_,y_,width_,height_,"libcam_fltk_preview");    
    _box = new Fl_Box(20,0,width_-20,height_);
    
    xparent<Fl_Roller>alphaRoller(5, 45, 180, 20);
    ((Fl_Roller *)&alphaRoller)->type(FL_HORIZONTAL);
    xRoller = &alphaRoller;
    
    xparent<Fl_Button> alphaBtn(5, 70, 180, 20, "Flip H");
    //alphaBtn.Fl_Button::callback(onBtn);
    xHFlip = &alphaBtn;
    
    _win->end();
    
    // TODO creator calls _win->show() 
    */
}

FltkPreview::~FltkPreview()
{
    std::cerr << "fp dtor" << std::endl;
    
    // TODO what to clean up, if anything?
    //_win->hide();
}

void FltkPreview::getWindowPos(int &x, int& y)
{
    x = _win->x();
    y = _win->y();
}

void FltkPreview::makeWindow(char const *name)
{
}

void FltkPreview::makeBuffer(int fd, size_t size, StreamInfo const &info, Buffer &buffer)
{
}

void FltkPreview::SetInfoText(const std::string &text)
{
    //std::cerr << "fp SIT" << std::endl;
    Fl::lock();
    if (!text.empty())
            _win->label(text.c_str());
    Fl::unlock();
}
    
	// Display the buffer. You get given the fd back in the BufferDoneCallback
	// once its available for re-use.
void FltkPreview::Show(int fd, libcamera::Span<uint8_t> span, StreamInfo const &info)
{
    //std::cerr << "fp Show" << std::endl;
    
    Fl::lock();
    
    auto oldimg = _box->image();
    delete oldimg;
    
    auto ptr = span.data();
    Fl_RGB_Image *img = new Fl_RGB_Image(ptr, info.width, info.height, 3, info.stride);
    //img->alloc_array = 1;
    _box->image(img);
    //_box->redraw();
    _win->redraw(); // for controls

    Fl::unlock();
    Fl::awake();
    
	if (last_fd_ >= 0)
		done_callback_(last_fd_);
	last_fd_ = fd;    
}

// Reset the preview window, clearing the current buffers and being ready to
// show new ones.
void FltkPreview::Reset()
{
    Fl::lock();
    //std::cerr << "fp Reset" << std::endl;
// TODO    
//	for (auto &it : buffers_)
//		glDeleteTextures(1, &it.second.texture);
	buffers_.clear();
	last_fd_ = -1;
    //_box->image(nullptr);
	first_time_ = true;    
    Fl::unlock();
}

	// Check if the window manager has closed the preview.
bool FltkPreview::Quit()
{
    //std::cerr << "fp Quit" << std::endl;
    
    return false;
}

