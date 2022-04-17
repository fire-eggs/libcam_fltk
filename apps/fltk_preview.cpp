#include "fltk_preview.h"

FltkPreview::FltkPreview(int x, int y, int w, int h) : Preview(nullptr), last_fd_(-1), first_time_(true)
{
    _win = new Fl_Double_Window(x,y,w,h,"libcam_fltk_preview");    
    _box = new Fl_Box(0,0,w,h);
    _win->end();
}

FltkPreview::FltkPreview(Options const *options) : Preview(options), last_fd_(-1), first_time_(true)
{
    // controls the max viewfinder size [see ConfigureViewfinder]
    max_image_height_ = 768; // TODO arbitrary
    max_image_width_ = 1024;
    
	x_ = options_->preview_x;
	y_ = options_->preview_y;
	width_ = options_->preview_width;
	height_ = options_->preview_height;

    _win = new Fl_Double_Window(x_,y_,width_,height_,"libcam_fltk_preview");    
    _box = new Fl_Box(0,0,width_,height_);
    _win->end();
    
    // TODO creator calls _win->show() 
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
    
    // NOTE: format must be RGB888
    auto ptr = span.data();
    Fl_RGB_Image *img = new Fl_RGB_Image(ptr, info.width, info.height, 3, info.stride);
    //img->alloc_array = 1;
    _box->image(img);
    _box->redraw();

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

